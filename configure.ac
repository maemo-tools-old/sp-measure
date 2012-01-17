#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.13])
AC_INIT([sp-measure], [1.3], [none])

AC_CONFIG_AUX_DIR([aux])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_CONFIG_SRCDIR([src/sp_measure.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
m4_defun([_LT_AC_LANG_F77_CONFIG], [:])


AC_PROG_CC
AC_PROG_LIBTOOL

# set libtool versioning
AC_SUBST(VERSION_INFO, $(echo -version-info $VERSION | sed s/\\./:/g))

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h memory.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memset strchr strdup strrchr strstr])

AC_CONFIG_FILES([Makefile
		src/Makefile
		doc/Makefile
		tests/Makefile])
AC_OUTPUT
