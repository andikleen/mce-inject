/* Copyright (c) 2008 by Intel Corp.
   Inject machine checks into kernel for testing.

   mce-inject is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; version
   2.

   mce-inject is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should find a copy of v2 of the GNU General Public License somewhere
   on your Linux system; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Authors:
        Andi Kleen
	Ying Huang
*/
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

static int cpu_num;
/* map from cpu index to cpu id */
static int *cpu_map;
static struct mce **cpu_mce;

void init_cpu_info(void)
{
	FILE *f = fopen("/proc/cpuinfo", "r");
	char *line = NULL;
	size_t linesz = 0;
	int max_cpu = sysconf(_SC_NPROCESSORS_CONF);
	if (!f)
		err("opening of /proc/cpuinfo");

	cpu_map = xcalloc(sizeof(int), max_cpu);

	while (getdelim(&line, &linesz, '\n', f) > 0) {
		unsigned cpu;
		if (sscanf(line, "processor : %u\n", &cpu) == 1 && 
			cpu_num < max_cpu)
			cpu_map[cpu_num++] = cpu;
	}
	free(line);
	fclose(f);

	if (!cpu_num)
		err("getting cpu ids from /proc/cpuinfo");
}

void init_inject(void)
{
	cpu_mce = xcalloc(cpu_num, sizeof(struct mce *));
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
		fprintf(stderr, "mce-inject: Short mce write %d: kernel does not match?\n",
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
void do_inject_mce(int fd, struct mce *m)
{
	int i, has_random = 0;
	struct mce otherm;
	struct thread *tlist = NULL;

	memset(&otherm, 0, sizeof(struct mce));
	// make sure to trigger exception on the secondaries
	otherm.mcgstatus = m->mcgstatus & MCG_STATUS_MCIP;
	if (m->status & MCI_STATUS_UC)
		otherm.mcgstatus |= MCG_STATUS_RIPV;
	otherm.status = m->status & MCI_STATUS_UC;

	blocked = 1;
	barrier();

	for (i = 0; i < cpu_num; i++) {
		unsigned cpu = cpu_map[i];
		struct thread *t;
		pthread_attr_t attr;
		cpu_set_t aset;

		NEW(t);
		if (cpu == m->extcpu) {
			t->m = m;
			if (MCJ_CTX(m->inject_flags) == MCJ_CTX_RANDOM)
				MCJ_CTX_SET(m->inject_flags, MCJ_CTX_PROCESS);
		} else if (cpu_mce[i])
			t->m = cpu_mce[i];
		else if (mce_flags & MCE_NOBROADCAST) {
			free(t);
			continue;
		} else {
			t->m = &t->otherm;
			t->otherm = otherm;
			t->otherm.cpu = t->otherm.extcpu = cpu;
		}

		if (no_random && MCJ_CTX(t->m->inject_flags) == MCJ_CTX_RANDOM)
			MCJ_CTX_SET(t->m->inject_flags, MCJ_CTX_PROCESS);
		else if (MCJ_CTX(t->m->inject_flags) == MCJ_CTX_RANDOM) {
			write_mce(fd, t->m);
			has_random = 1;
			free(t);
			continue;
		}

		t->fd = fd;
		t->next = tlist;
		tlist = t;

		pthread_attr_init(&attr);
		CPU_ZERO(&aset);
		CPU_SET(cpu, &aset);
		if (pthread_attr_setaffinity_np(&attr, sizeof(aset), &aset))
			err("pthread_attr_setaffinity");
		if (pthread_create(&t->thr, &attr, injector, t))
			err("pthread_create");
	}

	if (has_random)
		m->inject_flags |= MCJ_NMI_BROADCAST;

	/* could wait here for the threads to start up, but the kernel
	   timeout should be long enough to catch slow ones */

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

	if (mce_flags & MCE_HOLD) {
		int cpu_index = cpu_id_to_index(m->extcpu);
		struct mce *nm;
		NEW(nm);
		*nm = *m;
		cpu_mce[cpu_index] = nm;
		return;
	}

	inject_fd = open("/dev/mcelog", O_RDWR);
	if (inject_fd < 0)
		err("opening of /dev/mcelog");
	if (!(m->status & MCI_STATUS_UC))
		mce_flags |= MCE_NOBROADCAST;
	do_inject_mce(inject_fd, m);
	close(inject_fd);
}

void dump_mce(struct mce *m)
{
	printf("CPU %d\n", m->extcpu);
	printf("BANK %d\n", m->bank);
	printf("TSC 0x%Lx\n", m->tsc);
	printf("TIME %Lu\n", m->time);
	printf("RIP 0x%02x:0x%Lx\n", m->cs, m->ip);
	printf("MISC 0x%Lx\n", m->misc);
	printf("ADDR 0x%Lx\n", m->addr);
	printf("STATUS 0x%Lx\n", m->status);
	printf("MCGSTATUS 0x%Lx\n", m->mcgstatus);
	printf("PROCESSOR %u:0x%x\n\n", m->cpuvendor, m->cpuid);
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
