% CGC Application Binary Interface

# NAME
CGCABI - CGC Application Binary Interface

# SYNOPSIS
\#include \<libcgc.h\>

# DESCRIPTION

This document details the Application Binary Interface (ABI) interface
used by to CGC Challenge binaries to access system calls and the initial
state of registers when a binary is loaded.

The ABI and system calls have been designed to create a minimal number of
system entry points that allows for the creation of CGC Challenge Binaries
that reflect real world services and allow authors to inject memory corruption
vulnerabilities consistent with the stated goals of the CGC program.

Challenge binaries are run in an Intel 32bit Architecture (IA32) protected
mode, flat memory model execution environment.

# System Calls

The following system calls may be present in a CGC binary execution
environment. With few exceptions, these system calls adhere to the POSIX
semantics associated with each corresponding system call as implemented on
the Linux operating system.  Each system call below is fully described in its
own manual page.

Name         Number
----------  --------
_terminate       1
transmit         2
receive          3
fdwait           4
allocate         5
deallocate       6
random           7
----------  --------

A prototype for each system call is given in libcgc.h and a reference
implementation can be found in libcgc.a.

# System Call ABI

The CGC application binary interface is specific to compatible x86-32 bit
CGC execution environments.  In order to invoke a CGC system call, the
system call number must be placed in the eax register. Arguments to a
system call function are passed via registers ebx, ecx, edx, esi, edi,
and ebp as summarized in the following table:

 Register   Purpose
----------  -------------------------------------------
  eax       Contains system call number
  ebx       First argument to function
  ecx       Second argument to function (if required)
  edx       Third argument to function (if required)
  esi       Fourth argument to function (if required)
  edi       Fifth argument to function (if required)
  ebp       Sixth argument to function (if required)

Invocation of a system call function is accomplished through the use of
the 'int 0x80' instruction. All system calls that return, return a
status code in the eax register. A zero status indicates a successful
system call. A non-zero status indicates an error and the specific
error number is contained in the eax register. For system calls that
return a value, the return value is provided to the caller via a caller
supplied pointer parameter. See descriptions of individual system calls
for more detailed information on both status codes and required
parameters. If eax contains an invalid system call number upon entry to
the kernel, the status ENOSYS shall be returned.

# Invalid Instructions

The following instructions have undefined behavior if used in a CGC
Challenge Binary: rdpmc, rdrand, rdtsc, and rdtscp, and sysenter.

# Signals

Traditional POSIX signals are not supported by CGC Challenge Binaries.
A CB process receiving a signal is killed by the kernel and a core(5)
file may be produced. An exception to this rule is SIGPIPE. Normally,
SIGPIPE sent to a process kills the process. A SIGPIPE results in a
system call returning EPIPE.

# Constants / Defines

Several constants, structures, and macros are defined in libcgc.h
for use by CBs. The definitions of these symbols may not be the
same as the host operating system.

Symbol
---------- ----------
EBADF
EFAULT
EINVAL
ENOMEM
ENOSYS
EPIPE
FD_CLR
FD_ISSET
_fd_mask
fd_set
FD_SET
FD_SETSIZE
FD_ZERO
_NFDBITS
NULL
SIZE_MAX
size_t
SSIZE_MAX
ssize_t
STDERR
STDIN
STDOUT
timeval

# Initial State

The initial state of the various processor and coprocessor registers are
given below.

## Initial General Purpose Register State

Register       Value
--------  ----------
EAX                0
EBX                0
ECX          [note3]
EDX                0
EDI                0
ESI                0
ESP       0xbaaaaffc
EBP                0
EIP          [note1]
EFLAGS         0x202
CS           [note2]
DS           [note2]
ES                DS
FS                DS
GS                DS
SS                DS
--------  ----------

note1: The EIP is set to the value of "c_entry" specified in the header
of the CGC binary being loaded.

note2: The values of the segment registers are not specified; however,
the CS, DS, and SS registers will be set to correctly execute
the CB and DS = ES = FS = GS = SS.

note3: The ECX register contains a pointer to a page-aligned address
filled with random data.  The special flag page is read-only and it is
not permitted to be deallocated.

The value of the EFLAGS register means that interrupts are enabled
(bit 9 set).  Bit 1 is always set.

## Initial Floating Point Unit Register State

Register             Value
------------------- ------
Control             0x037f
Status                   0
Tag                 0xffff
Opcode                   0
Instruction Pointer      0
Data Pointer             0
R0, R1, R2, R3           0
R4, R5, R6, R7           0

