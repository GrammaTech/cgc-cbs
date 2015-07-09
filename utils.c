/*
* Utilities
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

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"

int is_executable(const char *path) {
    struct stat sb;
    return ((stat(path, &sb) >= 0) && (sb.st_mode > 0) && (sb.st_mode & S_IXUSR));
}

void print_filesizes(const int program_count, char **programs) {
    struct stat sts;
    int i;
    for (i = 0; i < program_count; i++) {
        VERIFY(stat, programs[i], &sts);
        printf("stat: %s filesize %llu\n", programs[i], (unsigned long long) sts.st_size);
    }
}

int get_random(char *buf, unsigned int size) {
    int fd = open("/dev/urandom", O_RDONLY);

    if (fd < 0)
        err(-1, "unable to open /dev/urandom");

    if (read_size(fd, buf, size) != size)
        err(-1, "read_size did not read specified amount");

    close(fd);
    return 1;
}

size_t read_size(int fd, char *buf, const size_t size) {
    ssize_t bytes_read = 0;
    size_t total = 0;
    size_t bytes_to_read = size;

    while (bytes_to_read > 0) {
        bytes_read = read(fd, buf + total, bytes_to_read);

        if (bytes_read <= 0)
            err(-1, "unable to read %zu bytes from %d", size, fd);

        total += (size_t) bytes_read;
        bytes_to_read = size - total;
    }

    return total;
}

unsigned long str_to_ulong(const char *s) {
    unsigned long result = 0;
    unsigned int pos = 0;
    unsigned char c;

    while ((c = (unsigned char)(s[pos] - '0')) < 10) {
        result = result * 10 + c;
        pos++;
    }

    return result;
}

unsigned short str_to_ushort(const char *s) {
    unsigned long result = 0;
    unsigned int pos = 0;
    unsigned char c;

    while ((c = (unsigned char)(s[pos] - '0')) < 10) {
        result = result * 10 + c;
        pos++;
    }

    return result & 0xFFFF;
}
