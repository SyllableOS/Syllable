CC = g++
LL = g++

COPTS = -Wall -c -O3 -DENABLE_UNDO

OBJSDIR = objs/

OBJS = $(OBJSDIR)main.o $(OBJSDIR)appwindow.o $(OBJSDIR)image_button.o $(OBJSDIR)button_bar.o $(OBJSDIR)editview.o $(OBJSDIR)gotodialog.o $(OBJSDIR)finddialog.o $(OBJSDIR)replacedialog.o $(OBJSDIR)settings.o
EXE = aedit

$(EXE): $(OBJS)
	$(LL) $(OBJS) -latheos -o $(EXE)

deps:

install: $(EXE)
	-strip --strip-all $(EXE)
	-mkdir /usr/aedit
	-mkdir /usr/aedit/bin
	-cp $(EXE) /usr/aedit/bin/

dist: $(EXE)
	-strip --strip-all $(EXE)
	-cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

$(OBJSDIR)main.o: main.cpp main.h
	$(CC) $(COPTS) main.cpp -o $@

$(OBJSDIR)appwindow.o: appwindow.cpp appwindow.h
	$(CC) $(COPTS) appwindow.cpp -o $@

$(OBJSDIR)image_button.o: image_button.cpp image_button.h
	$(CC) $(COPTS) image_button.cpp -o $@

$(OBJSDIR)button_bar.o: button_bar.cpp button_bar.h
	$(CC) $(COPTS) button_bar.cpp -o $@

$(OBJSDIR)editview.o: editview.cpp editview.h
	$(CC) $(COPTS) editview.cpp -o $@

$(OBJSDIR)gotodialog.o: gotodialog.cpp gotodialog.h
	$(CC) $(COPTS) gotodialog.cpp -o $@

$(OBJSDIR)finddialog.o: finddialog.cpp finddialog.h
	$(CC) $(COPTS) finddialog.cpp -o $@

$(OBJSDIR)replacedialog.o: replacedialog.cpp replacedialog.cpp
	$(CC) $(COPTS) replacedialog.cpp -o $@

$(OBJSDIR)settings.o: settings.cpp settings.h
	$(CC) $(COPTS) settings.cpp -o $@


