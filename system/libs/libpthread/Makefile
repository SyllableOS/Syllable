CFLAGS := -c -Iinc/

OBJDIR := objs

OBJS := attr.o cond.o mutex.o pthread.o rwlock.o bits.o

#DEBUG := y
ifdef DEBUG
 CFLAGS += -g -D__ENABLE_DEBUG__
 OBJS += debug.o
endif

include makefile.rules

VER := 2
LIB := libpthread.so.$(VER)

all: $(OBJDIR) $(OBJS)
	$(CC) -shared -Xlinker "-soname=$(LIB)" $(OBJS) -o $(OBJDIR)/$(LIB)

clean:
	-rm $(OBJS)
	-rm $(OBJDIR)/$(LIB)

install: all
	cp -f $(OBJDIR)/$(LIB) $(IMAGE)/system/libraries/
	ln -sf $(LIB) $(IMAGE)/system/libraries/libpthread.so

dist: install

$(OBJDIR):
	-mkdir $(OBJDIR)

