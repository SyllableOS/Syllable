EXT = hello-extension

#CC = gcc
I = src/include

LIB = $(EXT).so
CFLAGS += -I $I
LDFLAGS += -shared -Xlinker "-soname=$(LIB)"

# Windows (MinGW):
#LIB = $(EXT).dll
#CFLAGS += -DTO_WIN32 -I $I
#LDFLAGS += -mdll

all: $(EXT).c $I/* makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(LIB) $(EXT).c
	strip $(LIB)
