COPTS = -Wall -c -O2

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

clean:
	-rm $(OBJS)
	-rm $(EXE)

$(OBJDIR)main.o: main.cpp
	$(CC) $(COPTS) main.cpp -o $@

$(OBJDIR)appwindow.o: appwindow.cpp appwindow.h
	$(CC) $(COPTS) appwindow.cpp -o $@

$(OBJDIR)iview.o: iview.cpp iview.h
	$(CC) $(COPTS) iview.cpp -o $@

$(OBJDIR)statusbar.o: statusbar.cpp statusbar.h
	$(CC) $(COPTS) statusbar.cpp -o $@

$(OBJDIR)toolbar.o: toolbar.cpp toolbar.h
	$(CC) $(COPTS) toolbar.cpp -o $@

