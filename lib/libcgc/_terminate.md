% CGC Application Binary Interface

# NAME
*_terminate* - terminate the execution of a program

# LIBRARY
library "libcgc"

# SYNOPSIS
\#include \<libcgc.h\>

_void_ **_terminate**(_int status_)

# DESCRIPTION
The *_terminate* system call terminates a process.  All file descriptors
open in the process are flushed and closed. The scoring system is notified
of the value of *status*.

The *_terminate* function is invoked through system call number 1.

# RETURN VALUE
The *_terminate* system call never returns.

# SEE ALSO
allocate(2),
cgcabi(2),
deallocate(2),
fdwait(2),
random(2),
receive(2),
transmit(2)
