EXT = hello-extension

#I = path/to/your/rebol3/hostkit/src/include
I = src/include

# POSIX (Linux, Syllable, ...)
#CC = gcc
LIB = $(EXT).so
#CFLAGS += -DTO_LINUX -I $I
CFLAGS += -I $I
LDFLAGS += -shared -Xlinker "-soname=$(LIB)"

# Windows (MinGW)
#CC = gcc
#LIB = $(EXT).dll
#CFLAGS += -DTO_WIN32 -I $I
#LDFLAGS += -mdll

all: $(EXT).c $I/* makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(LIB) $(EXT).c
	strip $(LIB)