The Control register value is decoded as follows:
rounding control = 0 (round closes to infinitely precise result,
if equal, round to even least significant bit)
precision control = 3 (double extended precision)
all exceptions = 0 (masked)

The tag register encoding just states that all fpu stack positions are
empty (b'11 for all eight registers).

## Initial MMX register state

The initial MMX register state is the same as the state of FPU since
the registers are aliased to the same storage.

## Initial XMM Register State (SSE)

Register         Value
--------------- ------
MXCSR           0x1f80
XMM0 ... XMM15       0
--------------- ------

The MXCSR is decoded as follows:
flush to zero: disabled
rounding control = 0 (same as FPU, above)
all exceptions masked
no exceptions currently flagged

# Memory Layout

When a CB is loaded, the PT_LOAD sections of the binary describe the
layout of the binary in the 32bit virtual address space.  The permissions
of pages loaded by the binary are as specified (all pages are readable,
but may also be executable and/or writeable).

An initial stack is allocated for the process beginning from 0xbaaaaffc
and going down.  Stack pages are automatically allocated by the kernel
on behalf of the process.  All stack pages are readable, writable, and
executable.  Processes are allowed a maximum stack size of 8MB.

The data segment (program text section, readonly section, bss, and
read/write section), is limited to 1GB. This does not include memory
allocated by the allocate(2) system call.

There is no equivalent of argc, argv, envp normally provided by the
kernel to a process. The stack region is filled with zero's as it
is allocated and when execution begins at %eip, (%esp) = 0.

# Math Functions

The CGC library contains definitions for a variety of math functions
for which acceleration is available on the processor but would require
assembly language to access.  Those functions are as follows.

	 /* sine(x), where x is in radians */
	 float sinf(float);
	 double sin(double);
	 long double sinl(long double);

	 /* cosine(x), where x is in radians */
	 float cosf(float);
	 double cos(double);
	 long double cosl(long double);

	 /* tangent(x), where x is in radians */
	 float tanf(float);
	 double tan(double);
	 long double tanl(long double);

	 /* arctangent(y, x): arctangent of y/x in radians */
	 float atan2f(float, float);
	 double atan2(double, double);
	 long double atan2l(long double, long double);

	 /* square root */
	 float sqrtf(float);
	 double sqrt(double);
	 long double sqrtl(long double);

	 /* round to integral value */
	 float rintf(float);
	 double rint(double);
	 long double rintl(long double);

	 /* absolute value */
	 float fabsf(float);
	 double fabs(double);
	 long double fabsl(long double);

	 /* remainder(x,y): returns r = x - n * y where */
	 /* n is the integer closest to x/y */
	 float remainderf(float, float);
	 double remainder(double, double);
	 long double remainderl(long double, long double);

	 /* scalbn(x,y): scale x by power of 2**y: x * (2**y) */
	 float scalbnf(float, int);
	 double scalbn(double, int);
	 long double scalbnl(long double, int);
	 float scalblnf(float, long int);
	 double scalbln(double, long int);
	 long double scalblnl(long double, long int);

	 /* significand(x): mantissa of x */
	 float significandf(float);
	 double significand(double);
	 long double significandl(long double);

	 /* base 2 logarithm */
	 float log2f(float);
	 double log2(double);
	 long double log2l(long double);

	 /* natural logarithm */
	 float logf(float);
	 double log(double);
	 long double logl(long double);

	 /* base 10 logarithm */
	 float log10f(float);
	 double log10(double);
	 long double log10l(long double);

	 /* base 2 exponential */
	 float exp2f(float);
	 double exp2(double);
	 long double exp2l(long double);

	 /* natural (base e) exponential */
	 float expf(float);
	 double exp(double);
	 long double expl(long double);

	 /* pow(x,y) = x**y */
	 float powf(float, float);
	 double pow(double, double);
	 long double powl(long double, long double);

# Other Functions

In addition to the system calls and math functions, two additional
functions are provided by the CGC ABI: setjmp() and longjmp(). These
functions allow for saving processor state and restoring it at some
point in the future.  Their behavior is meant to mimic the behavior
of these functions available in other operating systems.

# CAVEATS

The pow() functions are not conformant with C99.  For instance,
they return NaN for infinite exponents and for finite negative
bases with finite integer exponents.

# SEE ALSO

cgc_executable_format(1),
_terminate(2),
transmit(2),
receive(2),
fdwait(2),
allocate(2),
deallocate(2),
random(2),
core(5).

# FILES

/usr/include/libcgc.h
/usr/lib/libcgc.a
