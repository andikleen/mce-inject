/* Inject machine checks into kernel for testing */
#define _GNU_SOURCE 1
#include <stdio.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "mce.h"
#include "inject.h"
#include "parser.h"
#include "util.h"

#define MAX_CPU_NUM	1024

static int cpu_num;
/* map from cpu index to cpu id */
static int cpu_map[MAX_CPU_NUM];
static struct mce **cpu_mce;

void init_cpu_info(void)
{
	FILE *f = fopen("/proc/cpuinfo", "r");
	char *line = NULL;
	size_t linesz = 0;

	if (!f)
		err("opening of /proc/cpuinfo");

	while (getdelim(&line, &linesz, '\n', f) > 0) {
		unsigned cpu;
		if (sscanf(line, "processor : %u\n", &cpu) == 1)
			cpu_map[cpu_num++] = cpu;
	}
	free(line);
	fclose(f);

	if (!cpu_num)
		err("geting cpu ids from /proc/cpuinfo");
}

void init_inject(void)
{
	cpu_mce = calloc(cpu_num, sizeof(struct mce *));
}

static inline int cpu_id_to_index(int id)
{
	int i;

	for (i = 0; i < cpu_num; i++)
		if (cpu_map[i] == id)
			return i;
	err("invalid cpu id");
	return -1;
}

static void write_mce(int fd, struct mce *m)
{
	int n = write(fd, m, sizeof(struct mce));
	if (n <= 0) 
		err("Injecting mce on /dev/mcelog");
	if (n < sizeof(struct mce)) { 
		fprintf(stderr, "Short mce write %d: kernel does not match?\n",
			n);
	}
}

struct thread { 	
	struct thread *next;
	pthread_t thr;
	struct mce *m;
	struct mce otherm;
	int fd;
};

volatile int blocked;

static void *injector(void *data)
{
	struct thread *t = (struct thread *)data;
	
	while (blocked)
		barrier();

	write_mce(t->fd, t->m);
	return NULL;
}

/* Simulate machine check broadcast.  */
void broadcast_mce(int fd, struct mce *m)
{
	FILE *f = fopen("/proc/cpuinfo", "r");
	char *line = NULL;
	size_t linesz = 0;
	struct mce otherm;
	struct thread *tlist = NULL;

	memset(&otherm, 0, sizeof(struct mce));
	// make sure to trigger exception on the secondaries
	otherm.mcgstatus = m->mcgstatus & MCG_STATUS_MCIP;
	otherm.status = m->status & MCI_STATUS_UC;

	if (!f) 
		err("opening of /proc/cpuinfo");

	blocked = 1;
	barrier();

	while (getdelim(&line, &linesz, '\n', f) > 0) { 
		unsigned cpu;
		if (sscanf(line, "processor : %u\n", &cpu) == 1) { 
			struct thread *t;
			pthread_attr_t attr;
			cpu_set_t aset;

			NEW(t);
			t->next = tlist;
			tlist = t;
			t->m = cpu == m->cpu ? m : &t->otherm;
			t->fd = fd;
			t->otherm = otherm;
			t->otherm.cpu = cpu;

			pthread_attr_init(&attr);
			CPU_ZERO(&aset);
			CPU_SET(cpu, &aset);
			if (pthread_attr_setaffinity_np(&attr, CPU_SETSIZE, &aset) < 0)
				err("pthread_attr_setaffinity");
			if (pthread_create(&t->thr, &attr, injector, t) < 0)
				err("pthread_create");
		}
	}
	free(line);
	fclose(f);

	/* could wait here for the threads to start up, but the kernel timeout should
	   be long enough to catch slow ones */

	barrier();
	blocked = 0;

	while (tlist) { 
		struct thread *next = tlist->next;
		pthread_join(tlist->thr, NULL);
		free(tlist);
		tlist = next;
	}
}

void inject_mce(struct mce *m)
{
	int inject_fd;

	inject_fd = open("/dev/mcelog", O_RDWR);
	if (inject_fd < 0) 
		err("opening of /dev/mcelog");

	if ((m->status & MCI_STATUS_UC) && !(mce_flags & MCE_NOBROADCAST)) { 
		/* broadcast */
		broadcast_mce(inject_fd, m);
	} else { 
		write_mce(inject_fd, m);
	}
	close(inject_fd);
}

void dump_mce(struct mce *m)
{
	printf("CPU %d\n", m->cpu);
	printf("BANK %d\n", m->bank);
	printf("TSC 0x%Lx\n", m->tsc);
	printf("RIP 0x%02x:0x%Lx\n", m->cs, m->ip);
	printf("MISC 0x%Lx\n", m->misc);
	printf("ADDR 0x%Lx\n", m->addr);
	printf("STATUS 0x%Lx\n", m->status);
	printf("MCGSTATUS 0x%Lx\n\n", m->mcgstatus);
}

void submit_mce(struct mce *m)
{
	if (do_dump)
		dump_mce(m);
	else
		inject_mce(m);
}

void init_mce(struct mce *m)
{
	memset(m, 0, sizeof(struct mce));
}

