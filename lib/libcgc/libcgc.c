/* Copyright 2015 Peter Goodman (peter@trailofbits.com), all rights reserved. */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <err.h>

#define LIBCGC_IMPL
#include "libcgc.h"
#undef fd_set
#undef timeval

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) < (b)) ? (b) : (a))

/* Terminates the process. */
void _terminate(unsigned int status) {
  _exit(status);
  __builtin_unreachable();
}

/* There used to be some broken code here to check if a pointer
   was readable/writable, it didn't work, and I don't think any
   cqe binaries actually depend on CGC_EFAULT being returned by
   syscalls, so I gutted it. Someday maybe we'll want it back
   though, so I didn't remove the calls to it. -BA (GT) */
#define OBJECT_IS_READABLE(x) 1
#define OBJECT_IS_WRITABLE(x) 1
#define num_readable_bytes(ptr, count) count
#define num_writable_bytes(ptr, count) count

/* Updates a byte counter and returns the corresponding status code. */
static int update_byte_count(size_t *counter, size_t count) {
  if (!counter) return 0;
  if (!OBJECT_IS_WRITABLE(counter)) {
    return CGC_EFAULT;
  } else {
    *counter = count;
    return 0;
  }
}

/* Transmits data from one CGC process to another. */
int transmit(int fd, const void *buf, size_t count, size_t *tx_bytes) {
  if (!count) {
    return update_byte_count(tx_bytes, 0);
  } else if (0 > fd) {
    return CGC_EBADF;
  }

  const size_t max_count = num_readable_bytes(buf, count);
  if (!max_count) {
    return CGC_EFAULT;
  } else if (max_count < count) {
    count = max_count & ~2047;
  } else {
    count = max_count;
  }

  errno = 0;
  const ssize_t ret = write(fd, buf, count);
  const int errno_val = errno;
  errno = 0;

  if (EFAULT == errno_val) {
    return CGC_EFAULT;
  } else if (EBADF == errno_val) {
    return CGC_EBADF;
  } else if (errno_val) {
    return CGC_EPIPE;  /* Guess... */
  } else {
    return update_byte_count(tx_bytes, (size_t) ret);
  }
}

/* Receives data from another CGC process. */
int receive(int fd, void *buf, size_t count, size_t *rx_bytes) {
  if (!count) {
    return update_byte_count(rx_bytes, 0);
  } else if (0 > fd) {
    return CGC_EBADF;
  }

  const size_t max_count = num_writable_bytes(buf, count);
  if (!max_count) {
    return CGC_EFAULT;
  }

  errno = 0;
  const ssize_t ret = read(fd, buf, max_count);
  const int errno_val = errno;
  errno = 0;

  if (EFAULT == errno_val) {
    return CGC_EFAULT;
  } else if (EBADF == errno_val) {
    return CGC_EBADF;
  } else if (errno_val) {
    return CGC_EPIPE;  /* Guess... */
  } else {
    return update_byte_count(rx_bytes, (size_t) ret);
  }
}

/* Tries to validate a timeout. */
static int check_timeout(const struct cgc_timeval *timeout) {
  if (!timeout) {
    return 0;
  } else if (!OBJECT_IS_READABLE(timeout)) {
    return CGC_EFAULT;
  } else if (0 > timeout->tv_sec || 0 > timeout->tv_usec) {
    return CGC_EINVAL;
  } else {
    return 0;
  }
}

enum {
    // Maximum number of binaries running for one challenge
    kPracticalMaxNumCBs = 10,
    
    // STD(IN/OUT/ERR) + a socketpair for every binary
    // All fds used by the binaries should be less than this
    kExpectedMaxFDs = 3 + (2 * kPracticalMaxNumCBs)
};

/* Adapted from Trail of Bits' libcgc */
/* Marshal a CGC fd set into an OS fd set. */
static int copy_cgc_fd_set(const cgc_fd_set *cgc_fds, fd_set *os_fds, int *num_fds) {
  unsigned fd;
  for (fd = 0; fd < CGC__NFDBITS; ++fd) {
    if (CGC_FD_ISSET(fd, cgc_fds)) {
      // Shouldn't be using an fd greater than the allowed values
      if (fd >= kExpectedMaxFDs) {
          return CGC_EBADF;
      }
      
      if (fd > NFDBITS) {
        continue;  /* OS set size is too small. */
      }
      FD_SET(fd, os_fds);
      ++*num_fds;
    }
  }
  return 0;
}

/* Adapted from Trail of Bits' libcgc */
/* Marshal an OS fd set into a CGC fd set. */
static void copy_os_fd_set(const fd_set *os_fds, cgc_fd_set *cgc_fds) {
  unsigned fd;
  for (fd = 0; fd < MIN(NFDBITS, CGC__NFDBITS); ++fd) {
    if (FD_ISSET(fd, os_fds)) {
      CGC_FD_SET(fd, cgc_fds);
    }
  }
}

