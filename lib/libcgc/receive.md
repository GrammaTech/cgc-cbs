% CGC Application Binary Interface

# NAME
*receive* - receive bytes from a file descriptor

# LIBRARY
library "libcgc"

# SYNOPSIS
\#include \<libcgc.h\>

_int_
**receive**(_int fd_, _void *buf_, _size_t count_, _size_t *rx_bytes_)

# DESCRIPTION
The *receive* system call reads up to *count* bytes from file descriptor
*fd* to the buffer pointed to by *buf*. If *count* is zero, *receive*
returns 0 and optionally sets *\*rx_bytes* to zero.

The *receive* function is invoked through system call number 3.

# RETURN VALUE
On success, zero is returned and, if *rx_bytes* is not *NULL*, the number
of bytes received is returned in *\*rx_bytes* (zero indicates nothing
was received or end-of-file). On error, an error code is returned and
*\*rx_bytes* is left unmodified.

# ERRORS

------ --------------------------------------------------------------
EBADF  *fd* is not a valid file descriptor or is not open.
EFAULT *buf* or *rx_bytes* points to an invalid address.
------ --------------------------------------------------------------

# SEE ALSO
allocate(2),
cgcabi(2),
deallocate(2),
fdwait(2),
random(2),
_terminate(2),
transmit(2)
