CFLAGS := -c -I.

OBJDIR := objs

OBJS := dlfcn.o
OBJS := $(addprefix $(OBJDIR)/,$(OBJS))

$(OBJDIR)/%.o : %.c
	@echo Compiling : $<
	@$(CC) $(CFLAGS) $< -o $@

VER := 3
LIB := libdl.so.$(VER)

all: $(OBJDIR) $(OBJS)
	$(CC) -shared -Xlinker "-soname=$(LIB)" $(OBJS) -o $(OBJDIR)/$(LIB)

clean:
	-rm $(OBJS)
	-rm $(OBJDIR)/$(LIB)

install:
	cp -f $(OBJDIR)/$(LIB) /system/libs/
	ln -sf $(LIB) /system/libs/libdl.so
	cp -f dlfcn.h /system/include/

dist:
	cp -f $(OBJDIR)/$(LIB) $(IMAGE)/system/libs/
	ln -sf $(LIB) $(IMAGE)/system/libs/libdl.so
	cp -f dlfcn.h $(IMAGE)/system/include/

$(OBJDIR):
	-mkdir $(OBJDIR)

