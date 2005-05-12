COPTS = -c

OBJDIR = objs/

OBJS = $(OBJDIR)main.o $(OBJDIR)appwindow.o $(OBJDIR)iview.o $(OBJDIR)statusbar.o $(OBJDIR)toolbar.o
EXE = AView

all: $(OBJDIR) $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) *.png

$(OBJDIR):
	mkdir -p $(OBJDIR)

install: $(EXE)
	-mkdir -p /Applications/AView
	-cp -pf $(EXE) /Applications/AView/
	-ln -sf /Applications/AView/$(EXE) /system/bin/aview

dist: $(EXE)
	-mkdir -p $(DIST_DIR)/atheos/Applications/AView
	-cp -pf $(EXE) $(DIST_DIR)/atheos/Applications/AView/
	-ln -sf /Applications/AView/$(EXE) $(DIST_DIR)/atheos/sys/bin/aview

clean:
	-rm $(OBJS)
	-rm $(EXE)

$(OBJDIR)main.o: main.cpp
	$(CXX) $(COPTS) main.cpp -o $@

$(OBJDIR)appwindow.o: appwindow.cpp appwindow.h
	$(CXX) $(COPTS) appwindow.cpp -o $@

$(OBJDIR)iview.o: iview.cpp iview.h
	$(CXX) $(COPTS) iview.cpp -o $@

$(OBJDIR)statusbar.o: statusbar.cpp statusbar.h
	$(CXX) $(COPTS) statusbar.cpp -o $@

$(OBJDIR)toolbar.o: toolbar.cpp toolbar.h
	$(CXX) $(COPTS) toolbar.cpp -o $@

