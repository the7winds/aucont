#define _GNU_SOURCE

#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "aucont_util.h"

int join_to_cgroup(int ppid, int pid)
{
	char cgpath[PATH_LEN];
	sprintf(cgpath, "/sys/fs/cgroup/cpu/%d/tasks", ppid);

	int fd = open(cgpath, O_WRONLY | O_APPEND);
	
	if (fd < 0) {
		perror("can't open cpu tasks");
		return 1;
	}

	char spid[10];
	int spid_len = sprintf(spid, "%d", pid);

	if (spid_len != write(fd, spid, spid_len)) {
		perror("can't write pid");
		close(fd);
		return 1;
	}

	close(fd);

	return 0;
}


int main(int argc, char** argv)
{
	int ppid = atoi(argv[1]);
	char** cmd = argv + 2;

	if (enterns(ppid, "pid", CLONE_NEWPID)) {
		perror("can't enter in userns");
		return 1;
	}

	int pipefd[2];
	if (pipe2(pipefd, O_CLOEXEC)) {
		perror("can't get pipe");
		return 1;
	}

	int pid = fork();

	if (pid < 0) {
		perror("can't fork");
		return 1;
	}

	if (pid) {
		if (join_to_cgroup(ppid, pid)) {
			perror("can't join to cgroup");
			return 1;
		}

		if (sizeof(pid) != write(pipefd[1], &pid, sizeof(pid))) {
			perror("can't send ok");
			return 1;
		}

		siginfo_t infop;
		waitid(P_PID, pid, &infop, WEXITED);
	} else {
		if (wait_parent(pipefd[0]) < 0) {
			perror("can't read msg");
			return 1;
		}

		if (enterns(ppid, "ipc", CLONE_NEWIPC)) {
			perror("can't enter in ipcns");
			return 1;
		}

		if (enterns(ppid, "uts", CLONE_NEWUTS)) {
			perror("can't enter in utsns");
			return 1;
		}

		if (enterns(ppid, "net", CLONE_NEWNET)) {
			perror("can't enter in netns");
			return 1;
		}

		if (enterns(ppid, "user", CLONE_NEWUSER)) {
			perror("can't enter in userns");
			return 1;
		}

		if (enterns(ppid, "mnt", CLONE_NEWNS)) {
			perror("can't enter in mountns");
			return 1;
		}

		setuid(0);
		setgid(0);

		execv(cmd[0], cmd);
		perror("can't execv");

		return 1;
	}

	return 0;
}
