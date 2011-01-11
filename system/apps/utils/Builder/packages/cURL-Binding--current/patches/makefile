EXT = cURL-binding

#I = path/to/your/rebol3/hostkit/src/include
I = src/include

# POSIX (Linux, Syllable, ...)
#CC = gcc
LIB = $(EXT).so
#CFLAGS += -DTO_LINUX -I $I $(shell curl-config --cflags)
CFLAGS += -I $I $(shell curl-config --cflags)
LDFLAGS += -shared $(shell curl-config --libs)

# Windows (MinGW)
#CURL = path/to/your/curl/development/installation
#CC = gcc
#LIB = $(EXT).dll
#CFLAGS += -DTO_WIN32 -I $I -I $CURL/include
#LDFLAGS += -mdll -lcurl -L $CURL/bin -L $CURL/lib

all: $(EXT).c $I/* makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(LIB) $(EXT).c
	strip $(LIB)
