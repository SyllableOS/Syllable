PROGRAMS = xargs
MANS = xargs.1
INCLUDES = -I.. -I$(top_srcdir)/lib
LDADD = ../find/version.o ../lib/libfind.a
CONFIG_HEADER = ../config.h

$(PROGRAMS): ../find/version.o ../lib/libfind.a

xargs.o: ../lib/wait.h
