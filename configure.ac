AC_INIT([ipl], [1.0])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

AC_LANG([C++])

AC_PROG_CXX
AC_PROG_LIBTOOL

AC_CHECK_LIB([pdbg], [pdbg_targets_init], [LIBS="-lpdbg -lfdt $LIBS"], [], [-lfdt])
if test x"$ac_cv_lib_pdbg_pdbg_targets_init" != "xyes" ; then
	AC_MSG_ERROR([PDBG library not found])
fi

AC_CHECK_LIB([atdb], [atdb_blob_open])
if test x"$ac_cv_lib_atdb_atdb_blob_open" != "xyes" ; then
	AC_MSG_ERROR([ATDB library not found])
fi

# Assume EKB is isntalled at the same place as libpdbg.h
CPPFLAGS=$CXXFLAGS
AX_ABSOLUTE_HEADER([libpdbg.h])
if [ x"$gl_cv_absolute_libpdbg_h" = "x" ] ; then
	AC_MSG_ERROR([Cannot find libpdbg.h path])
fi
INCDIR=$(dirname $gl_cv_absolute_libpdbg_h)
AC_SUBST([INCDIR])

EKB_CXXFLAGS="-DFAPI2_NO_FFDC \
-I${INCDIR}/hwpf/fapi2/include \
-I${INCDIR}/hwpf/fapi2/include/attributes \
-I${INCDIR}/hwpf/fapi2/include/error_info \
-I${INCDIR}/hwpf/fapi2/include/plat \
-I${INCDIR}/ekb/hwpf/fapi2/include"
AC_SUBST([EKB_CXXFLAGS])

saved_LDFLAGS=$LDFLAGS
saved_CXXFLAGS=$CXXFLAGS
LDFLAGS="$LDFLAGS -lekb"
CXXFLAGS="$CXXFLAGS $EKB_CXXFLAGS"
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([#include <plat_trace.H>
#include <plat_utils.H>],
    [plat_set_atdb_context])],
  [LIBS="$LIBS -lekb"],
  [AC_MSG_ERROR([libekb not found.])])
LDFLAGS=$saved_LDFLAGS
CXXFLAGS=$saved_CXXFLAGS

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
