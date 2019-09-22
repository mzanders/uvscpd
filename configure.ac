#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([uvscpd], [1.0.0], [maarten@zanders.be])
AC_CONFIG_SRCDIR([src/uvscpd.c])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_HEADERS([src/config.h]) # use config.h instead of passing -D in the command line
AC_CONFIG_MACRO_DIR([m4])

AC_LANG([C]) # Use C not C++

# Checks for programs.
AC_PROG_CC

# In case that you want to check for specific versions of gcc
# For example in case that you need C11 support you want to
# check for gcc-4.9
#AC_PROG_CC([gcc-4.9 gcc cc])

# AC_PROG_CC_C99 # or AC_PROG_CC_89 to force C89 mode or AC_PROG_CC_STDC to go to latest supported standard (currently C99)

AC_PROG_INSTALL
# AC_PROG_CC_C_O # Need to have per product flags myexecutable_CFLAG
# AC_PROG_RANLIB # Need for to create libraries: .a


# Checks for libraries.

# Found libraries are automatically addded to LIBS
#AC_SEARCH_LIBS([pthread])
AX_PTHREAD([],[AC_MSG_ERROR([pthreads are required])])

# AC_SEARCH_LIBS([g_test_init], [glib-2.0],[],[
#                 AC_MSG_ERROR([You need to install glib-2.0 library.])
#                 ])

# Checks for header files.
AC_HEADER_ASSERT # ./configure --disable-assert to define NDEBUG
AC_CHECK_HEADER([arpa/inet.h])
AC_CHECK_HEADER([assert.h])
AC_CHECK_HEADER([ctype.h])
AC_CHECK_HEADER([errno.h])
AC_CHECK_HEADER([fcntl.h])
AC_CHECK_HEADER([getopt.h])
AC_CHECK_HEADER([linux/can.h])
AC_CHECK_HEADER([linux/can/raw.h])
AC_CHECK_HEADER([linux/sockios.h])
AC_CHECK_HEADER([malloc.h])
AC_CHECK_HEADER([net/if.h])
AC_CHECK_HEADER([netinet/in.h])
AC_CHECK_HEADER([poll.h])
AC_CHECK_HEADER([pthread.h])
AC_CHECK_HEADER([semaphore.h])
AC_CHECK_HEADER([signal.h])
AC_CHECK_HEADER([stdint.h])
AC_CHECK_HEADER([stdio.h])
AC_CHECK_HEADER([stdlib.h])
AC_CHECK_HEADER([string.h])
AC_CHECK_HEADER([sys/ioctl.h])
AC_CHECK_HEADER([sys/socket.h])
AC_CHECK_HEADER([sys/stat.h])
AC_CHECK_HEADER([sys/types.h])
AC_CHECK_HEADER([syslog.h])
AC_CHECK_HEADER([time.h])
AC_CHECK_HEADER([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

# The following statement will use pkg-config --cflags --libs
# to find out CFLAGS and -l options required to build a target that
# it's going to link against glib2.0.
# The required CFLAGS and -l options are available as DEPS_CFLAGS
# and DEPS_LIBS in Makefile.am
# PKG_CHECK_MODULES([DEPS], [glib-2.0 >= 2.24.1])

#AC_ARG_ENABLE([myfeature],
#              AS_HELP_STRING([--disable-myfeature],[disable myfeature to get remove support for this and that]),
#              [enable_myfeature=${enableval}],[enable_myfeature=yes])
#
# if test "x${enable_myfeature}" == "xyes"; then
#   AC_DEFINE([MYFEATURE], 1, [myfeature is enabled])
# else
#   AC_MSG_WARN([
# -----------------------------------------------------------------------------------
# Are you sure that you want to have myfeature disabled? You will lose this and that.
# -----------------------------------------------------------------------------------
#                ])
# fi

AC_CONFIG_FILES([Makefile])
#AC_REQUIRE_AUX_FILE([tap-driver.sh])
#AX_VALGRIND_CHECK # http://www.gnu.org/software/autoconf-archive/ax_valgrind_check.html - make check-valgrind
#AX_CODE_COVERAGE # http://www.gnu.org/software/autoconf-archive/ax_code_coverage.html#ax_code_coverage - make check-code-coverage generates coverage report
AC_OUTPUT
