# uvscpd

**uvscpd** - Micro VSCP Daemon - is a minimalist open source implementation for
a VSCP (Very Simple Control Protocol) Daemon targeting a SocketCAN device on a
Linux host system. It basically bridges the TCP/IP interface as specified by
VSCP to a single CAN bus on which VSCP nodes reside.
The primary target is situations in which all high level decisions are made by
other components. As such, uvscpd does not contain any automation or decision
making.
The implementation aims to be small & maintainable and avoids any dependencies.

**Warning**
uvscpd is meant to be run locally and not on a public port. There are no
provisions for security or encryption. The username/password option is only
there to provide compatibility with other implementations of VSCP daemons.

## Links
More information on VSCP:
- Main VSCP page: <https://www.vscp.org/>
- VSCP Daemon documentation <https://grodansparadis.gitbooks.io/the-vscp-daemon/>
(includes the TCP/IP protocol description)

SocketCAN documentation:
- Official (& good) Linux documentation: <https://www.kernel.org/doc/Documentation/networking/can.txt>

## Building & installing
After obtaining and exctracting the distribution package, run

    ./configure
    ./make

To install the package, run './make install'.

## Running
uvscpd does not require any configuration file. All runtime configuration is
entered through command line options.

The options are:

    -h, --help: show help information
    -v, --version: show version information
    -s, --stay: don't daemonize
    -U <usr>, --user=<usr>: set username to <usr>
    -P <pwd>, --password=<pwd>: set password to <pwd>
    -c <can>, --canbus=<can>: set socketcan interface to <can>, defaults to can0
    -i <address>, --ip=<address>: bind to <address>, defaults to all interfaces
    -p <N>, --port=<N>: set IP port number to <N>, defaults to 8598
    -g <GUID>, --guid=<GUID>: set interface GUID to <GUID>, defaults to all 0's

## Access Control
uvscpd provides the means to configure a username and password combination.
This is not required, but when it is used, uvscpd checks that the supplied
username and passwords in the commands are correct. However, uvscpd functions
exactly the same with or without supplied username and password.

## IP Address selection
By default, uvscpd binds to all available IP interfaces. Please consider using
the loopback address (127.0.0.1) to ensure that external access is not possible.

## Features
uvscpd implements the following commands:
- *noop*: do nothing
- *quit*: close the TCP/IP connection
- *user*:
- *+*: repeats the last command
- *user* & *pass*: check supplied user and password but ignore them
- *send*: send VSCP frames
- *retr*: retrieve buffered VSCP frame, if argument is given, retrieve N frames
- *rcvloop*: enter receive loop mode, forwarding frames as they come in on CAN
- *quitloop*: leave receive loop mode
- *chdata*: show how many frames are in the buffer
- *clra*: flush the receive buffer
- *ggid* or *getguid*: get the configured GUID
- *sgid* or *setguid*: set the configured GUID
- *wcyd* or *whatcanyoudo*: shows encoded "what can you do" information
- *vers* or *version*: show version information
- *stat*: show some statistics on RX and TX data for this interface
- *chid*: show channel ID, always 0
- *interface list*: show interface list

Please have a look at the VSCP Daemon specification (linked above) for the exact
arguments to be passed to these commands.

## Not implemented features:
uvscpd was kept simple by not implementing these commands:
- *smsk* & *setfilter*: filtering at the daemon level is not supported
- *restart*, *shutdown*
- *help*
- *challenge*
- *info*
- *measurement*
- *driver*
- *file*
- *udp*
- *remote*
- *interface close*

Further more, uvscpd does not implement the Decision Matrix (*DM*), Variables
(*VAR*) or Tables (*TABLE*) and all associated commands.

## How it works
To guide you through the few source files, here's a bit how uvscpd is setup:

- *uvscpd.c*: the starting point and home of main(). Responsible for all 
argument parsing, showing command line info, daemonizing and handling
signals.
- *tcpserver.c*: manages the threads and dispatching. uvscpd is configured
to handle up to 5 simultaneous connections. For each of those, a worker thread
is created. A dispatch thread opens the listening socket and dispatches
incoming connections to available worker threads.   
- *tcpserver_worker.c*: the actual work done in a worker thread. Each thread
works in its own context which is initialized upon each new connection.
The worker threads block on a poll structure which is waiting for either CAN or
TCP input and handles those accordingly. This allows for all data passing
inside the thread to be synchronous.
- *tcpserver_commands.c*: the implementation for the TCP/IP commands listed
above. Conveniently uses cmd_interpreter.c to dispatch parsed commands in
argc/argv-style.
- *vscp_buffer.c*: implements a simple FIFO buffer for VSCP messages
- *cmd_interpreter.c*: command parser and executor
