LIBRARIES = find
find_SOURCES = regex.c dirname.c error.c filemode.c getopt.c getopt1.c \
idcache.c listfile.c modechange.c nextelem.c savedir.c xmalloc.c getline.c \
xstrdup.c xgetcwd.c fnmatch.c $(find_OPT_SOURCES)

find_OPT_SOURCES = fileblocks.c memcmp.c memset.c mktime.c stpcpy.c strdup.c \
strftime.c strspn.c strstr.c strtol.c alloca.c

DIST_OTHER = fnmatch.h getopt.h modechange.h modetype.h pathmax.h \
regex.h wait.h

CONFIG_HEADER = ../config.h
INCLUDES = -I.. -I$(srcdir)

fnmatch.o: fnmatch.h
getopt1.o: getopt.h
listfile.o xgetcwd.o: pathmax.h
modechange.o: modechange.h
regex.o: regex.h
