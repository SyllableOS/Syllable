######################################################################
#
# Makefile for Info-ZIP's unzip, unzipsfx, and funzip on AtheOS
# Version 5.53
#
# Copyright (C) 1998-2006 Info-ZIP
#                    Chris Herborth (chrish@qnx.com)
# Copyright (C) 2004 Nikolaev Ruslan (nruslan@hotbox.ru)
#
# To automatically install the fresh new unzip, use the "install" target:
# make -f atheos/Makefile install
######################################################################

# Punish those of you not running on SMP hardware...
MAKE  = make -j 4 -f atheos/Makefile

#####################
# MACRO DEFINITIONS #
#####################

# Defaults most systems use (use LOCAL_UNZIP in environment to add flags,
# such as -DDOSWILD).

# UnZip flags
CC = gcc
LD = $(CC)
AS = as
LOC = $(D_USE_BZ2) $(LOCAL_UNZIP)
AF = $(LOC)
#CFLAGS = -O
CFLAGS = -O3 -march=i586 -Wall -I. -DPASSWD_FROM_STDIN -DASM_CRC -DUSE_UNSHRINK $(LOCAL_UNZIP)
CF_NOOPT = -I. -I$(IZ_BZIP2) -DUNIX $(LOC)
CF = $(CFLAGS) $(CF_NOOPT)
LFLAGS1 =
LF = -o unzip$E $(LFLAGS1)
LF2 = -s

# UnZipSFX flags
SL = -o unzipsfx$E $(LFLAGS1)
SL2 = $(LF2)

# fUnZip flags
FL = -o funzip$E $(LFLAGS1)
FL2 = $(LF2)

# general-purpose stuff
#CP = cp
CP = ln -sf
LN = ln -sf
RM = rm -f
CHMOD = chmod
BINPERMS = 755
MANPERMS = 644
STRIP = strip
E =
O = .o
M = atheos
SHELL = /bin/bash
MAKEF = -f atheos/Makefile

# system dependent headers
OSDEP_H = atheos/athcfg.h

# optional inclusion of bzip2 decompression
IZ_OUR_BZIP2_DIR = bzip2
IZ_BZIP2 = $(IZ_OUR_BZIP2_DIR)
## The following symbols definitions need to be set to activate bzip2 support:
#D_USE_BZ2 = -DUSE_BZIP2
#L_BZ2 = -lbz2
#LIBBZ2 = $(IZ_BZIP2)/libbz2.a

# defaults for unzip's "built-in" bzip2 library compilation
CC_BZ = $(CC)
CFLAGS_BZ = $(CFLAGS)

# object files
OBJS1 = unzip$O crc32$O crc_i386$O crypt$O envargs$O explode$O
OBJS2 = extract$O fileio$O globals$O inflate$O list$O match$O
OBJS3 = process$O ttyio$O ubz2err$O unreduce$O unshrink$O zipinfo$O
OBJS = $(OBJS1) $(OBJS2) $(OBJS3) $M$O
LOBJS = $(OBJS)
OBJX = unzipsfx$O crc32_$O crc_i386$O crypt_$O extract_$O fileio_$O \
	globals_$O inflate_$O match_$O process_$O ttyio_$O ubz2err_$O $M_$O
LOBJX = $(OBJX)
OBJF = funzip$O crc32$O crc_i386$O cryptf$O globalsf$O inflatef$O ttyiof$O
UNZIP_H = unzip.h unzpriv.h globals.h $(OSDEP_H) $(ACONF_DEP)

# installation
INSTALL = install
manext = 1
prefix = /usr
BINDIR = $(prefix)/bin
MANDIR = $(prefix)/man/man$(manext)
INSTALLEDBIN = $(BINDIR)/funzip$E $(BINDIR)/zipinfo$E $(BINDIR)/unzipsfx$E $(BINDIR)/unzip$E
INSTALLEDMAN = $(MANDIR)/unzip.$(manext) $(MANDIR)/funzip.$(manext) $(MANDIR)/unzipsfx.$(manext) $(MANDIR)/zipinfo.$(manext)
#
UNZIPS = unzip$E funzip$E unzipsfx$E zipinfo$E
# this is a little ugly...well, no, it's a lot ugly:
MANS = man/unzip.1 man/unzipsfx.1 man/zipinfo.1 man/funzip.1 man/zipgrep.1

######################################################################
# Basic compile instructions and dependencies

# this is for GNU make; comment out and notify zip-bugs if it causes errors
.SUFFIXES:	.c .o

# default for compiling C files
.c.o:
	$(CC) -c $(CF) $*.c

unzips:		$(UNZIPS)
objs:		$(OBJS)
objsdll:	$(OBJSDLL)
unzipsman:	unzips


