AC_INIT([ipl], [1.0])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

CHIP="p9"
AC_SUBST([CHIP])

AM_CONDITIONAL([BUILD_P9], [test "$CHIP" = "p9"])

AC_LANG([C++])

AC_PROG_CXX
AC_PROG_LIBTOOL

AC_CHECK_LIB([pdbg], [pdbg_targets_init], [LIBS="-lpdbg -lfdt $LIBS"], [], [-lfdt])
if test x"$ac_cv_lib_pdbg_pdbg_targets_init" != "xyes" ; then
	AC_MSG_ERROR([PDBG library not found])
fi

# Find path to libekb.H
CPPFLAGS=$CXXFLAGS
AX_ABSOLUTE_HEADER([libekb.H])
if [ x"$gl_cv_absolute_libekb_H" = "x" ] ; then
	AC_MSG_ERROR([Cannot find libekb.H path])
fi
INCDIR=$(dirname $gl_cv_absolute_libekb_H)
AC_SUBST([INCDIR])

EKB_CXXFLAGS=" \
-I${INCDIR}/hwpf/fapi2/include \
-I${INCDIR}/hwpf/fapi2/include/attributes \
-I${INCDIR}/hwpf/fapi2/include/error_info \
-I${INCDIR}/hwpf/fapi2/include/plat \
-I${INCDIR}/ekb/hwpf/fapi2/include \
-I${INCDIR}/ekb/chips/${CHIP}/procedures/hwp/ffdc"
AC_SUBST([EKB_CXXFLAGS])

saved_LDFLAGS=$LDFLAGS
saved_CXXFLAGS=$CXXFLAGS
LDFLAGS="$LDFLAGS -lekb -lfdt-prop"
CXXFLAGS="$CXXFLAGS $EKB_CXXFLAGS"
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([#include <libekb.H>],
    [libekb_init])],
  [LIBS="$LIBS -lekb -lfdt-prop"],
  [AC_MSG_ERROR([libekb not found.])])
LDFLAGS=$saved_LDFLAGS
CXXFLAGS=$saved_CXXFLAGS

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
