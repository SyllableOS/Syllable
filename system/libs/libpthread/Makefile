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
	cp -f $(OBJDIR)/$(LIB) /system/libs/
	if [ ! -e /system/libs/libpthread.so ];then \
		ln -s /system/libs/$(LIB) /system/libs/libpthread.so; \
	fi

dist: all
	cp -f $(OBJDIR)/$(LIB) $(IMAGE)/system/libs/
	if [ ! -e $(IMAGE)/system/libs/libpthread.so ];then \
		ln -s $(LIB) $(IMAGE)/system/libs/libpthread.so; \
	fi

$(OBJDIR):
	-mkdir $(OBJDIR)

