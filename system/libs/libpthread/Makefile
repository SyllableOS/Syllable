CFLAGS := -c

OBJDIR := objs

OBJS := attr.o cond.o mutex.o pthread.o rwlock.o bits.o

include makefile.rules

VER := 1
LIB := libpthread.so.$(VER)

all: $(OBJDIR) $(OBJS)
	$(CC) -shared -Xlinker "-soname=$(LIB)" $(OBJS) -o $(OBJDIR)/$(LIB)

clean:
	-rm $(OBJS)
	-rm $(OBJDIR)/$(LIB)

install: all
	cp -f $(OBJDIR)/$(LIB) $(IMAGE)/system/libraries/
	ln -s $(LIB) $(IMAGE)/system/libraries/libpthread.so

dist: install

$(OBJDIR):
	-mkdir $(OBJDIR)

