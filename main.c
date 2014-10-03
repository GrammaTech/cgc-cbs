/*
* Launch a Challenge Binary
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

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "privileges.h"
#include "resources.h"
#include "signals.h"
#include "sockets.h"
#include "timeout.h"
#include "utils.h"

#ifdef RANDOM_UID
#define OPTARG_STR "b:d:hl:m:op:t:"
#else
#define OPTARG_STR "b:g:hl:m:op:t:u:"
#endif

extern volatile unsigned long num_children;
extern int exit_val;

volatile unsigned long num_children;
int insecure_flag;

void usage(const char *error) __attribute__((noreturn));
void start_program(const unsigned timeout, const char *program);
void handle(const int connection, const uid_t uid, const gid_t gid, const unsigned timeout, const int program_count, char **programs);


void usage(const char *error) {
    if (error != NULL)
        printf("ERROR: %s\n", error);

#ifdef RANDOM_UID
    printf("usage: cb-server [-h] [-t timeout] [-b backlog] [-l limit_children] [-m max_connections] [--insecure] -p <port> -d <directory> binary [... binary]\n");
#else
    printf("usage: cb-server [-h] [-t timeout] [-b backlog] [-l limit_children] [-m max_connections] [--insecure] -p <port> -g <gid> -u <uid> binary [... binary]\n");
#endif
    _exit(-1);
}

void start_program(const unsigned timeout, const char *program) {
    char *argv[] = {NULL};
    char *envp[] = {NULL};
    set_timeout(timeout);
    unsetup_signals();
#ifdef DEBUG
    fprintf(stderr, "pid=%d is_executable=%d program=%s\n", getpid(), is_executable(program), program);
#endif
    VERIFY(execve, program, argv, envp);
}

void handle(const int connection, const uid_t uid, const gid_t gid, const unsigned timeout, const int program_count, char **programs) {
    int i;

    VERIFY(setsid);
    setup_connection(connection);
    setup_sockpairs(program_count, STDERR_FILENO + 1);

    if (insecure_flag == 0) {
        drop_privileges(uid, gid);
        setup_sandbox();
    }

    num_children = 0;
    zero_perf_stats();

    for (i = 0; i < program_count; i++) {
        switch (fork()) {
	case 0:
	    start_program(timeout, programs[i]);
	    break;

	case -1:
	    err(-1, "unable to fork");

	default:
	    break;
	}

	num_children++;
    }
    while (num_children > 0)
	wait_for_signal();
    show_perf_stats();
    if (exit_val < 0) {
	unsetup_signals();
	no_core();
	raise(-exit_val);
	pause();
    }
    _exit(exit_val);
}


int main(int argc, char **argv) {
    int backlog = 20;
    int connection;
    int opt;
    int option_index = 0;
    int server;
    gid_t gid = 0;
    uid_t uid = 0;
    unsigned int limit = 40;
    unsigned int max_connections = 0;
    unsigned int connections = 0;
    unsigned int timeout = 0;
    unsigned short port = 0;

#ifdef RANDOM_UID
    char *directory = NULL;
#endif /* RANDOM_UID */
    num_children = 0;
    insecure_flag = 0;

    struct option long_options[] = {
        {"insecure", no_argument, &insecure_flag, 1},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, OPTARG_STR, long_options, &option_index)) != -1) {
        switch (opt) {
            case 0:
                /* ignore long opts here */
                break;

            case 'b':
                backlog = (int) str_to_ulong(optarg);
                break;
#ifdef RANDOM_UID

            case 'd':
                directory = strdup(optarg);
                break;
#else

            case 'u':
                uid = str_to_ulong(optarg);
                break;

            case 'g':
                gid = str_to_ulong(optarg);
                break;
#endif

            case 'l':
                limit = (unsigned int) str_to_ulong(optarg);
                break;
            
            case 'm':
                max_connections = (unsigned int) str_to_ulong(optarg);
                break;

            case 'p':
                port = str_to_ushort(optarg);
                break;

            case 't':
                timeout = (unsigned int) str_to_ulong(optarg);
                break;

            case 'h':
                usage("HELP");

            default:
                usage("unknown option");
        }
    }

    if (argc == 1)
        usage(NULL);

    if (optind == argc)
        usage("no challenge binaries specified");

    argc -= optind;
    argv += optind;

    if (port == 0)
        usage("port not set");

#ifdef RANDOM_UID

    if (directory == NULL)
        usage("invalid directory");

#else

    if (uid == 0)
        usage("invalid uid (0)");

    if (gid == 0)
        usage("invalid gid (0)");

#endif

    if (limit == 0)
        usage("invalid limit");

    setup_signals();
    server = socket_bind(port, backlog);

    if (server == -1)
        err(-1, "unable to bind");

    for (;;) {
        while (num_children >= limit)
            wait_for_signal();

        connection = socket_accept(server);

        if (connection == -1)
            continue;

        connections++;
        num_children++;

        switch (fork()) {
            case 0:
                close(server);
#ifdef RANDOM_UID
                uid = get_unused_uid();
                gid = get_unused_gid();
                setup_chroot(directory);
#endif
                handle(connection, uid, gid, timeout, argc, argv);
                break;

            case -1:
                err(-1, "unable to fork");

            default:
                break;
        }

        close(connection);
        if (max_connections > 0 && connections >= max_connections)
            break;
    }
    while (num_children > 0) {
        wait_for_signal();
    }
    if (exit_val < 0) {
	unsetup_signals();
	no_core();
	raise(-exit_val);
	pause();
    }
    return (exit_val);
}
