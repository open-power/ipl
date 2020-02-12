AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -Im4
AM_MAKEFLAGS = --no-print-directory
AM_CXXFLAGS = -Wall -Werror

bin_PROGRAMS = istep0 ipl
lib_LTLIBRARIES = libipl.la
include_HEADERS = libipl/libipl.H

istep0_SOURCES = istep0.C
istep0_LDADD = libipl.la

ipl_SOURCES = ipl.C
ipl_LDADD = libipl.la

if BUILD_P9
libipl_istep_SOURCES = \
	libipl/p9/ipl0.C \
	libipl/p9/ipl1.C \
	libipl/p9/ipl2.C \
	libipl/p9/ipl3.C \
	libipl/p9/ipl4.C \
	libipl/p9/ipl5.C \
	libipl/p9/ipl6.C \
	libipl/p9/ipl7.C \
	libipl/p9/ipl8.C \
	libipl/p9/ipl9.C \
	libipl/p9/ipl10.C \
	libipl/p9/ipl11.C \
	libipl/p9/ipl12.C \
	libipl/p9/ipl13.C \
	libipl/p9/ipl14.C \
	libipl/p9/ipl15.C \
	libipl/p9/ipl16.C \
	libipl/p9/ipl17.C \
	libipl/p9/ipl18.C \
	libipl/p9/ipl19.C \
	libipl/p9/ipl20.C \
	libipl/p9/ipl21.C

libipl_istep_CXXFLAGS = \
	-I$(INCDIR)/ekb/chips/p9/procedures/hwp/nest
endif

if BUILD_P10
libipl_istep_SOURCES = \
	libipl/p10/common.C \
	libipl/p10/ipl0.C \
	libipl/p10/ipl1.C \
	libipl/p10/ipl2.C \
	libipl/p10/ipl3.C \
	libipl/p10/ipl4.C \
	libipl/p10/ipl5.C \
	libipl/p10/ipl6.C \
	libipl/p10/ipl7.C \
	libipl/p10/ipl8.C \
	libipl/p10/ipl9.C \
	libipl/p10/ipl10.C \
	libipl/p10/ipl11.C \
	libipl/p10/ipl12.C \
	libipl/p10/ipl13.C \
	libipl/p10/ipl14.C \
	libipl/p10/ipl15.C \
	libipl/p10/ipl16.C \
	libipl/p10/ipl17.C \
	libipl/p10/ipl18.C \
	libipl/p10/ipl19.C \
	libipl/p10/ipl20.C \
	libipl/p10/ipl21.C
endif

libipl_la_SOURCES = \
	$(libipl_istep_SOURCES) \
	libipl/libipl.C \
	libipl/libipl_internal.H \
	libipl/libipl.H

libipl_la_CXXFLAGS = -Wall -Werror $(EKB_CXXFLAGS) \
	$(libipl_istep_CXXFLAGS) \
	-I$(srcdir)/libipl
