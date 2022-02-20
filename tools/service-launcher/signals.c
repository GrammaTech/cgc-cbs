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
#include <sys/resource.h>
#include <unistd.h>
#include <time.h>
#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <string.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <stdlib.h>
#include "utils.h"
#include "signals.h"

void sigchld(const int sig);
void sigterm(const int sig) __attribute__((noreturn));

extern unsigned long num_children;
extern int monitor_process;

unsigned long long sts_cpu_clock;
unsigned long long sts_task_clock;
long sts_maxrss;
long sts_minflt;
struct timeval sts_utime;
unsigned long sts_nkids;
int exit_val = 0;

void read_counters(pid_t pid);
void print_registers(pid_t pid);

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
    struct rusage ru;

    /* unused argument */
    (void) sig;
    int signum;

    while ((pid = wait4(-1, &status, WNOHANG, &ru)) > 0) {
        signum = 0;

        if (WIFEXITED(status)) {
            printf("CB exited (pid: %d, exit code: %d)\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            signum = WTERMSIG(status);
            // printf("SIGNALED %d\n", signum);
        } else if (WIFSTOPPED(status)) {
            signum = WSTOPSIG(status);
            // printf("STOPPED %d\n", signum);
            if (signum == SIGPIPE) {
                // printf("continuing on SIGPIPE\n");
                ptrace(PT_CONTINUE, pid, (caddr_t)1, signum);
                continue;
            }
        }

        switch (signum) {
            case 0:
            case SIGUSR1:
                /* exited normally */
                break;

            case SIGALRM:
                if (monitor_process == 1)
                    printf("CB timed out (pid: %d)\n", pid);

                if (exit_val == 0) {
                    exit_val = -signum;
                }
                break;

            case SIGSEGV:
            case SIGILL:
            case SIGBUS:
                print_registers(pid);
           
            default:
                if (monitor_process == 1)
                    printf("CB generated signal (pid: %d, signal: %d)\n", pid, signum);

                if (exit_val == 0) {
                    exit_val = -signum;
                }

                break;
        }

        ptrace(PT_DETACH, pid, 0, 0);

        if (signum == 0)
            signum = SIGUSR1;

        kill(pid, signum);
        
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

    if (exit_val < 0) {
        kill(-getpid(), SIGUSR1);
    }
        
    sigsuspend(&empty_set);
}

void setup_ptrace(pid_t pid) {
    int status;

    if (ptrace(PT_ATTACH, pid, 0, 0) != 0) {
        err(-1, "ptrace attach failed");
    }

    if (waitpid(pid, &status, 0) == pid) {
        if (WIFSTOPPED(status)) {
            if (WSTOPSIG(status) == SIGSTOP) {
                if (ptrace(PT_CONTINUE, pid, (caddr_t)1, 0) != 0) {
                    err(-1, "ptrace continue failed");
                }
                return;
            }
        }

        err(-1, "unexpected waitpid status: %d\n", status);
    }
}

void continue_ptrace(pid_t pid) {
    int status;

    if (waitpid(pid, &status, 0) == pid) {
        if (WIFSTOPPED(status)) {
            if (WSTOPSIG(status) == SIGTRAP) {
                if (ptrace(PT_CONTINUE, pid, (caddr_t)1, 0) != 0) {
                    err(-1, "ptrace continue failed");
                }
                return;
            }
        }
        err(-1, "unexpected waitpid status (continue): %d\n", status);
    }
}

void print_registers(pid_t pid) {
    int res;

    struct user_regs_struct registers;
    res = ptrace(PT_GETREGS, pid, 0, (caddr_t)&registers);
    if (res == 0) {
        printf("register states - ");
#ifdef __i386__
        printf("eax: %08lx ", registers.eax);
        printf("ecx: %08lx ", registers.ecx);
        printf("edx: %08lx ", registers.edx);
        printf("ebx: %08lx ", registers.ebx);
        printf("esp: %08lx ", registers.esp);
        printf("ebp: %08lx ", registers.ebp);
        printf("esi: %08lx ", registers.esi);
        printf("edi: %08lx ", registers.edi);
        printf("eip: %08lx\n", registers.eip);
#elif defined(__x86_64__)
        printf("rax: %08llx ", registers.rax);
        printf("rcx: %08llx ", registers.rcx);
        printf("rdx: %08llx ", registers.rdx);
        printf("rbx: %08llx ", registers.rbx);
        printf("rsp: %08llx ", registers.rsp);
        printf("rbp: %08llx ", registers.rbp);
        printf("rsi: %08llx ", registers.rsi);
        printf("rdi: %08llx ", registers.rdi);
        printf("rip: %08llx\n", registers.rip);
#else
#error "your turn..."
#endif
        fflush(stdout);
    }
}

void show_perf_stats(void) {
    int i;

    printf("total children: %lu\n", sts_nkids);
    printf("total maxrss %ld\n", sts_maxrss);
    printf("total minflt %ld\n", sts_minflt);
    printf("total utime %lu.%06lu\n", (unsigned long)sts_utime.tv_sec,
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
    int grp = -1, i, fd;
    struct cntr *cntr = &cntrs[procno * NCNTRS_PER_PROC];

    for (i = 0; i < NCNTRS_PER_PROC; i++) { 
        fd = make_counter(cntr + i, &cntr_desc[i], pid, grp); 
        if (grp == -1) 
            grp = fd; 
    }
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

/* Local variables: */
/* mode: c */
/* c-basic-offset: 4 */
/* End: */
