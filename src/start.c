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

int host_side_init(int pid, int pipe_out);

int container_side_init(void* args);

#define STACK_SIZE (1 << 20)


struct cont_args {
	int pipe_in;
	char* rootfs;
	char* old;
	char** cmd;
};


int main(int argc, char** argv)
{
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
		.rootfs = argv[1],
		.old = argv[2],
		.cmd = argv + 3
	};

	int pid = clone(container_side_init, stack_top, flags, &cont_args);

	if (pid < 0) { 
		perror("can't clone");
		return 1;
	}

	if (pid) {
		close(pipefd[0]);
		return host_side_init(pid, pipefd[1]);
	}
}


int host_side_init(int pid, int pipe_out)
{
	// TODO: configure cgroup
	// TODO: configure netns

	printf("%d\n", pid);

	siginfo_t infop;
	waitid(P_PID, pid, &infop, WEXITED);

	return 0;
}


int set_uid_mapping()
{
	int fd = open("/proc/self/uid_map", O_WRONLY);
	if (fd < 0) {
		perror("can't open uid_map");
		return 1;
	}

	char *mapping = "0 1000 1";
	int mapping_len = strlen(mapping);

	if (mapping_len != write(fd, mapping, mapping_len)) {
		close(fd);
		perror("can't write uid_map");
		return 1;
	}

	close(fd);
	return 0;
}

// TODO: set_gid_mapping()

/**
 * CONTRACT: _rootfs_ and _old_ start from the same drirectory
 */

int umount_old(char* rootfs, char* old)
{
	int root_len = strlen(rootfs);
	return umount2(old + root_len, MNT_DETACH);
}


int mount_rootfs(char* rootfs, char* old)
{
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


int wait_host(int pipe_in)
{
	return 0;
}


int container_side_init(void* args)
{
	struct cont_args* cont_args = args;

	if (wait_host(cont_args->pipe_in)) {
		perror("problems with host");
		return 1;
	}

	if (set_uid_mapping()) {
		perror("can't set uid mapping");
		return 1;
	}

	if (mount_rootfs(cont_args->rootfs, cont_args->old)) {
		perror("can't mount rootfs");
		return 1;
	}

	// it seems that at this line the process loses some capabilities
	execv(cont_args->cmd[0], cont_args->cmd);

	perror("can't do execv");
	return 1;
}

