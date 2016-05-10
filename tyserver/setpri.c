#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#define _STRUCT_TIMESPEC
#include <asm/unistd.h>
#include <linux/sched.h>

_syscall3(int, sched_setscheduler, \
		pid_t, pid, int, policy, struct sched_param, *param);

extern int sched_setscheduler(pid_t pid, int policy, struct sched_param *param);

static char *pri[] = { "ts", "fifo", "rr" };

void help() {
	printf("setpri [ts|rr|fifo] <PRI> <PID>\n");
	exit(0);
}

int main(int argc, char ** argv) {
	pid_t pid;
	int policy, x;
	struct sched_param param;
	if (argc<4) help();
	
	for (x=0;x<sizeof(pri)/sizeof(char *);x++)
		if (!strcasecmp(argv[1],pri[x]))
			policy=x;
	
	param.sched_priority = atoi(argv[2]);
	pid = atoi(argv[3]);

	if(sched_setscheduler(pid,policy,&param))
		perror("setpri");

        return 0;
}

