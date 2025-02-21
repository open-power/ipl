AC_INIT([ipl], [1.1])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])
AC_SUBST(SONAME_CURRENT, 1)
AC_SUBST(SONAME_REVISION, 0)
AC_SUBST(SONAME_AGE, 0)

AC_ARG_VAR(CHIP, [Target chip to build respective ipl steps])
AS_IF([test "x$CHIP" == "x"], [CHIP="p10"])
AC_SUBST([CHIP])

AM_CONDITIONAL([BUILD_P9], [test "$CHIP" = "p9"])
AM_CONDITIONAL([BUILD_P10], [test "$CHIP" = "p10"])

if test "$CHIP" = "p9" ; then
	AC_DEFINE([IPL_P9], [1], [Define to 1 for P9])
fi

if test "$CHIP" = "p10" ; then
	AC_DEFINE([IPL_P10], [1], [Define to 1 for P10])
fi

AC_ARG_ENABLE([libphal],
	AS_HELP_STRING([--enable-libphal]
		[build phal util library]))
AM_CONDITIONAL([BUILD_PHAL_API], [test "x$enable_libphal" = "xyes"])

if test "x$enable_libphal" = "xyes"; then
	PKG_CHECK_MODULES([LIBDT_API], [libdt-api])
	LIBS="$LIBS $LIBDT_API_LIBS"
	CFLAGS="$CFLAGS $LIBDT_API_CFLAGS"
fi

AC_LANG([C++])
# Adding the required C++ version directly is not typically recommended
# practice. However, Autotools does not support AX_CXX_COMPILE_STDCXX_23 or
# AX_CXX_COMPILE_STDCXX for version 23.
saved_CXXFLAGS=$CXXFLAGS
CXXFLAGS="$saved_CXXFLAGS -std=c++23"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <expected>],
                                   [std::expected<int, int> testObj;])],
                  [saved_CXXFLAGS="$saved_CXXFLAGS -std=c++23"],
                  [AC_MSG_ERROR([C++23 is not supported, but it is required])])
CXXFLAGS=$saved_CXXFLAGS

AC_PROG_CXX
AC_PROG_LIBTOOL

AC_CHECK_LIB([pdbg], [pdbg_targets_init], [LIBS="-lpdbg -lfdt $LIBS"], [], [-lfdt])
if test x"$ac_cv_lib_pdbg_pdbg_targets_init" != "xyes" ; then
	AC_MSG_ERROR([PDBG library not found])
fi

# Find path to libekb.H
CPPFLAGS=$CXXFLAGS
AX_ABSOLUTE_HEADER([libekb.H])
if test x"$gl_cv_absolute_libekb_H" = "x" ; then
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
-I${INCDIR}/ekb/chips/${CHIP}/procedures/hwp/ffdc \
-I${INCDIR}/ekb/chips/${CHIP}/procedures/hwp/lib \
-I${INCDIR}/ekb/chips/${CHIP}/procedures/hwp/sbe" 

AC_SUBST([EKB_CXXFLAGS])

saved_LDFLAGS=$LDFLAGS
saved_CXXFLAGS=$CXXFLAGS
LDFLAGS="$LDFLAGS -lekb"
CXXFLAGS="$CXXFLAGS $EKB_CXXFLAGS"
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([#include <libekb.H>],
    [libekb_init])],
  [LIBS="$LIBS -lekb"],
  [AC_MSG_ERROR([libekb not found.])])
LDFLAGS=$saved_LDFLAGS
CXXFLAGS=$saved_CXXFLAGS

saved_LDFLAGS=$LDFLAGS
saved_CXXFLAGS=$CXXFLAGS
LDFLAGS="$LDFLAGS -lguard"
CXXFLAGS="$CXXFLAGS -I${INCDIR}/libguard -I${INCDIR}/libguard/include"
AC_LINK_IFELSE(
               [AC_LANG_PROGRAM([#include <guard_interface.hpp>],
			   [openpower::guard::libguard_init])],
			   [LIBS="$LIBS -lguard"],
			   [AC_MSG_ERROR([Guard library not found])])
LDFLAGS=$saved_LDFLAGS
CXXFLAGS=$saved_CXXFLAGS

LDFLAGS="$LDFLAGS -lhostfw_ipl"
CXXFLAGS="$CXXFLAGS -I${INCDIR}"
AC_LINK_IFELSE(
               [AC_LANG_PROGRAM([#include<iplfacade.H>],
			   [])],
			   [LIBS="$LIBS -lhostfw_ipl"],
			   [AC_MSG_ERROR([hostfw library not found])])
LDFLAGS=$saved_LDFLAGS
CXXFLAGS=$saved_CXXFLAGS

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
