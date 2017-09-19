% CGC Application Binary Interface

# NAME
*transmit* - send bytes through a file descriptor

# LIBRARY
library "libcgc"

# SYNOPSIS
\#include \<libcgc.h\>

_int_ **transmit**(_int fd_, _const void *buf_, _size_t count_, _size_t *tx_bytes_)

# DESCRIPTION
The *transmit* system call writes up to *count* bytes from the buffer
 pointed to by *buf* to the file descriptor *fd*. If *count* is zero,
*transmit* returns 0 and optionally sets *\*tx_bytes* to zero.

The *transmit* function is invoked through system call number 2.

# RETURN VALUE
On success, zero is returned and, if *tx_bytes* is not *NULL*, the number
of bytes transmitted in returned in *\*tx_bytes* (zero indicates nothing
was transmitted). On error, an error code is returned and *\*tx_bytes*
is left unmodified.

# ERRORS

------ --------------------------------------------------------------
EBADF  *fd* is not a valid file descriptor or is not open.
EFAULT *buf* or *tx_bytes* points to an invalid address.
------ --------------------------------------------------------------

# SEE ALSO
allocate(2),
cgcabi(2),
deallocate(2),
fdwait(2),
random(2),
receive(2),
_terminate(2)
