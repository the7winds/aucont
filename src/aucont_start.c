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

int container_side_init(void* args);

#define STACK_SIZE (1 << 20)

struct start_args {
	bool detatched;
	bool net;
	char* host_side_ip;
	int cpu_perc;
	char* image_path;
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
			args->host_side_ip = argv[i + 1];
			i += 2;
		} else {
			break;
		}
	}

	if (argv[i] != NULL && argv[i + 1] != NULL) {
		args->image_path = argv[i];
		args->cmd = argv + i + 1;
		return 0;
	}

	return 1;
}

#define USAGE "usage: ./aucont_start [-d --cpu CPU_PERC --net IP] IMAGE_PATH CMD [ARGS]\n"

int configure_environment(int pid, struct start_args* args);

int host_side_init(int pid, int pipe_out, struct start_args* args);

int main(int argc, char** argv)
{
	struct start_args args = {
		.detatched = false,
		.net = false,
		.host_side_ip = "null",
		.cpu_perc = 100
	};

	if (parse_args(argv, &args)) {
		fprintf(stderr, USAGE);
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
	char cmd[100];
	sprintf(cmd, "./config_env.py %d %s %s", pid, args->image_path, args->host_side_ip);
	if (system(cmd)) {
		perror("can't call system");
		return 1;
	}
	
	return 0;
}


#define MAX_IP_LEN 19

int configure_netns(int pid)
{
	int addrs_len = 2 * MAX_IP_LEN + 1;
	char addrs[addrs_len];

	FILE* netns_config = fopen("net_config", "r");

	fgets(addrs, addrs_len, netns_config);

	fclose(netns_config);

	char cmd[100];
	sprintf(cmd, "./aucont_net.sh %d %s", pid, addrs);

	if (system(cmd)) {
		perror("can't configure netns");
		return 1;
	}

	return 0;
}


int set_uid_mapping(int pid)
{
	char path[50];
	sprintf(path, "/proc/%d/uid_map", pid);

	int fd = open(path, O_WRONLY);
	if (fd < 0) {
		perror("can't open uid_map");
		return 1;
	}

	char *mapping = "0 0 1";
	int mapping_len = strlen(mapping);

	if (mapping_len != write(fd, mapping, mapping_len)) {
		close(fd);
		perror("can't write uid_map");
		return 1;
	}

	close(fd);
	return 0;
}


int set_gid_mapping(int pid)
{
	char path[50];
	sprintf(path, "/proc/%d/gid_map", pid);

	int fd = open(path, O_WRONLY);
	if (fd < 0) {
		perror("can't open gid_map");
		return 1;
	}

	char *mapping = "0 0 1";
	int mapping_len = strlen(mapping);

	if (mapping_len != write(fd, mapping, mapping_len)) {
		close(fd);
		perror("can't write gid_map");
		return 1;
	}

	close(fd);
	return 0;
}

int host_side_init(int pid, int pipe_out, struct start_args* args)
{
	// TODO: configure cgroup
	
	int msg = -1;
	if (args->net) {
		if (configure_netns(pid)) {
			perror("can't configure network");
			write(pipe_out, &msg, sizeof(msg));
			return 1;
		}
	}
	
	if (set_uid_mapping(pid)) {
		perror("can't configure uid mapping");
		write(pipe_out, &msg, sizeof(msg));
		return 1;
	}

	if (set_gid_mapping(pid)) {
		perror("can't configure gid mapping");
		write(pipe_out, &msg, sizeof(msg));
		return 1;
	}

	msg = pid;
	write(pipe_out, &msg, sizeof(msg));

	printf("%d\n", pid);

	siginfo_t infop;
	waitid(P_PID, pid, &infop, WEXITED);

	return 0;
}

/*
int set_uid_mapping()
{
	int fd = open("/proc/self/uid_map", O_WRONLY);
	if (fd < 0) {
		perror("can't open uid_map");
		return 1;
	}

	char mapping[10];
	sprintf(mapping, "%d 1000 1", geteuid());
	fprintf(stderr, "euid: %d\n", geteuid());
	int mapping_len = strlen(mapping);

	if (mapping_len != write(fd, mapping, mapping_len)) {
		close(fd);
		perror("can't write uid_map");
		return 1;
	}

	close(fd);
	return 0;
}

int set_gid_mapping()
{
	int fd;

	if ((fd = open("/proc/self/setgroups", O_WRONLY)) < 0) {
		perror("can't open setgroups");
		return 1;
	}

	char *deny = "deny";
	int deny_len = strlen(deny);

	if (deny_len != write(fd, deny, deny_len)) {
		perror("can't write 'deny'");
		return 1;
	}

	close(fd);


	fd = open("/proc/self/gid_map", O_WRONLY);
	if (fd < 0) {
		perror("can't open gid_map");
		return 1;
	}

	char *mapping = "0 1000 1";
	int mapping_len = strlen(mapping);

	if (mapping_len != write(fd, mapping, mapping_len)) {
		close(fd);
		perror("can't write gid_map");
		return 1;
	}

	close(fd);
	return 0;
}
*/


int umount_old(char* rootfs, char* old)
{
	int root_len = strlen(rootfs);
	// return umount2(old + root_len, 0); //MNT_DETACH);
	return umount2(old + root_len, MNT_DETACH);
}


int mount_rootfs(int pid)
{
	char rootfs[30];
	sprintf(rootfs, "%d/rootfs", pid);
	
	char old[30];
	sprintf(old, "%d/rootfs/old", pid);

	if (mount(rootfs, rootfs, "bind", MS_BIND | MS_REC, NULL)) {
		perror("can't mount");
		return 1;
	}

	if (syscall(SYS_pivot_root, rootfs, old)) {
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

	// this line removes ability to mount procfs
	if (umount_old(rootfs, old)) {
		perror("can't umount old");
	}

	return 0;
}

/**
 * on success returns PID, else -1
 */

int wait_host(int pipe_in)
{
	int msg;
	read(pipe_in, &msg, sizeof(msg));
	return msg;
}


int container_side_init(void* args)
{
	struct cont_args* cont_args = args;
	
	int pid;
	if ((pid = wait_host(cont_args->pipe_in)) < 0) {
		perror("problems with host");
		return 1;
	}

/*
	if (mount("proc", "/proc", "proc", 0, NULL)) {
		perror("can't mount proc");
		return 1;
	}

	if (set_uid_mapping()) {
		perror("can't set uid mapping");
		return 1;
	}

	if (set_gid_mapping()) {
		perror("can't set gid mapping");
		return 1;
	}
*/
	if (mount_rootfs(pid)) {
		perror("can't mount rootfs");
		return 1;
	}

	// it seems that at this line the process loses some capabilities
	execv(cont_args->cmd[0], cont_args->cmd);

	perror("can't do execv");
	return 1;
}

