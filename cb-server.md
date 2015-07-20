% CB-SERVER(1) Cyber Grand Challenge Manuals
% Brian Caswell <bmc@lungetech.com>
% April 17, 2014

# NAME

cb-server - Challenge Binary launch daemon

# SYNOPSIS

cb-server [options] -p *PORT* -d *DIRECTORY* challenge-binary [... challenge-binary]

# DESCRIPTION

cb-server is a inetd style TCP server, launching instances of the specified challenge binaries for each connection within a restricted environment.  Challenge binaries are able to communicate via the TCP connection with the use of *STDIN* and *STDOUT*.

# ARGUMENTS

-p *PORT*
:   Specify the TCP Port used for incoming connections.

-d *DIRECTORY*
:   Specify the directory containing the challenge binaries

`challenge-binary`
:   The filename the specified challenge-binary.  *NOTE:* The file must exist in the specified directory.

# OPTIONS

-h
:   Display a usage message and exit

-c *SIZE*
:   Specify a maximum core file size.  Specifying a *SIZE* value of 0 will disable creating core files.

-w *WRAPPER*
:   Specify an executable that will wrap the containing challenge binaries

-s *SEED*
:   Specify the per-process PRNG seed.

-S *SKIPRNG*
:   Specify the amount to advance the PRNG state before launching the challenge binary

-t *TIMEOUT*
:   Specify the maximum amount of time each instance of a challenge binary may execute.  Specifying a *TIMEOUT* value of 0 will use the default value.

-b *BACKLOG*
:   Specify the maximum amount of pending TCP connections that can be held while existing instances are executing.

-m *MAX_CONNECTIONS*
:   Specify the maximum total connections to accept.

-l *CHILDREN*
:   Specify the maximum amount of concurrently executing challenge binaries.

--insecure
:   Launch the challenge binaries in a less restricted environment

--debug
:   Enable debugging output.

# RESTRICTED ENVIRONMENT

cb-server performs numerous actions to restrict the execution environment of the challenge binaries before execution.

`chroot`
:   cb-server will *chroot* into the directory specified via command-line argument unless the *insecure* option is specified.  The insecure flag will not chroot, rather chdir to the specified directory.  This reduces the ability of the challenge binaries to impact the rest of the running system.

`random UID/GID`
:   cb-server will identify an unused uid and gid via getpwuid and getgrgid respectively for each connection, setting the UID and GID to the identified values prior to execution of the challenge binaries.  This reduces the ability of the challenge binary to impact other instances of challenge binaries.

`rlimits`
:   cb-server will set the maximum number of file descriptors that may be opened, bytes that can be allocated for message queues, number of flock/fcntl locks that may be established, and amount of memory that can be locked into ram to 0.  The maximum size of the process stack for the challenge binaries is set to 8MB.  The maximum size of the data segment for the challenge binaries is set to 1GB.

# CHALLENGE BINARY IPC

cb-server allows for IPC between multiple challenge binaries per connection via sockets allocated via `sockpair`.   If multiple binaries are specified on the command line, an additional sockpair is created for each binary.  All sockets, including the original *STDIN*, *STDOUT*, and *STDERR*, are made available to all of the specified challenge binaries.

This allows a CB author wishing to use IPC to complex multi-process challenges, including pipelined services or producer/consumer models.

The TCP connection is available to the challenge binaries via the file descriptor 0 and 1, *STDIN* and *STDOUT* respectively, the file descriptor 2 used for debugging purposes as *STDERR*, with each additional socket allocated sequentially starting at 3.

# EXAMPLE USES

- cb-server -p 10000 -d /tmp cb1

This will create a server to handle network connections on TCP port 10000, chrooting to '/tmp', and setting the UID and GID to random unused values, launching 'cb1' upon each connection.

- cb-server --insecure -p 10000 -d /bin echo

This will create a server to handle network connections on TCP port 10000, changing directory to '/bin', launching 'cb1' upon each connection.

- cb-server -p 10000 -d /tmp cb1 cb2 cb3

This will create a server to handle network connections on TCP port 10000, chrooting to '/tmp', and setting the UID and GID to random unused values, launching 'cb1', 'cb2', and 'cb3' upon each connection.  Three additional socketpairs will be allocated and provided to each of the processes, with the IDs 3, 4, 5, 6, 7, and 8

# COPYRIGHT

Copyright (C) 2014, Brian Caswell <bmc@lungetech.com>

# SEE ALSO

`setrlimit` (2), `chroot` (2), `getpwuid` (3), `getgrgid` (3), `socketpair` (2)

For more information relating to DARPA's Cyber Grand Challenge, please visit <http://www.darpa.mil/cybergrandchallenge/>