/* Adapted from Trail of Bits' libcgc */
int fdwait(int nfds, cgc_fd_set *readfds, cgc_fd_set *writefds,
               const struct cgc_timeval *timeout, int *readyfds) {

  int ret = check_timeout(timeout);
  int actual_num_fds = 0;
  struct timeval max_wait_time = {0, 0};
  fd_set read_fds;
  fd_set write_fds;

  if (ret) {
    return ret;
  } else if (0 > nfds || CGC__NFDBITS < nfds) {
    return EINVAL;
  }

  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);

  if (readfds) {
    if (!OBJECT_IS_WRITABLE(readfds)) {  /* Opportunistic. */
      return CGC_EFAULT;
    } else if (0 != (ret = copy_cgc_fd_set(readfds, &read_fds, &actual_num_fds))) {
      return ret;
    }
  }

  if (writefds) {
    if (!OBJECT_IS_WRITABLE(writefds)) {  /* Opportunistic. */
      return CGC_EFAULT;
    } else if (0 != (ret = copy_cgc_fd_set(writefds, &write_fds, &actual_num_fds))) {
      return ret;
    }
  }

  if (actual_num_fds != nfds) {
    return EINVAL;  /* Not actually specified, but oh well. */
  }

  if (readfds)  CGC_FD_ZERO(readfds);
  if (writefds) CGC_FD_ZERO(writefds);

  if (timeout) {
    max_wait_time.tv_sec = timeout->tv_sec;
    max_wait_time.tv_usec = timeout->tv_usec;
  }

  errno = 0;
  int num_selected_fds = select(
          nfds,
          (readfds ? &read_fds : NULL),
          (writefds ? &write_fds : NULL),
          NULL,
          (timeout ? &max_wait_time : NULL));
  const int errno_val = errno;
  errno = 0;

  if (errno_val) {
    if (ENOMEM == errno_val) {
      return CGC_ENOMEM;
    } else if (EBADF == errno_val) {
      return CGC_EBADF;
    } else {
      return CGC_EINVAL;
    }
  }

  if (readfds) {
    copy_os_fd_set(&read_fds, readfds);
  }

  if (writefds) {
    copy_os_fd_set(&write_fds, writefds);
  }

  if (readyfds) {
    if (!OBJECT_IS_WRITABLE(readyfds)) {
      return CGC_EFAULT;
    }
    *readyfds = num_selected_fds;
  }

  return 0;
}

#define PAGE_ALIGN(x) (((x) + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1))

int allocate(size_t length, int is_executable, void **addr) {
  void *p;
  
  if(!length)
    return CGC_EINVAL;

  if (!OBJECT_IS_WRITABLE(addr))
    return CGC_EFAULT;
  
  length = PAGE_ALIGN(length);
  
  p = mmap(NULL, length, 
           PROT_READ | PROT_WRITE | (is_executable ? PROT_EXEC : 0),
           MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

  if(p == MAP_FAILED)
    return CGC_ENOMEM;

  *addr = p;
  return 0;
}

int deallocate(void *addr, size_t length) {
  length = PAGE_ALIGN(length);
  return munmap(addr, length) < 0 ? CGC_EINVAL : 0;
}

/* So this isn't really a random number generator. */
int random(void *buf, size_t count, size_t *rnd_bytes) {
  if (!count) {
    return update_byte_count(rnd_bytes, 0);
  } else if (count > SSIZE_MAX) {
    return CGC_EINVAL;
  } else if (!(count = num_writable_bytes(buf, count))) {
    return CGC_EFAULT;
  } else {
#if defined(APPLE)
    // TODO: Support seeds from the testing. arc4random_buf is easy but 
    //  not the right way to do it.
    arc4random_buf(buf, count);
    return update_byte_count(rnd_bytes, count);
#else
	//FILE *rdev = fopen("/dev/urandom", "rb");
	//fread(buf, count, 1, rdev);
	//fclose(rdev);
    int fd;
    ssize_t n;
    if ((fd = open("/dev/urandom", O_RDONLY)) < 0)
        return CGC_ENOSYS;
    n = read(fd, buf, count);
    close(fd);
    return (n < 0 ? CGC_ENOSYS : update_byte_count(rnd_bytes, n));
#endif
  }
}

void *cgc_initialize_secret_page(void)
{
  void * MAGIC_PAGE_ADDRESS = (void *)0x4347C000;
  const size_t MAGIC_PAGE_SIZE = 4096;

  void *mmap_addr = mmap(MAGIC_PAGE_ADDRESS, MAGIC_PAGE_SIZE, 
                         PROT_READ | PROT_WRITE,
                         MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, 
                         -1, 0);

  if (mmap_addr != MAGIC_PAGE_ADDRESS)
  {
    err(1, "[!] Failed to map the secret page");
  }

#if defined(APPLE)
    // TODO: Support seeds from the testing. arc4random_buf is easy but 
    //  not the right way to do it.
    arc4random_buf(mmap_addr, MAGIC_PAGE_SIZE);
#else
	//FILE *rdev = fopen("/dev/urandom", "rb");
	//fread(mmap_addr, MAGIC_PAGE_SIZE, 1, rdev);
	//fclose(rdev);
    int fd;
    ssize_t n;
    if ((fd = open("/dev/urandom", O_RDONLY)) < 0) {
        err(1, "[!] Failed to open /dev/urandom");
    }
    n = read(fd, mmap_addr, MAGIC_PAGE_SIZE);
    close(fd);
#endif

  return mmap_addr;
}
