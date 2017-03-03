#define _GNU_SOURCE

#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#include "aucont_util.h"

#define STACK_SIZE (1 << 20)

struct start_args {
	bool detatched;
	bool net;
	char* cont_ip;
	int cpu_perc;
	char image_path[1024];
	char** cmd;
};


struct cont_args {
	int pipe_in;
	bool detatched;
	char** cmd;
};


int parse_args(char** argv, struct start_args* args)
{
	int i = 1;
	while (argv[i] != NULL) {
		if (strcmp(argv[i], "-d") == 0) {
			args->detatched = true;
			i += 1;
		} else if (strcmp(argv[i], "--cpu") == 0) {
			args->cpu_perc = atoi(argv[i + 1]);
			i += 2;
		} else if (strcmp(argv[i], "--net") == 0) {
			args->net = true;
			args->cont_ip = argv[i + 1];
			i += 2;
		} else {
			break;
		}
	}

	if (argv[i] != NULL && argv[i + 1] != NULL) {
		if (realpath(argv[i], args->image_path) == NULL) {
			return 1;
		}
		args->cmd = argv + i + 1;
		return 0;
	}

	return 1;
}

#define USAGE "usage: ./aucont_start [-d --cpu CPU_PERC --net IP] IMAGE_PATH CMD [ARGS]\n"

int configure_environment(int pid, struct start_args* args);

int host_side_init(int pid, int pipe_out, struct start_args* args);

int container_side_init(void* args);

int main(int argc, char** argv)
{
	struct start_args args = {
		.detatched = false,
		.net = false,
		.cont_ip = "null",
		.cpu_perc = 100
	};

	if (parse_args(argv, &args)) {
		fprintf(stderr, USAGE);
		return 1;
	}

	if (set_work_directory()) {
		perror("can't set work directory");
		return 1;
	}

	int pipefd[2];

	if (pipe2(pipefd, O_CLOEXEC)) {
		perror("can't get pipe");
		return 1;
	}

	int flags = SIGCHLD 
		| CLONE_NEWIPC
		| CLONE_NEWNS
		| CLONE_NEWPID
		| CLONE_NEWUTS
		| CLONE_NEWUSER
		| CLONE_NEWNET;

	void* stack_top = malloc(STACK_SIZE) + STACK_SIZE;

	struct cont_args cont_args = {
		.pipe_in = pipefd[0],
		.detatched = args.detatched,
		.cmd = args.cmd
	};

	int pid = clone(container_side_init, stack_top, flags, &cont_args);

	if (pid < 0) { 
		perror("can't clone");
		return 1;
	}


	if (configure_environment(pid, &args)) {
		perror("can't configure environment");
		return 1;
	}

	close(pipefd[0]);

	if (host_side_init(pid, pipefd[1], &args)) {
		perror("host side init goes wrong");
		return 1;
	}

	return 0;
}


int configure_environment(int pid, struct start_args* args)
{
	char cmd[CMD_LEN];
	sprintf(cmd, "./env.py %d %s %s", pid, args->image_path, args->cont_ip);
	if (system(cmd)) {
		perror("can't call system");
		return 1;
	}

	return 0;
}

/**
 * HOST-SIDE INITIALIZATION
 */

int configure_cgroup(int pid, int cpu);

int configure_netns(int pid);

int set_id_mapping(char* map_name, int pid, int id);

int host_side_init(int pid, int pipe_out, struct start_args* args)
{
	int msg = -1;

	if (configure_cgroup(pid, args->cpu_perc)) {
		perror("can't configure cgroups");
		write(pipe_out, &msg, sizeof(msg));
		return 1;
	}

	if (args->net) {
		if (configure_netns(pid)) {
			perror("can't configure network");
			write(pipe_out, &msg, sizeof(msg));
			return 1;
		}
	}

	if (set_id_mapping("uid_map", pid, getuid())) {
		perror("can't configure uid mapping");
		write(pipe_out, &msg, sizeof(msg));
		return 1;
	}

	if (set_id_mapping("gid_map", pid, getgid())) {
		perror("can't configure gid mapping");
		write(pipe_out, &msg, sizeof(msg));
		return 1;
	}

	msg = pid;
	write(pipe_out, &msg, sizeof(msg));

	printf("%d\n", pid);

	if (!args->detatched) {
		siginfo_t infop;
		waitid(P_PID, pid, &infop, WEXITED);
	}

	return 0;
}


