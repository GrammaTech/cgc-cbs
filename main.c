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

#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>

#include <stdlib.h>
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
#define OPTARG_STR "b:c:d:hl:m:M:op:t:s:S:w:"
#else
#define OPTARG_STR "b:c:g:hl:m:M:op:t:s:S:u:w:"
#endif

extern volatile unsigned long num_children;
extern int exit_val;

int monitor_process;

volatile unsigned long num_children;
int insecure_flag;
int negotiate_flag;
int debug_flag;

void usage(const char *error) __attribute__((noreturn));
void start_program(const unsigned timeout, char *program, char *wrapper, char *seed, char *skiprng, char *max_transmit, char *max_receive);
void handle(const int connection, const uid_t uid, const gid_t gid, const unsigned timeout, char *wrapper, char *seed, char *skiprng, char *max_transmit, char *max_receive, const int program_count, char **programs);

void usage(const char *error) {
    if (error != NULL)
        printf("ERROR: %s\n", error);

#ifdef RANDOM_UID
    printf("usage: cb-server [-h] [-b backlog] [-c core_size] [-s seed] [-S skiprng] [-t timeout] [-l limit_children] [-m max_connections] [-m max_send] [-w wrapper] [--debug] [--negotiate] [--insecure] -p <port> -d <directory> binary [... binary]\n");
#else
    printf("usage: cb-server [-h] [-b backlog] [-c core_size] [-s seed] [-S skiprng] [-t timeout] [-l limit_children] [-m max_connections] [-m max_send] [-w wrapper] [--debug] [--negotiate] [--insecure] -p <port> -g <gid> -u <uid> binary [... binary]\n");
#endif
    _exit(-1);
}

void start_program(const unsigned timeout, char *program, char *wrapper, char *seed, char *skiprng, char *max_transmit, char *max_receive) {
    char *envp[] = {NULL};
    char *argv[] = {program, program, seed, skiprng, max_transmit, max_receive, NULL};

    set_timeout(timeout);
    unsetup_signals();

#ifdef DEBUG
    dprintf(STDERR_FILENO, "pid=%d is_executable=%d program=%s\n", getpid(), is_executable(program), program);
    dprintf(STDERR_FILENO, "pid=%d is_executable=%d program=%s\n", getpid(), is_executable(wrapper), wrapper);
#endif

    if (wrapper != NULL) {
        VERIFY(execve, wrapper, argv, envp);
    } else {
        VERIFY(execve, program, argv, envp);
    }
}

void negotiate(const int connection, char **seed) {
    uint32_t record_count;
    uint32_t current_record_type;
    uint32_t current_record_size;
    uint32_t i;
    uint32_t done = 1;
    unsigned char *current_record;

    printf("negotation flag: %d\n", negotiate_flag);
    if (!negotiate_flag) {
        return;
    }

    record_count = read_uint32_t(connection);
    
    for (i = 0; i < record_count; i++) {
        current_record_type = read_uint32_t(connection);
        current_record_size = read_uint32_t(connection);
        current_record = read_buffer(connection, current_record_size);

        switch(current_record_type) {
            case 1:
                *seed = set_prng_seed(current_record, current_record_size);
                break;
            case 2:
                print_source_identifier(current_record, current_record_size);
                break;
            case 4:
                print_hash(current_record, current_record_size);
                break;
            default:
                err(-1, "unsupported record type %d", current_record_type);
                break;
        }
        free(current_record);
    }
    send_all(connection, (char *) &done, sizeof(done));
}

