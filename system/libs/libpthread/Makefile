CFLAGS := -Wall -c -O2

OBJDIR := objs

OBJS := attr.o cond.o mutex.o pthread.o rwlock.o bits.o

include makefile.rules

VER := 0
LIB := libpthread.so.$(VER)
SON := libpthread.so

all: $(OBJDIR) $(OBJS)
	$(CC) -shared -Xlinker "-soname=$(SON)" $(OBJS) -o $(OBJDIR)/$(LIB)

clean:
	-rm $(OBJS)
	-rm $(OBJDIR)/$(LIB)

install:
	-cp -f $(OBJDIR)/$(LIB) /system/libs/
	if [ ! -e /system/libs/$(SON) ];then \
		ln -s /system/libs/$(LIB) /system/libs/$(SON); \
	fi

$(OBJDIR):
	-mkdir $(OBJDIR)

