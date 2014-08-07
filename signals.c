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

#include "signals.h"

extern unsigned long num_children;

void sigchld(const int sig);
void sigterm(const int sig) __attribute__((noreturn));

void sigchld(const int sig) {
    int status;
    pid_t pid;
    (void) sig;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
#ifdef DEBUG
        printf("terminated child (pid: %d) - %d\n", pid, sig);
#endif

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

    if (signal(SIGTERM, sigterm) == SIG_ERR)
        err(-1, "signal(SIGTERM) failed");

    if (signal(SIGPIPE, sigterm) == SIG_ERR)
        err(-1, "signal(SIGPIPE) failed");
}

void unsetup_signals(void) {
    if (signal(SIGCHLD, SIG_DFL) == SIG_ERR)
        err(-1, "signal(SIGCHLD) failed");

    if (signal(SIGTERM, SIG_DFL) == SIG_ERR)
        err(-1, "signal(SIGTERM) failed");

    if (signal(SIGPIPE, SIG_DFL) == SIG_ERR)
        err(-1, "signal(SIGPIPE) failed");
}

void wait_for_signal(void) {
    sigset_t ss;
    sigemptyset(&ss);
    sigsuspend(&ss);
}
