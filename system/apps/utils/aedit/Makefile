CC = g++
LL = g++

COPTS = -Wall -c -O3 -DENABLE_UNDO

OBJSDIR = objs/

OBJS = $(OBJSDIR)main.o $(OBJSDIR)appwindow.o $(OBJSDIR)button_bar.o $(OBJSDIR)editview.o $(OBJSDIR)gotodialog.o $(OBJSDIR)finddialog.o $(OBJSDIR)replacedialog.o $(OBJSDIR)settings.o
EXE = aedit

all: $(OBJSDIR) $(EXE)

$(OBJSDIR):
	mkdir $(OBJSDIR)

$(EXE): $(OBJS)
	$(LL) $(OBJS) -lsyllable -o $(EXE)

deps:

install: all
	-strip --strip-all $(EXE)
	-cp $(EXE) /system/bin/

clean:
	-rm $(OBJS)
	-rm $(EXE)

$(OBJSDIR)main.o: main.cpp main.h
	$(CC) $(COPTS) main.cpp -o $@

$(OBJSDIR)appwindow.o: appwindow.cpp appwindow.h
	$(CC) $(COPTS) appwindow.cpp -o $@

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



