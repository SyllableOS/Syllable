VERSION_MAJOR = 1
VERSION_MINOR = 0
LOBJS = objs/inneredit.o objs/codeview.o objs/format_c.o objs/format_java.o objs/format_perl.o objs/format_html.o
OBJS = objs/main.o
CC = g++
COPTS = -Wall -fPIC
LL = g++
EXE = testapp
LIB = libcodeview.so
INSTALLDIR = /system/libs/


$(EXE): $(OBJS) $(LOBJS)
	$(LL) $(OBJS) $(LOBJS) -latheos -o $(EXE)

#$(EXE): $(OBJS) lib
#	$(LL) $(OBJS) -latheos -L. -lcodeview -o $(EXE)


lib: $(LOBJS)
	$(LL) $(LOBJS) -latheos -shared -Wl,-soname,$(LIB).$(VERSION_MAJOR) -o $(LIB)


.PHONY : install
install: lib

	-cp -f $(LIB) $(INSTALLDIR)$(LIB).$(VERSION_MAJOR).$(VERSION_MINOR)
	-ln -fs $(INSTALLDIR)$(LIB).$(VERSION_MAJOR).$(VERSION_MINOR) \
	       $(INSTALLDIR)$(LIB).$(VERSION_MAJOR)
	-ln -fs $(INSTALLDIR)$(LIB).$(VERSION_MAJOR) \
	       $(INSTALLDIR)$(LIB)

	-mkdir -p /ainc/codeview
	-cp -f codeview.h /ainc/codeview/
	-cp -f format.h /ainc/codeview/
	-cp -f format_c.h /ainc/codeview/
	-cp -f format_java.h /ainc/codeview/
	-cp -f format_perl.h /ainc/codeview/
	-cp -f format_html.h /ainc/codeview/

.PHONY : docs
docs:
	-mkdir -p docs
	-doxytag -t docs/atheos.gui.tag /boot/atheos/Documentation/gui/
	-doxygen doxygen.conf


.PHONY : clean
clean:
	-rm -f $(OBJS)
	-rm -f $(LOBJS)
	-rm -f $(EXE)
	-rm -f $(LIB)
	-rmdir objs


objs/%.o: %.cpp
	@mkdir -p objs
	$(CC) $< $(COPTS) -c -o $@

objs:
	mkdir objs

objs/main.o: main.cpp codeview.h

objs/inneredit.o: inneredit.cpp inneredit.h format.h
objs/codeview.o:  codeview.cpp codeview.h inneredit.h
objs/format_c.o:  format_c.cpp format_c.h format.h
objs/format_perl.o:  format_perl.cpp format_perl.h format.h
objs/format_html.o:  format_html.cpp format_html.h format.h
objs/format_java.o:  format_c.cpp format_c.h format_java.cpp \
	format_java.h format.h


