CFLAGS  := 
LIBS    := -latheos -lstdc++ -lpng -lz -lcrypt
LDFLAGS := $(LIBS)

ifeq ($(DEBUG),false)
CFLAGS  := $(CFLAGS) -DDEBUG=false
else
CFLAGS  := $(CFLAGS) -g -DDEBUG=true
endif

CXXSRC := main.cpp userswindow.cpp labelview.cpp bitmapview.cpp \
          userlistview.cpp changepwddlg.cpp propertiesdlg.cpp

OBJS := $(patsubst %.cpp,%.o,$(CXXSRC))

EXENAME := Users

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@


all :	objs $(EXENAME)

objs:
	mkdir objs
dist:
	mkdir $(DIST_DIR)/Applications/Users
	mkdir $(DIST_DIR)/Applications/Users/share
	cp -f Users $(DIST_DIR)/Applications/Users
	cp -f keys.png $(DIST_DIR)/Applications/Users/share

deps:
	mkdir objs

$(EXENAME): $(OBJS) Makefile
	$(CXX) $(OBJS) $(LDFLAGS) -o $(EXENAME)

su:	$(EXENAME)
	su root -c "sh -c 'chown root:root $(EXENAME); chmod 4755 $(EXENAME)'"

clean:
	rm -rf $(OBJDIR) $(EXENAME)

clobber: clean
	rm -f *~ *.d


# Implicit rule for dependencies
%.d: %.cpp
	set -e; \
	$(CPP) -MM $(CPPFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/$(subst /,\/,$(OBJDIR))\/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@

include $(CXXSRC:.cpp=.d)
