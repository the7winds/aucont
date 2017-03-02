#define _GNU_SOURCE

#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>


int enterns(int pid, char* ns, int ns_flag)
{
	char nspath[100];
	sprintf(nspath, "/proc/%d/ns/%s", pid, ns);

	int fd = open(nspath, 0);
	
	if (fd < 0) {
		perror("can't open ns");
		return 1;
	}

	if (setns(fd, ns_flag)) {
		perror("can't set ns");
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

	int pid = fork();

	if (pid < 0) {
		perror("can't fork");
		return 1;
	}

	if (pid) {
		siginfo_t infop;
		waitid(P_PID, pid, &infop, WEXITED);
	} else {
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