unzip$E:	$(OBJS) $(LIBBZ2)	# add `&' for parallel makes
	$(LD) $(LF) -L$(IZ_BZIP2) $(LOBJS) $(L_BZ2) $(LF2)

unzipsfx$E:	$(OBJX)			# add `&' for parallel makes
	$(LD) $(SL) $(LOBJX) $(SL2)

funzip$E:	$(OBJF)			# add `&' for parallel makes
	$(LD) $(FL) $(OBJF) $(FL2)

zipinfo$E:	unzip$E
	$(LN) unzip$E zipinfo$E

# when the optional bzip2 support is provided (as recommended) by sources
# in the 'bzip2' subdirectory, create/update the library:
$(IZ_OUR_BZIP2_DIR)/libbz2.a:
	@echo "Building/updating bzip2 object library..."
	( cd $(IZ_OUR_BZIP2_DIR) ; $(MAKE) -f Makebz2.iz CC="$(CC_BZ)"\
	 CFLAGS="$(CFLAGS_BZ)" RM="rm -f" )


crc32$O:	crc32.c $(UNZIP_H) zip.h crc32.h
crypt$O:	crypt.c $(UNZIP_H) zip.h crypt.h crc32.h ttyio.h
envargs$O:	envargs.c $(UNZIP_H)
explode$O:	explode.c $(UNZIP_H)
extract$O:	extract.c $(UNZIP_H) crc32.h crypt.h
fileio$O:	fileio.c $(UNZIP_H) crc32.h crypt.h ttyio.h ebcdic.h
funzip$O:	funzip.c $(UNZIP_H) crc32.h crypt.h ttyio.h
globals$O:	globals.c $(UNZIP_H)
inflate$O:	inflate.c inflate.h $(UNZIP_H)
list$O:		list.c $(UNZIP_H)
match$O:	match.c $(UNZIP_H)
process$O:	process.c $(UNZIP_H) crc32.h
ttyio$O:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
ubz2err$O:	ubz2err.c $(UNZIP_H)
unreduce$O:	unreduce.c $(UNZIP_H)
unshrink$O:	unshrink.c $(UNZIP_H)
unzip$O:	unzip.c $(UNZIP_H) crypt.h unzvers.h consts.h
zipinfo$O:	zipinfo.c $(UNZIP_H)

# unzipsfx compilation section
unzipsfx$O:	unzip.c $(UNZIP_H) crypt.h unzvers.h consts.h
	$(CC) -c $(CF) -DSFX -o $@ unzip.c

crc32_$O:	crc32.c $(UNZIP_H) zip.h crc32.h
	$(CC) -c $(CF) -DSFX -o $@ crc32.c

crypt_$O:	crypt.c $(UNZIP_H) zip.h crypt.h crc32.h ttyio.h
	$(CC) -c $(CF) -DSFX -o $@ crypt.c

extract_$O:	extract.c $(UNZIP_H) crc32.h crypt.h
	$(CC) -c $(CF) -DSFX -o $@ extract.c

fileio_$O:	fileio.c $(UNZIP_H) crc32.h crypt.h ttyio.h ebcdic.h
	$(CC) -c $(CF) -DSFX -o $@ fileio.c

globals_$O:	globals.c $(UNZIP_H)
	$(CC) -c $(CF) -DSFX -o $@ globals.c

inflate_$O:	inflate.c inflate.h $(UNZIP_H) crypt.h
	$(CC) -c $(CF) -DSFX -o $@ inflate.c

match_$O:	match.c $(UNZIP_H)
	$(CC) -c $(CF) -DSFX -o $@ match.c

process_$O:	process.c $(UNZIP_H) crc32.h
	$(CC) -c $(CF) -DSFX -o $@ process.c

ttyio_$O:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
	$(CC) -c $(CF) -DSFX -o $@ ttyio.c

ubz2err_$O:	ubz2err.c $(UNZIP_H)
	$(CC) -c $(CF) -DSFX -o $@ ubz2err.c


# funzip compilation section
cryptf$O:	crypt.c $(UNZIP_H) zip.h crypt.h crc32.h ttyio.h
	$(CC) -c $(CF) -DFUNZIP -o $@ crypt.c

globalsf$O:	globals.c $(UNZIP_H)
	$(CC) -c $(CF) -DFUNZIP -o $@ globals.c

inflatef$O:	inflate.c inflate.h $(UNZIP_H) crypt.h
	$(CC) -c $(CF) -DFUNZIP -o $@ inflate.c

ttyiof$O:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
	$(CC) -c $(CF) -DFUNZIP -o $@ ttyio.c


# optional assembler replacements
crc_i386$O:	crc_i386.S
	$(CC) -E crc_i386.S > crc_i386s.s
	$(AS) -o $@ crc_i386s.s
	$(RM) crc_i386s.s

