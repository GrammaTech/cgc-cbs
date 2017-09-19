% CGC Application Binary Interface

# NAME
*fdwait* - wait for file descriptors to become ready

# LIBRARY
library "libcgc"

# SYNOPSIS
\#include \<libcgc.h\>

_int_ **fdwait**(_int nfds_, _fd_set *readfds_, _fd_set *writefds_, _const struct timeval *timeout_, _int *readyfds_)

# DESCRIPTION
The *fdwait* system call allows a program to monitor multiple file descriptors,
waiting until one or more of the descriptors become "ready" for some class of
I/O operation (e.g., input possible). A file descriptor is considered ready if
it is possible to perform the corresponding I/O operation (e.g., receive(2))
without blocking.

Two independent sets of file descriptors are watched. Those listed in
*readfds* will be watched to see if characters become available for reading
(more precisely, to see if a read will not block; in particular, a file
descriptor is also ready on end-of-file), and those in *writefds* will be
watched to see if a write will not block.  On exit, the sets are modified
in place to indicate which file descriptors actually changed status.
Each of the two file descriptor sets may be specified as
*NULL* if no file descriptors are to be watched for the corresponding class
of events.

Four macros are provided to manipulate the sets. *FD_ZERO* clears a set.
*FD_SET* and *FD_CLR* respectively add and remove a given file descriptor
from a set. *FD_ISSET* tests to see if a file descriptor is part of the set;
this is useful after *fdwait* returns.

The *nfds* parameter is the highest-numbered file descriptor in any of the
two sets, plus 1.

The timeout argument specifies the minimum interval that *fdwait* should
block waiting for a file descriptor to become ready. This interval will be
rounded up to the system clock granularity, and kernel scheduling delays
mean that the blocking interval may overrun by a small amount.
If both fields of the timeval structure are zero, then *fdwait* returns
immediately (useful for polling). If timeout is *NULL* (no timeout),
*fdwait* can block indefinitely.

The fdwait function is invoked through system call number 4.

# RETURN VALUE
On success, *fdwait* returns zero and, if *readyfds* is not *NULL*,
*\*readyfds* is set to the number of file descriptors contained in the two
returned descriptor sets (that is, the total number of bits that are set in
*readfds* and *writefds*) which may be zero if the timeout expires before
anything interesting happens.

On error, an error code is returned and *\*readyfds* is undefined;
the sets become undefined, so do not rely on their contents after an error.

# ERRORS

------ --------------------------------------------------------------
EBADF  an invalid file descriptor was given in one of the sets (perhaps a file descriptor that was already closed, or one on which an error has occurred).
EINVAL *nfds* is negative or the value contained within *\*timeout* is invalid.
EFAULT One of the arguments *readfds*, *writefds*, *timeout*, *readyfds* points to an invalid address.
ENOMEM unable to allocate memory for internal tables.
------ --------------------------------------------------------------

# SEE ALSO
allocate(2),
cgcabi(2),
deallocate(2),
random(2).
receive(2),
_terminate(2),
transmit(2)
