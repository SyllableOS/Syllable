VERSION_MAJOR = 2
VERSION_MINOR = 0
LOBJS = objs/inneredit.o objs/codeview.o objs/format_c.o objs/format_java.o objs/format_perl.o objs/format_html.o objs/format_ruby.o
OBJS = objs/main.o
COPTS = -Wall -fPIC
EXE = testapp
LIB = libcodeview.so
INSTALLDIR = /system/libraries/


all: $(LIB)

$(EXE): $(OBJS) $(LOBJS)
	$(CXX) $(OBJS) $(LOBJS) -lsyllable -o $(EXE)

#$(EXE): $(OBJS) $(LIB)
#	$(CXX) $(OBJS) -lsyllable -L. -lcodeview -o $(EXE)


$(LIB): $(LOBJS)
	$(CXX) $(LOBJS) -lsyllable -shared -g -Wl,-soname,$(LIB).$(VERSION_MAJOR) -o $(LIB)


.PHONY : install
install: all
	cp -f $(LIB) $(INSTALLDIR)$(LIB).$(VERSION_MAJOR).$(VERSION_MINOR)
	ln -fs $(INSTALLDIR)$(LIB).$(VERSION_MAJOR).$(VERSION_MINOR) \
	       $(INSTALLDIR)$(LIB).$(VERSION_MAJOR)
	ln -fs $(INSTALLDIR)$(LIB).$(VERSION_MAJOR) \
	       $(INSTALLDIR)$(LIB)
	mkdir -p /system/development/headers/codeview
	cp -f codeview.h /system/development/headers/codeview/
	cp -f format.h /system/development/headers/codeview/
	cp -f format_c.h /system/development/headers/codeview/
	cp -f format_java.h /system/development/headers/codeview/
	cp -f format_perl.h /system/development/headers/codeview/
	cp -f format_html.h /system/development/headers/codeview/
	cp -f format_ruby.h /system/development/headers/codeview/

.PHONY : docs
docs:
	mkdir -p docs
	doxytag -t docs/atheos.gui.tag /boot/Documentation/gui/
	doxygen doxygen.conf


.PHONY : clean
clean:
	-rm -f $(OBJS)
	-rm -f $(LOBJS)
	-rm -f $(EXE)
	-rm -f $(LIB)
	-rmdir objs


objs/%.o: %.cpp
	@mkdir -p objs
	$(CXX) $< $(COPTS) -c -o $@

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
objs/format_ruby: format_ruby.cpp format_ruby.h format.h