# AtheOS specific code
atheos$O:	atheos/atheos.c $(UNZIP_H) unzvers.h atheos/atheos.h
	$(CC) -c $(CF) atheos/atheos.c

# version() not used by unzipsfx, so no unzvers.h dependency
atheos_$O:	atheos/atheos.c $(UNZIP_H) atheos/atheos.h	# unzipsfx only
	$(CP) atheos/atheos.c atheos_.c
	$(CC) -c $(CF) -Iatheos -DSFX atheos_.c
	$(RM) atheos_.c

# this really only works for Unix targets, unless E and O specified on cmd line
clean:
	$(RM) $(UNZIPS) $(OBJS) $(OBJF) $(OBJX) api$O apihelp$O crc_gcc$O unzipstb$O

install:	unzips
	$(INSTALL) -m 755 $(UNZIPS) $(BINDIR)
	$(RM) $(BINDIR)/zipinfo$E
	$(LN) unzip$E $(BINDIR)/zipinfo$E
	$(RM) $(BINDIR)/zipgrep$E
	$(INSTALL) -m 755 unix/zipgrep $(BINDIR)/zipgrep$E
	$(INSTALL) -m 644 man/unzip.1 $(MANDIR)/unzip.$(manext)
	$(INSTALL) -m 644 man/unzipsfx.1 $(MANDIR)/unzipsfx.$(manext)
	$(INSTALL) -m 644 man/zipinfo.1 $(MANDIR)/zipinfo.$(manext)
	$(INSTALL) -m 644 man/funzip.1 $(MANDIR)/funzip.$(manext)
	$(INSTALL) -m 644 man/zipgrep.1 $(MANDIR)/zipgrep.$(manext)

# alternatively, could use zip method:  -cd $(BINDIR); $(RM) $(UNZIPS)  [etc.]
uninstall:
	$(RM) $(INSTALLEDBIN) $(INSTALLEDMAN)


TESTZIP = testmake.zip	# the test zipfile

# test some basic features of the build
test:		check

check:
	@echo '#####  This is a Unix-specific target.  (Just so you know.)'
	@echo '#####     Make sure unzip, funzip and unzipsfx are compiled and'
	@echo '#####     in this directory.'
	@if test ! -f ./unzip; then \
	    echo "#####  ERROR:  can't find ./unzip"; exit 1; fi
	@if test ! -f ./funzip; then \
	    echo "#####  ERROR:  can't find ./funzip"; exit 1; fi
	@if test ! -f ./unzipsfx; then \
	    echo "#####  ERROR:  can't find ./unzipsfx"; exit 1; fi
#
	@if test ! -f $(TESTZIP); then \
	    echo "#####  ERROR:  can't find test file $(TESTZIP)"; exit 1; fi
#
	@echo "#####  testing extraction"
	@./unzip -bo $(TESTZIP) testmake.zipinfo
	@if test ! -f testmake.zipinfo ; then \
	    echo "#####  ERROR:  file extraction from $(TESTZIP) failed"; \
	    exit 1; fi
#
	@echo '#####  testing zipinfo (unzip -Z)'
	@./unzip -Z $(TESTZIP) > testmake.unzip-Z
	@if diff testmake.unzip-Z testmake.zipinfo; then echo "OK."; else \
	    echo "#####  WARNING:  zipinfo output doesn't match stored version"; \
	    echo '#####     (If the only difference is the file times, compare your'; \
	    echo '#####      timezone with the Central European timezone, which is one'; \
	    echo '#####      hour east of Greenwich but effectively 2 hours east'; \
	    echo '#####      during summer Daylight Savings Time.  The upper two'; \
	    echo '#####      lines should correspond to your local time when the'; \
	    echo '#####      files were created, on 19 November 1998 at 10:46pm CET.'; \
	    echo '#####      If the times are consistent, please ignore this warning.)'; \
	    fi
	@$(RM) testmake.unzip-Z testmake.zipinfo
#
	@echo '#####  testing unzip -d exdir option'
	@./unzip -bo $(TESTZIP) -d testun notes
	@cat testun/notes
#
	@echo '#####  testing unzip -o and funzip (ignore funzip warning)'
	@./unzip -boq $(TESTZIP) notes -d testun
	@./funzip < $(TESTZIP) > testun/notes2
	@if diff testun/notes testun/notes2; then true; else \
	    echo '#####  ERROR:  funzip output disagrees with unzip'; fi
#
	@echo '#####  testing unzipsfx (self-extractor)'
	@cat unzipsfx $(TESTZIP) > testsfx
	@$(CHMOD) 0700 testsfx
	@./testsfx -bo notes
	@if diff notes testun/notes; then true; else \
	    echo '#####  ERROR:  unzipsfx file disagrees with unzip'; fi
	@$(RM) testsfx notes testun/notes testun/notes2
	@rmdir testun
#
	@echo '#####  testing complete.'
