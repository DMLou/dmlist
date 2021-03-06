
# Makefile.in for PCRE (Perl-Compatible Regular Expression) library.

#---------------------------------------------------------------------------#
# To build mingw32 DLL uncomment the next two lines. This addition for      #
# mingw32 was contributed by <Paul.Sokolovsky@technologist.com>. I (Philip  #
# Hazel) don't know anything about it! There are some additional targets at #
# the bottom of this Makefile.                                              #
#---------------------------------------------------------------------------#
#
# include dll.mk
# DLL_LDFLAGS=-s


#############################################################################

# PCRE is developed on a Unix system. I do not use Windows or Macs, and know
# nothing about building software on them. Although the code of PCRE should
# be very portable, the building system in this Makefile is designed for Unix
# systems, with the exception of the mingw32 stuff just mentioned.

# This setting enables Unix-style directory scanning in pcregrep, triggered
# by the -f option. Maybe one day someone will add code for other systems.

PCREGREP_OSTYPE=-DIS_UNIX

#############################################################################


#---------------------------------------------------------------------------#
# The following lines are modified by "configure" to insert data that it is #
# given in its arguments, or which it finds out for itself.                 #
#---------------------------------------------------------------------------#

SHELL = /bin/sh
prefix = c:/progra~1/pcre
exec_prefix = ${prefix}
top_srcdir = .

mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs

# NB: top_builddir is not referred to directly below, but it is used in the
# setting of $(LIBTOOL), so don't remove it!

top_builddir = .

# BINDIR is the directory in which the pcregrep, pcretest, and pcre-config
#          commands are installed.
# INCDIR is the directory in which the public header files pcre.h and
#          pcreposix.h are installed.
# LIBDIR is the directory in which the libraries are installed.
# MANDIR is the directory in which the man pages are installed.

BINDIR = ${exec_prefix}/bin
LIBDIR = ${exec_prefix}/lib
INCDIR = ${prefix}/include
MANDIR = ${prefix}/man

CC = gcc
CFLAGS = -DSTATIC -D__STATIC__ -Wl,--static -O2 -fnative-struct -fno-rtti -fno-exceptions -mcpu=pentium -march=i386
RANLIB = ranlib
UTF8   = -DSUPPORT_UTF8
NEWLINE = 

INSTALL = /bin/install.exe -cpD
INSTALL_DATA = /bin/install.exe -cp

# LIBTOOL enables the building of shared and static libraries. It is set up
# to do one or the other or both by ./configure.

LIBTOOL = $(SHELL) $(top_builddir)/libtool
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) -c $(CFLAGS) -I. $(NEWLINE)
LINK = $(LIBTOOL) --mode=link $(CC)

# These are the version numbers for the shared libraries

PCRELIBVERSION = 0:1:0
PCREPOSIXLIBVERSION = 0:0:0

##############################################################################


OBJ = maketables.o get.o study.o pcre.o
LOBJ = maketables.lo get.lo study.lo pcre.lo

all:            libpcre.la libpcreposix.la pcretest pcregrep

pcregrep:       libpcre.la pcregrep.o
		$(LINK) $(CFLAGS) -o pcregrep pcregrep.o libpcre.la

pcretest:       libpcre.la libpcreposix.la pcretest.o
		$(LINK) $(PURIFY) $(CFLAGS) -o pcretest pcretest.o \
		libpcre.la libpcreposix.la

libpcre.la:     $(OBJ)
		-rm -f libpcre.la
		$(LINK) -rpath $(LIBDIR) -version-info \
		'$(PCRELIBVERSION)' -o libpcre.la $(LOBJ)

libpcreposix.la: pcreposix.o
		-rm -f libpcreposix.la
		$(LINK) -rpath $(LIBDIR) -version-info \
		'$(PCREPOSIXLIBVERSION)' -o libpcreposix.la pcreposix.lo

pcre.o:         $(top_srcdir)/chartables.c $(top_srcdir)/pcre.c \
		$(top_srcdir)/internal.h pcre.h config.h Makefile
		$(LTCOMPILE) $(UTF8) $(top_srcdir)/pcre.c

pcreposix.o:    $(top_srcdir)/pcreposix.c $(top_srcdir)/pcreposix.h \
		$(top_srcdir)/internal.h pcre.h config.h Makefile
		$(LTCOMPILE) $(top_srcdir)/pcreposix.c

maketables.o:   $(top_srcdir)/maketables.c $(top_srcdir)/internal.h \
		pcre.h config.h Makefile
		$(LTCOMPILE) $(top_srcdir)/maketables.c

get.o:          $(top_srcdir)/get.c $(top_srcdir)/internal.h \
		pcre.h config.h Makefile
		$(LTCOMPILE) $(top_srcdir)/get.c

