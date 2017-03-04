#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "aucont_util.h"


int main(int argc, char** argv)
{
	if (set_work_directory()) {
		perror("can't set work directory");
		return 1;
	}

	int sig = SIGTERM;
	pid_t pid = (pid_t) atoi(argv[1]);

	if (argv[2] != NULL) {
		sig = atoi(argv[2]);
	}

	if (ptrace(PTRACE_SEIZE, pid, 0, PTRACE_O_TRACEEXIT)) {
		if (errno == ESRCH) {
			goto end;
		}

		perror("can't ptrace");
		return 1;
	}

	if (kill(pid, sig)) {
		perror("can't send signal");
		return 1;
	}

	int status;
	waitpid(pid, &status, 0);

	if ((status >> 8) == (SIGTRAP | (PTRACE_EVENT_EXIT << 8))) {
		if (ptrace(PTRACE_DETACH, pid, 0, 0)) {
			perror("can't detach");
			return 1;
		}
	} else {
		perror("impossible status");
		return 1;
	}

	char path[50];
	sprintf(path, "/proc/%d", pid);

	int pd;
	while ((pd = open(path, O_RDONLY)) > 0) {
		close(pd);
	}

	char cmd[CMD_LEN];
end:
	sprintf(cmd, "rm -rf ./%d", pid);
	return system(cmd);
}
