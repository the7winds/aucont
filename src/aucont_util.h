#ifndef __AUCONT_UTIL_H__
#define __AUCONT_UTIL_H__

#define CMD_LEN 100
#define PATH_LEN 1024


int set_work_directory();

int enterns(int pid, char* ns, int ns_flag);

int wait_parent(int pipe_in);


#endif /* __AUCONT_UTIL_H__ */
