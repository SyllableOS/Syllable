CFLAGS   = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer
CPPFLAGS = $(CFLAGS)
AOPTS   = -c

APPBIN = /Applications/Preferences

VPATH = ./

CC	= gcc
LL	= ld

OBJS = main.o pluginwindow.o mainwindow.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/Prefs-Media

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/Prefs-Media: $(OBJS)
	gcc  $(OBJS) -lstdc++ -latheos -lmedia -o Prefs-Media
	
deps:

install:  $(OBJDIR)/Prefs-Media
	mkdir -p $(APPBIN)
	install -s Prefs-Media $(APPBIN)/Prefs-Media

dist:  $(OBJDIR)/Prefs-Media
	mkdir -p $(DIST_DIR)/atheos/Applications/Preferences
	install -s Prefs-Media $(DIST_DIR)/atheos/Applications/Preferences/Prefs-Media

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d Prefs-Media

-include $(OBJDIR)/*.d

































