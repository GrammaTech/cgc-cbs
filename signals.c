/*
* Setup signals
*
* Copyright (C) 2014 - Brian Caswell <bmc@lungetech.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <string.h>

#include "utils.h"
#include "signals.h"

void sigchld(const int sig);
void sigterm(const int sig) __attribute__((noreturn));

extern unsigned long num_children;

unsigned long long sts_cpu_clock;
unsigned long long sts_task_clock;
long sts_maxrss;
long sts_minflt;
struct timeval sts_utime;
unsigned long sts_nkids;
int exit_val;

void read_counters(pid_t pid);

struct cntr_desc {
	__u32 type;
	__u32 config;
	const char *name;
	unsigned long long *val;
} cntr_desc[] = {
	{ PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_CLOCK,
	  "sw-cpu-clock", &sts_cpu_clock },
	{ PERF_TYPE_SOFTWARE, PERF_COUNT_SW_TASK_CLOCK,
	  "sw-task-clock", &sts_task_clock },
};
#define NCNTRS_PER_PROC (sizeof(cntr_desc) / sizeof(cntr_desc[0]))

struct cntr {
	pid_t pid;
	const struct cntr_desc *desc;
	int fd;
} *cntrs = NULL;
int ncntrs = 0;

void sigchld(const int sig) {
    int status;
    pid_t pid;
    (void) sig;
    struct rusage ru;

    while ((pid = wait4(-1, &status, WNOHANG, &ru)) > 0) {
        if (!exit_val && WIFEXITED(status))
            exit_val = WEXITSTATUS(status);
        if (exit_val >= 0 && WIFSIGNALED(status) && WTERMSIG(status) != SIGUSR1)
            exit_val = -WTERMSIG(status);

        /* save off the stats */
        sts_minflt += ru.ru_minflt;
        sts_maxrss += ru.ru_maxrss;
        sts_utime.tv_sec += ru.ru_utime.tv_sec;
        sts_utime.tv_usec += ru.ru_utime.tv_usec;
        while (sts_utime.tv_usec >= 1000000) {
            sts_utime.tv_sec += 1;
            sts_utime.tv_usec -= 1000000;
        }
        sts_nkids++;
	read_counters(pid);

        if (num_children)
            num_children--;
    }
}

void sigterm(const int sig) {
    (void) sig;
    _exit(0);
}

void setup_signals(void) {
    sigset_t blocked_set;
    struct sigaction setup_action;

    sigemptyset(&blocked_set);
    sigaddset(&blocked_set, SIGCHLD);

    setup_action.sa_handler = sigchld;
    setup_action.sa_mask = blocked_set;
    setup_action.sa_flags = 0;

    VERIFY(sigaction, SIGCHLD, &setup_action, NULL);

    sigprocmask(SIG_BLOCK, &blocked_set, NULL);

    if (signal(SIGUSR1, SIG_IGN) == SIG_ERR)
        err(-1, "signal(SIGINFO) failed");
}

void unsetup_signals(void) {
    if (signal(SIGCHLD, SIG_DFL) == SIG_ERR)
        err(-1, "signal(SIGCHLD) failed");

    if (signal(SIGUSR1, SIG_DFL) == SIG_ERR)
        err(-1, "signal(SIGINFO) failed");
}

void handle_blocked_children(void) {
    sigset_t blocked_set;
    sigemptyset(&blocked_set);
    sigaddset(&blocked_set, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &blocked_set, NULL);
    /* if we had any SIGCHLD waiting, we should hit it... now */
    sigprocmask(SIG_BLOCK, &blocked_set, NULL);
}

void wait_for_signal(void) {
    sigset_t empty_set;
    sigemptyset(&empty_set);

    if (exit_val < 0)
        kill(-getpid(), SIGUSR1);

    sigsuspend(&empty_set);
}

void show_perf_stats(void) {
    int i;

    printf("total children: %lu\n", sts_nkids);
    printf("total maxrss %ld\n", sts_maxrss);
    printf("total minflt %ld\n", sts_minflt);
    printf("total utime %lu.%06lu\n",
            (unsigned long)sts_utime.tv_sec,
            (unsigned long)sts_utime.tv_usec);

    for (i = 0; i < NCNTRS_PER_PROC; i++)
        printf("total %s %llu\n", cntr_desc[i].name, *cntr_desc[i].val);
    fflush(stdout);
}

static int perf_event_open(struct perf_event_attr *event, pid_t pid, int cpu,
        int groupfd, unsigned long flags) {
    return syscall(__NR_perf_event_open, event, pid, cpu, groupfd, flags);
}

int make_counter(struct cntr *cntr, const struct cntr_desc *desc, pid_t pid,
        int gfd) {
    struct perf_event_attr attr;

    memset(&attr, 0, sizeof(attr));
    attr.type = desc->type;
    attr.size = sizeof(attr);
    attr.config = desc->config;
    attr.disabled = 1;
    attr.exclude_idle = 1;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.enable_on_exec = 1;

    if ((cntr->fd = perf_event_open(&attr, pid, -1, gfd, 0)) == -1)
        err(-1, "perf_event_open");

    cntr->desc = desc;
    cntr->pid = pid;
    return (cntr->fd);
}

void zero_perf_stats(int nprocs) {
    int i;

    sts_utime.tv_sec = 0;
    sts_utime.tv_usec = 0;
    sts_nkids = 0;
    sts_maxrss = 0;
    sts_minflt = 0;

    ncntrs = nprocs * NCNTRS_PER_PROC;
    cntrs = (struct cntr *)calloc(ncntrs, sizeof(*cntrs));

    for (i = 0; i < NCNTRS_PER_PROC; i++)
        *cntr_desc[i].val = 0ULL;
}

void setup_counters(int procno, pid_t pid) {
    int grp = -1, i;
    struct cntr *cntr = &cntrs[procno * NCNTRS_PER_PROC];

    for (i = 0; i < NCNTRS_PER_PROC; i++)
        grp = make_counter(cntr + i, &cntr_desc[i], pid, grp);
}

void read_counters(pid_t pid) {
    int i;

    for (i = 0; i < ncntrs; i++) {
        unsigned long long ct;

        if (cntrs[i].pid != pid)
            continue;
        if (read(cntrs[i].fd, &ct, sizeof(ct)) != sizeof(ct))
            continue;
        *cntrs[i].desc->val += ct;
    }
}
