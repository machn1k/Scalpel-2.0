#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT(scalpel, 2.0, scalpel@digdeeply.com)
AM_INIT_AUTOMAKE(scalpel, 2.0)
AC_CONFIG_SRCDIR([pthreads-win32/pthread.h])
#AC_CONFIG_HEADERS([config.h])
AC_PROG_INSTALL
AC_SYS_LARGEFILE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([m], [fabs])
AC_CHECK_LIB([pthread], [pthread_create], [], [AC_MSG_ERROR(Scalpel requires the pthreads library.)])
AC_CHECK_LIB([tre], [regcomp], [], [AC_MSG_ERROR(Scalpel requires libtre and libtre-dev. See http://laurikari.net/tre/.)])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stddef.h stdlib.h string.h sys/ioctl.h sys/mount.h sys/param.h sys/time.h sys/timeb.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero gettimeofday mkdir realpath regcomp strchr strdup strerror strncasecmp strtoul strtoull])

#AC_CONFIG_SUBDIRS([tre-0.7.5-win32])
AC_OUTPUT(Makefile src/Makefile man/Makefile)
