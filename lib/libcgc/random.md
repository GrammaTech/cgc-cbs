% CGC Application Binary Interface

# NAME
*random* - fill a buffer with random data

# LIBRARY
library "libcgc"

# SYNOPSIS
\#include \<libcgc.h\>

_int_ **random**(_void *buf_, _size_t count_, _size_t *rnd_bytes_)

# DESCRIPTION
The *random* system call populates the buffer referenced by *buf*
with up to count bytes of random data. If *count* is zero, *random*
returns 0 and optionally sets *\*rnd_bytes* to zero.
If *count* is greater than *SSIZE_MAX*, the result is unspecified.

The *random* function is invoked through system call number 7.

# RETURN VALUE
On success, zero is returned and if *rnd_bytes* is not *NULL*, the
number of bytes copied into *buf* is returned in *\*rnd_bytes*.
On error, an error code is returned, the contents of *\*buf*
are undefined, and the value of *\*rnd_bytes* is left unmodified.

# ERRORS
------ --------------------------------------------------------------
EINVAL *count* is invalid.
EFAULT *buf* or *rnd_bytes* points to an invalid address.
------ --------------------------------------------------------------

# SEE ALSO
allocate(2),
cgcabi(2),
deallocate(2),
fdwait(2),
receive(2),
_terminate(2),
transmit(2)