int configure_cgroup(int pid, int cpu)
{
	char cmd[CMD_LEN];
	sprintf(cmd, "./cgroup.sh %d %d", pid, cpu);
	return system(cmd);
}


#define MAX_IP_LEN 19

int configure_netns(int pid)
{
	int addrs_len = 2 * MAX_IP_LEN + 1;
	char addrs[addrs_len];

	char path[PATH_LEN];
	sprintf(path, "./%d/net_config", pid);

	FILE* netns_config = fopen(path, "r");

	fgets(addrs, addrs_len, netns_config);

	fclose(netns_config);

	char cmd[CMD_LEN];
	sprintf(cmd, "./net.sh %d %s", pid, addrs);

	if (system(cmd)) {
		perror("can't configure netns");
		return 1;
	}

	return 0;
}


#define MAP_LEN 10

int set_id_mapping(char* map_name, int pid, int id)
{
	char path[PATH_LEN];
	sprintf(path, "/proc/%d/%s", pid, map_name);

	int fd = open(path, O_WRONLY);
	if (fd < 0) {
		perror("can't open map file");
		return 1;
	}

	char mapping[MAP_LEN];
	int mapping_len = sprintf(mapping, "0 %d 1", id);

	if (mapping_len != write(fd, mapping, mapping_len)) {
		perror("can't write mapping");
		return 1;
	}

	close(fd);
	return 0;	
}


/*
 * CONTAINER-SIDE INITIALIZATION
 */

#define HOSTNAME "container"

int mount_rootfs(int pid);

int daemonize();

int container_side_init(void* args)
{
	struct cont_args* cont_args = args;

	int pid;
	if ((pid = wait_parent(cont_args->pipe_in)) < 0) {
		perror("problems with host");
		return 1;
	}

	if (mount_rootfs(pid)) {
		perror("can't mount rootfs");
		return 1;
	}

	sethostname(HOSTNAME, strlen(HOSTNAME));

	setuid(0);
	setgid(0);

	if (cont_args->detatched) {
		if (daemonize()) {
			perror("daemonization failed");
			return 1;
		}
	}

	execve(cont_args->cmd[0], cont_args->cmd, NULL);

	perror("can't do execv");
	return 1;
}

int mount_rootfs(int pid)
{
	char rootfs[PATH_LEN];
	sprintf(rootfs, "./%d", pid);

	char old[PATH_LEN];
	sprintf(old, "./%d/old", pid);

	if (mount(rootfs, rootfs, "bind", MS_BIND, NULL)) {
		fprintf(stderr, "%s\n", rootfs);
		perror("can't bind mount");
		return 1;
	}

	if (syscall(SYS_pivot_root, rootfs, old)) {
		fprintf(stderr, "%s\n", rootfs);
		fprintf(stderr, "%s\n", old);
		perror("can't do pivot_root");
		return 1;
	}

	if (chdir("/")) {
		perror("can't chdir");
		return 1;
	}

	if (chroot("/")) {
		perror("can't chroot");
		return 1;
	}

	// TODO: should we do it here???

	if (mount("sysfs", "/sys", "sysfs", 0, NULL)) {
		perror("sys");
		return 1;
	}

	if (mount("proc", "/proc", "proc", 0, NULL)) {
		perror("proc");
		return 1;
	}

	if (umount2("/old", MNT_DETACH)) {
		perror("can't umount old");
	}
/*
	if (rmdir("/old")) {
		perror("can't rmdir /old");
	}
*/
	return 0;
}

#define HOSTNAME "container"

int daemonize()
{
	close(0);
	close(1);
	close(2);
	
	int tmpfd;
	if ((tmpfd = open("/dev/zero", 'r')) < 0) {
		perror("can't open /dev/zero");
		return 1;
	}

	if (dup2(tmpfd, 0) != 0) {
		perror("can't steer stdin to /dev/zero");
		return 1;
	}

	if ((tmpfd = open("/dev/null", 'r')) < 0) {
		perror("can't open /dev/zero");
		return 1;
	}

	if (dup2(tmpfd, 1) != 1) {
		perror("can't steer stdout to /dev/null");
		return 1;
	}

	if ((tmpfd = open("/dev/null", 'r')) < 0) {
		perror("can't open /dev/zero");
		return 1;
	}

	if (dup2(tmpfd, 2) != 2) {
		perror("can't steer stder to /dev/null");
		return 1;
	}

	return 0;
}
