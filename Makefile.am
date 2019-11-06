AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -Im4
AM_MAKEFLAGS = --no-print-directory
AM_CXXFLAGS = -Wall -Werror

bin_PROGRAMS = istep0 ipl
lib_LTLIBRARIES = libipl.la
include_HEADERS = libipl/libipl.h

istep0_SOURCES = istep0.C
istep0_LDADD = libipl.la

ipl_SOURCES = ipl.C
ipl_LDADD = libipl.la

libipl_la_SOURCES = \
	libipl/ipl_init.C \
	libipl/ipl0.C \
	libipl/ipl1.C \
	libipl/ipl2.C \
	libipl/ipl3.C \
	libipl/ipl4.C \
	libipl/ipl5.C \
	libipl/ipl6.C \
	libipl/ipl7.C \
	libipl/ipl8.C \
	libipl/ipl9.C \
	libipl/ipl10.C \
	libipl/ipl11.C \
	libipl/ipl12.C \
	libipl/ipl13.C \
	libipl/ipl14.C \
	libipl/ipl15.C \
	libipl/ipl16.C \
	libipl/ipl17.C \
	libipl/ipl18.C \
	libipl/ipl19.C \
	libipl/ipl20.C \
	libipl/ipl21.C \
	libipl/libipl_internal.h \
	libipl/libipl.c \
	libipl/libipl.h

libipl_la_CXXFLAGS = -Wall -Werror $(EKB_CXXFLAGS) \
	-I$(INCDIR)/ekb/chips/p9/procedures/hwp/nest

