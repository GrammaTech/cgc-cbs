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

#include "signals.h"

void sigchld(const int sig);
void sigterm(const int sig) __attribute__((noreturn));

extern unsigned long num_children;

long sts_maxrss;
struct timeval sts_utime;
unsigned long sts_nkids;
int exit_val;

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
        sts_maxrss += ru.ru_maxrss;
        sts_utime.tv_sec += ru.ru_utime.tv_sec;
        sts_utime.tv_usec += ru.ru_utime.tv_usec;
        while (sts_utime.tv_usec >= 1000000) {
            sts_utime.tv_sec += 1;
            sts_utime.tv_usec -= 1000000;
        }
        sts_nkids++;

        if (num_children)
            num_children--;
    }
}

void sigterm(const int sig) {
    (void) sig;
    _exit(0);
}

void setup_signals(void) {
    if (signal(SIGCHLD, sigchld) == SIG_ERR)
        err(-1, "signal(SIGCHLD) failed");

    if (signal(SIGUSR1, SIG_IGN) == SIG_ERR)
        err(-1, "signal(SIGINFO) failed");
}

void unsetup_signals(void) {
    if (signal(SIGCHLD, SIG_DFL) == SIG_ERR)
        err(-1, "signal(SIGCHLD) failed");

    if (signal(SIGUSR1, SIG_DFL) == SIG_ERR)
        err(-1, "signal(SIGINFO) failed");
}

void wait_for_signal(void) {
    sigset_t ss;
    if (exit_val < 0)
        kill(-getpid(), SIGUSR1);
    sigemptyset(&ss);
    sigsuspend(&ss);
}

void show_perf_stats(void) {
    printf("total children: %lu\n", sts_nkids);
    printf("total maxrss %ld\n", sts_maxrss);
    printf("total utime %lu.%06lu\n",
            (unsigned long)sts_utime.tv_sec,
            (unsigned long)sts_utime.tv_usec);
}

void zero_perf_stats(void) {
    sts_utime.tv_sec = 0;
    sts_utime.tv_usec = 0;
    sts_nkids = 0;
    sts_maxrss = 0;
}
