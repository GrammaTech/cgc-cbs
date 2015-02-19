/*
* Handle TCP setup
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

#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include "utils.h"
#include "sockets.h"

extern int debug_flag;

int socket_accept(const int server) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int s;
    s = accept(server, (struct sockaddr *) &addr, &addr_len);

    if (s == -1) return -1;

    printf("connection from: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    return s;
}

int socket_bind(const unsigned short port, const int backlog) {
    int s;
    struct sockaddr_in addr;
    int opt = 1;
    struct linger so_linger;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    s = socket(AF_INET, SOCK_STREAM, 0);

    so_linger.l_onoff = 1;
    so_linger.l_linger = 5;

    if (s == -1)
        err(-1, "unable to create socket");

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        err(-1, "unable to set SO_REUSEADDR");

    if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == -1)
        err(-1, "unable to set TCP_NODELAY");

    if (setsockopt(s, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) == -1)
        err(-1, "unable to set SO_LINGER");

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        err(-1, "unable to bind port: %d", port);

    if (listen(s, backlog) == -1)
        err(-1, "unable to listen");

    return s;
}

void close_saved_sockets(int *sockets) {
    close(sockets[0]);
    close(sockets[1]);
}

void reset_base_sockets(int *sockets) {
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    VERIFY(fcntl, sockets[0], F_DUPFD, STDIN_FILENO);
    VERIFY(fcntl, sockets[1], F_DUPFD, STDOUT_FILENO);
}

void setup_connection(const int connection, int program_count, int *sockets) {
    int dev_null;

    int last_fd = program_count * 2 + 3;

    VERIFY_ASSN(sockets[0], fcntl, STDIN_FILENO, F_DUPFD, last_fd);
    VERIFY_ASSN(sockets[1], fcntl, STDOUT_FILENO, F_DUPFD, last_fd + 1);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);

    if (debug_flag != 1) {
        dev_null = open("/dev/null", O_WRONLY);

       if (dev_null == -1)
          err(-1, "unable to open /dev/null");

        close(STDERR_FILENO);
        VERIFY(fcntl, dev_null, F_DUPFD, STDERR_FILENO);
        stderr = fdopen(STDERR_FILENO, "w");
        close(dev_null);
    }

    VERIFY(fcntl, connection, F_DUPFD, STDIN_FILENO);
    VERIFY(fcntl, connection, F_DUPFD, STDOUT_FILENO);
}

void setup_sockpairs(const int program_count, int destination_fd) {
    int sockets[2];
    int i;

    if (program_count > 1) {
#ifdef DEBUG
        printf("opening %d socket pairs\n", program_count);
#endif

        for (i = 0; i < program_count; i++) {
            close(destination_fd);
            close(destination_fd + 1);

            VERIFY(socketpair, AF_UNIX, SOCK_STREAM, 0, sockets);
#ifdef DEBUG
            printf("opened %d and %d\n", sockets[0], sockets[1]);
            printf("putting on on %d and %d\n", destination_fd, destination_fd + 1);
#endif

            if (sockets[0] != destination_fd)
                VERIFY(fcntl, sockets[0], F_DUPFD, destination_fd);

            destination_fd++;

            if (sockets[1] != destination_fd)
                VERIFY(fcntl, sockets[1], F_DUPFD, destination_fd);

            destination_fd++;
        }
    }
}
