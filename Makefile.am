AM_CFLAGS = $(PTHREAD_CFLAGS) # CFLAGS applicable to all executables (products)
AM_CPPFLAGS = -I$(top_srcdir)/src # so that tests also find header files

AM_LDFLAGS = $(PTHREAD_LIBS)

# The main product
bin_PROGRAMS = uvscpd

#include .c and .h in SOURCES so that both appear in dist
uvscpd_SOURCES = \
											 src/cmd_interpreter.c \
											 src/cmd_interpreter.h \
                       src/syserror.c \
                       src/syserror.h \
                       src/tcpserver_commands.c \
                       src/tcpserver_commands.h \
                       src/tcpserver_context.h \
                       src/tcpserver_worker.c \
                       src/tcpserver_worker.h \
                       src/tcpserver.c \
                       src/tcpserver.h \
                       src/uvscpd.c \
                       src/version.h \
                       src/vscp_buffer.c \
                       src/vscp_buffer.h \
                       src/vscp.c \
                       src/vscp.h
