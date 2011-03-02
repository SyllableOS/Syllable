EXT = ZeroMQ-binding

#I = path/to/your/rebol3/hostkit/src/include
I = src/include

# POSIX (Linux, Syllable, ...)
#CC = gcc
LIB = $(EXT).so
#CFLAGS += -DTO_LINUX -I $I $(shell pkg-config libzmq --cflags)
CFLAGS += -I $I $(shell pkg-config libzmq --cflags)
LDFLAGS += -shared -lzmq $(shell pkg-config libzmq --libs)

# Windows (MinGW)
#ZeroMQ = path/to/your/0mq/development/installation
#CC = gcc
#LIB = $(EXT).dll
#CFLAGS += -DTO_WIN32 -I $I -I $ZeroMQ/include
#LDFLAGS += -mdll -lzmq -L $ZeroMQ/bin -L $ZeroMQ/lib -luuid -lpthread

all: $(EXT).c $I/* makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(LIB) $(EXT).c
	strip $(LIB)
