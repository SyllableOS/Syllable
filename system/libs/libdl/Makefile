CFLAGS := -Wall -c -O2 -I.

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
	ln -sf /system/libs/$(LIB) libdl.so
	cp -f dlfcn.h /ainc/

$(OBJDIR):
	-mkdir $(OBJDIR)

