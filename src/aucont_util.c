#define _GNU_SOURCE

#include "aucont_util.h"

#include <unistd.h>
#include <libgen.h>
#include <stdio.h>
#include <fcntl.h>
#include <sched.h>


int set_work_directory()
{
	char base_val[PATH_LEN];

	if (readlink("/proc/self/exe", base_val, sizeof(base_val)) < 0) {
		perror("can't readlink");
		return 1;
	}	

	char *wd = dirname(base_val);

	if (chdir(wd)) {
		perror("can't chdir");
		return 1;
	}

	return 0;
}


int enterns(int pid, char* ns, int ns_flag)
{
	char nspath[PATH_LEN];
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


int wait_parent(int pipe_in)
{
	int msg;
	
	if (sizeof(msg) != read(pipe_in, &msg, sizeof(msg))) {
		perror("received invalid message");
		return -1;
	}

	return msg;
}
