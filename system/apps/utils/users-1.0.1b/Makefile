ifeq ($(DEBUG),false)
CFLAGS  += -DDEBUG=false
else
CFLAGS  += -g -DDEBUG=true
endif

LIBS    := -lsyllable -lz -lcrypt
LDFLAGS := $(LIBS)

CXXSRC := main.cpp userswindow.cpp labelview.cpp \
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

$(EXENAME): $(OBJS) Makefile
	@echo Building : $(EXENAME)
	@$(CXX) $(OBJS) $(LDFLAGS) -o $(EXENAME)
	@echo Adding images into : $(EXENAME)
	@rescopy -r $(EXENAME) keys.png
su:	$(EXENAME)
	su root -c "sh -c 'chown root:root $(EXENAME); chmod 4755 $(EXENAME)'"

dist: all
	mkdir $(DIST_DIR)/atheos/Applications/Users
	install -s $(EXENAME) $(DIST_DIR)/atheos/Applications/Users/

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

