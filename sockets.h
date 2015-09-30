/*
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

#ifndef SOCKETS_H
#define SOCKETS_H

#include <stdint.h>

void reset_base_sockets(int *sockets);
int socket_accept(int server);
int socket_bind(const unsigned short port, const int backlog);
void setup_connection(int connection, int program_count, int *sockets);
void setup_sockpairs(int program_count, int destination_fd);
void close_saved_sockets(int *);
void setup_pairwise_wait(int pause_sockets[2]);
void ready_pairwise(int pause_sockets[2]);
void wait_pairwise(int pause_sockets[2]);

size_t read_size(int fd, char *buf, const size_t size);
unsigned char * read_buffer(const int fd, const size_t size);
uint32_t read_uint32_t(int fd);
void send_all(int fd, char *buf, const size_t size);

#endif