void handle(const int connection, const uid_t uid, const gid_t gid, const unsigned timeout, char *wrapper, char *seed, char *skiprng, char *max_transmit, char *max_receive, const int program_count, char **programs) {
    int i;
    int saved_sockets[2];
    pid_t pid;
    monitor_process = 1;

    print_filesizes(program_count, programs);
    drop_privileges(uid, gid);

    setup_connection(connection, program_count, saved_sockets);
    setup_sockpairs(program_count, STDERR_FILENO + 1);

    setup_sandbox();

    num_children = program_count;
    zero_perf_stats(program_count);

    for (i = 0; i < program_count; i++) {
        int pause_sockets_1[2];
        int pause_sockets_2[2];
        setup_pairwise_wait(pause_sockets_1);
        setup_pairwise_wait(pause_sockets_2);

        VERIFY_ASSN(pid, fork);

        if (pid == 0) {
            ready_pairwise(pause_sockets_1);
            wait_pairwise(pause_sockets_2);

            close_saved_sockets(saved_sockets);
            set_cb_resources(wrapper);
            start_program(timeout, programs[i], wrapper, seed, skiprng, max_transmit, max_receive);
            break;
        } else {
            wait_pairwise(pause_sockets_1);

            if (wrapper == NULL && debug_flag == 0)
                setup_ptrace(pid);

            setup_counters(i, pid);
            ready_pairwise(pause_sockets_2);

            if (wrapper == NULL && debug_flag == 0)
                continue_ptrace(pid);
        }
    }

    reset_base_sockets(saved_sockets);
        
    while (num_children > 0)
        wait_for_signal();

    show_perf_stats();
    if (exit_val < 0) {
        unsetup_signals();
        set_core_size(0);
        raise(-exit_val);
        pause();
    }
    _exit(exit_val);
}

int main(int argc, char **argv) {
    int backlog = 20;
    int connection;
    int core_size = -1;
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

    char *skiprng = NULL;
    char *seed = NULL;
    char *wrapper = NULL;
    char *max_receive = NULL;
    char *max_transmit = NULL;

#ifdef RANDOM_UID
    char *directory = NULL;
#endif /* RANDOM_UID */
    num_children = 0;
    insecure_flag = 0;
    negotiate_flag = 0;

    struct option long_options[] = {
        {"debug", no_argument, &debug_flag, 1},
        {"negotiate", no_argument, &negotiate_flag, 1},
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

            case 'c':
                core_size = (int) str_to_ulong(optarg);
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
            
            case 'M':
                VERIFY(asprintf, &max_receive, "max_receive=%d",  str_to_ulong(optarg));
                VERIFY(asprintf, &max_transmit, "max_transmit=%d",  str_to_ulong(optarg));
                break;

            case 'p':
                port = str_to_ushort(optarg);
                break;

            case 's':
                VERIFY(asprintf, &seed, "seed=%s", optarg);
                break;

            case 'S':
                VERIFY(asprintf, &skiprng, "skiprng=%s", optarg);
                break;

            case 't':
                timeout = (unsigned int) str_to_ulong(optarg);
                break;

            case 'w':
                wrapper = strdup(optarg);
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

    if (core_size != -1)
        set_core_size(core_size);

    if (insecure_flag == 0 && getuid() != 0)
        usage("unable to chroot.  either run as root or add --insecure");

    if (negotiate_flag != 0 && seed != NULL)
        usage("Seed will be negotiated with cb-replay");

    setup_signals();
    server = socket_bind(port, backlog);

    if (server == -1)
        err(-1, "unable to bind");

    for (;;) {
        handle_blocked_children();

        while (num_children >= limit)
            wait_for_signal();

        connection = socket_accept(server);

        if (connection == -1)
            continue;

        connections++;
        num_children++;

        exit_val = 0;
        switch (fork()) {
            case 0:
                close(server);

                negotiate(connection, &seed);

                if (!seed)
                    seed = get_prng_seed();
#ifdef RANDOM_UID
                uid = get_unused_uid();
                gid = get_unused_gid();
                setup_chroot(directory);
#endif
                handle(connection, uid, gid, timeout, wrapper, seed, skiprng, max_transmit, max_receive, argc, argv);
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

    handle_blocked_children();

    while (num_children > 0) {
        wait_for_signal();
    }

    if (exit_val != 0) {
        unsetup_signals();
        set_core_size(0);
        exit_val = -exit_val;
    }

    return exit_val;
}

/* Local variables: */
/* mode: c */
/* c-basic-offset: 4 */
/* End: */
