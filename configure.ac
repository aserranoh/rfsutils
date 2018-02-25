AC_INIT([rfsutils], [0.1], [toni.serranoh@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AM_CPPFLAGS=-DSYSCONFDIR='\"$(sysconfdir)\"'
AC_SUBST(AM_CPPFLAGS)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
   Makefile
   src/Makefile
   data/Makefile
])
AC_OUTPUT