study.o:        $(top_srcdir)/study.c $(top_srcdir)/internal.h \
		pcre.h config.h Makefile
		$(LTCOMPILE) $(UTF8) $(top_srcdir)/study.c

pcretest.o:     $(top_srcdir)/pcretest.c $(top_srcdir)/internal.h pcre.h config.h Makefile
		$(LTCOMPILE) $(UTF8) $(top_srcdir)/pcretest.c

pcregrep.o:     $(top_srcdir)/pcregrep.c pcre.h Makefile config.h
		$(LTCOMPILE) $(UTF8) $(PCREGREP_OSTYPE) $(top_srcdir)/pcregrep.c

# An auxiliary program makes the default character table source

$(top_srcdir)/chartables.c:   dftables
		./dftables >$(top_srcdir)/chartables.c

dftables:       $(top_srcdir)/dftables.c $(top_srcdir)/maketables.c \
		$(top_srcdir)/internal.h pcre.h config.h Makefile
		$(LINK) -o dftables $(CFLAGS) $(top_srcdir)/dftables.c

install:        all
		$(mkinstalldirs) $(DESTDIR)$(LIBDIR)
		echo "$(LIBTOOL) --mode=install $(INSTALL) libpcre.la $(DESTDIR)$(LIBDIR)/libpcre.la"
		$(LIBTOOL) --mode=install $(INSTALL) libpcre.la $(DESTDIR)$(LIBDIR)/libpcre.la
		echo "$(LIBTOOL) --mode=install $(INSTALL) libpcreposix.la $(DESTDIR)$(LIBDIR)/libpcreposix.la"
		$(LIBTOOL) --mode=install $(INSTALL) libpcreposix.la $(DESTDIR)$(LIBDIR)/libpcreposix.la
		$(LIBTOOL) --finish $(DESTDIR)$(LIBDIR)
		$(mkinstalldirs) $(DESTDIR)$(INCDIR)
		$(INSTALL_DATA) pcre.h $(DESTDIR)$(INCDIR)/pcre.h
		$(INSTALL_DATA) $(top_srcdir)/pcreposix.h $(DESTDIR)$(INCDIR)/pcreposix.h
		$(mkinstalldirs) $(DESTDIR)$(MANDIR)/man3
		$(INSTALL_DATA) $(top_srcdir)/doc/pcre.3 $(DESTDIR)$(MANDIR)/man3/pcre.3
		$(INSTALL_DATA) $(top_srcdir)/doc/pcreposix.3 $(DESTDIR)$(MANDIR)/man3/pcreposix.3
		$(mkinstalldirs) $(DESTDIR)$(MANDIR)/man1
		$(INSTALL_DATA) $(top_srcdir)/doc/pcregrep.1 $(DESTDIR)$(MANDIR)/man1/pcregrep.1
		$(INSTALL_DATA) $(top_srcdir)/doc/pcretest.1 $(DESTDIR)$(MANDIR)/man1/pcretest.1
		$(mkinstalldirs) $(DESTDIR)$(BINDIR)
		$(LIBTOOL) --mode=install $(INSTALL) pcregrep $(DESTDIR)$(BINDIR)/pcregrep
		$(LIBTOOL) --mode=install $(INSTALL) pcretest $(DESTDIR)$(BINDIR)/pcretest
		$(INSTALL) pcre-config $(DESTDIR)$(BINDIR)/pcre-config

# We deliberately omit dftables and chartables.c from 'make clean'; once made
# chartables.c shouldn't change, and if people have edited the tables by hand,
# you don't want to throw them away.

clean:;         -rm -rf *.o *.lo *.a *.la .libs pcretest pcregrep testtry

# But "make distclean" should get back to a virgin distribution

distclean:      clean
		-rm -f chartables.c libtool pcre-config pcre.h \
		Makefile config.h config.status config.log config.cache

check:          runtest

test:           runtest

runtest:        all
		./RunTest

######## MINGW32 ############### MINGW32 ############### MINGW32 #############

# This addition for mingw32 was contributed by  Paul Sokolovsky
# <Paul.Sokolovsky@technologist.com>. I (PH) don't know anything about it!

dll:            _dll libpcre.dll.a pcregrep_d pcretest_d

_dll:
		$(MAKE) CFLAGS=-DSTATIC pcre.dll

pcre.dll:       $(OBJ) pcreposix.o pcre.def
libpcre.dll.a:  pcre.def

pcregrep_d:     libpcre.dll.a pcregrep.o
		$(CC) $(CFLAGS) -L. -o pcregrep pcregrep.o -lpcre.dll

pcretest_d:     libpcre.dll.a pcretest.o
		$(PURIFY) $(CC) $(CFLAGS) -L. -o pcretest pcretest.o -lpcre.dll

# End
