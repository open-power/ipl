AC_INIT([ipl], [1.0])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

AC_PROG_CXX

AC_CHECK_LIB([pdbg], [pdbg_targets_init], [], [], [-lfdt])
if test x"$ac_cv_lib_pdbg_pdbg_targets_init" != "xyes" ; then
	AC_MSG_ERROR([PDBG library not found])
fi

AC_CHECK_LIB([atdb], [atdb_blob_open])
if test x"$ac_cv_lib_atdb_atdb_blob_open" != "xyes" ; then
	AC_MSG_ERROR([ATDB library not found])
fi

AC_CHECK_LIB([ipl], [ipl_run_major], [LIBS="-lipl -lekb $LIBS"], [], [-lekb])
if test x"$ac_cv_lib_ipl_ipl_run_major" != "xyes" ; then
	AC_MSG_ERROR([IPL library not found])
fi

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
