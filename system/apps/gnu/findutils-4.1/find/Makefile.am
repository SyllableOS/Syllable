PROGRAMS = find
find_SOURCES = find.c fstype.c parser.c pred.c tree.c util.c version.c
DIST_OTHER = defs.h
INCLUDES = -I.. -I$(top_srcdir)/lib
LDADD = ../lib/libfind.a
MANS = find.1
CONFIG_HEADER = ../config.h

$(PROGRAMS): ../lib/libfind.a

parser.o: ../lib/modechange.h
find.o fstype.o parser.o pred.o: ../lib/modetype.h
find.o fstype.o parser.o pred.o tree.o util.o: defs.h 
pred.o: ../lib/wait.h
