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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sockets.h"
#include "utils.h"

int is_executable(const char *path) {
    struct stat sb;
    return ((stat(path, &sb) >= 0) && (sb.st_mode > 0) && (sb.st_mode & S_IXUSR));
}

#define HASH_LEN_EXPECTED 16
void print_hash(const unsigned char *hash, const size_t hash_len) {
    char hash_hex[HASH_LEN_EXPECTED*2 + 1];
    int i;

    if (hash_len != HASH_LEN_EXPECTED)
        err(-1, "incorrect hash size.  Expected %d, got %d", HASH_LEN_EXPECTED, hash_len);
    
    for (i = 0; i < HASH_LEN_EXPECTED; i++) {
        snprintf(hash_hex + (2 * i), 3, "%02X", hash[i]);
    }

    printf("negotiated hash: %s\n", hash_hex);
}

void print_source_identifier(const unsigned char *identifier, const size_t identifer_len) {
    unsigned int id;

    if (identifer_len != sizeof(id))
        err(-1, "incorrect source identifier size.  Expected %d, got %d", sizeof(id), identifer_len);

    id = (unsigned int) &identifier;

    printf("source identifier: %d\n", id);
}

void print_filesizes(const int program_count, char **programs) {
    struct stat sts;
    int i;
    for (i = 0; i < program_count; i++) {
        VERIFY(stat, programs[i], &sts);
        printf("stat: %s filesize %llu\n", programs[i], (unsigned long long) sts.st_size);
    }
}

void get_random(char *buf, unsigned int size) {
    int fd = open("/dev/urandom", O_RDONLY);

    if (fd < 0)
        err(-1, "unable to open /dev/urandom");

    if (read_size(fd, buf, size) != size)
        err(-1, "read_size did not read specified amount");

    close(fd);
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

#define SEED_SIZE 48

char * set_prng_seed(const unsigned char *buf, const size_t size) {
    char seed_hex[SEED_SIZE*2 + 1];
    char *seed;
    int i;

    if (size != SEED_SIZE)
        err(-1, "invalid seed size: %d", size);

    for (i = 0; i < SEED_SIZE; i++) {
        snprintf(seed_hex + (2 * i), 3, "%02X", buf[i]);
    }

    VERIFY(asprintf, &seed, "seed=%s", seed_hex);
    
    printf("seed: %s\n", seed_hex);

    return seed;
}

char * get_prng_seed() {
    unsigned char seed_raw[SEED_SIZE];

    printf("getting random seed\n");
    get_random((char *) &seed_raw, SEED_SIZE);
   
    return set_prng_seed(seed_raw, SEED_SIZE);
}
