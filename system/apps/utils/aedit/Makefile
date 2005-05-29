COPTS = -c -DENABLE_UNDO

OBJSDIR = objs/

OBJS = $(OBJSDIR)main.o $(OBJSDIR)appwindow.o $(OBJSDIR)button_bar.o $(OBJSDIR)editview.o $(OBJSDIR)gotodialog.o $(OBJSDIR)finddialog.o $(OBJSDIR)replacedialog.o $(OBJSDIR)settings.o
EXE = aedit

all: $(OBJSDIR) $(EXE)

$(OBJSDIR):
	mkdir $(OBJSDIR)

$(EXE): $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) icon24x24.png icon48x48.png
	rescopy -a $(EXE) resources/aedit.catalog
	rescopy -a $(EXE) sv/aedit.catalog=resources/sv/aedit.catalog
	rescopy -a $(EXE) de/aedit.catalog=resources/de/aedit.catalog
	rescopy -a $(EXE) nl/aedit.catalog=resources/nl/aedit.catalog
	rescopy -a $(EXE) it/aedit.catalog=resources/it/aedit.catalog
	rescopy -a $(EXE) es/aedit.catalog=resources/es/aedit.catalog
	rescopy -a $(EXE) fr/aedit.catalog=resources/fr/aedit.catalog
	rescopy -a $(EXE) da/aedit.catalog=resources/da/aedit.catalog
	rescopy -a $(EXE) icon-copy=resources/aedit-icons/icon-copy.png
	rescopy -a $(EXE) icon-cut=resources/aedit-icons/icon-cut.png
	rescopy -a $(EXE) icon-paste=resources/aedit-icons/icon-paste.png
	rescopy -a $(EXE) icon-new=resources/aedit-icons/icon-new.png
	rescopy -a $(EXE) icon-open=resources/aedit-icons/icon-open.png
	rescopy -a $(EXE) icon-save=resources/aedit-icons/icon-save.png
	rescopy -a $(EXE) icon-redo=resources/aedit-icons/icon-redo.png
	rescopy -a $(EXE) icon-undo=resources/aedit-icons/icon-undo.png
	rescopy -a $(EXE) icon-find=resources/aedit-icons/icon-find.png
	rescopy -a $(EXE) fy/aedit.catalog=resources/fy/aedit.catalog

deps:

install: all
	mkdir -p /Applications/AEdit
	cp -pf $(EXE) /Applications/AEdit/AEdit
	ln -sf /Applications/AEdit/AEdit /system/bin/aedit

dist: all
	mkdir $(DIST_DIR)/atheos/Applications/AEdit
	install -s $(EXE) $(DIST_DIR)/atheos/Applications/AEdit/AEdit
	ln -sf /Applications/AEdit/AEdit $(DIST_DIR)/atheos/sys/bin/aedit

clean:
	-rm $(OBJS)
	-rm $(EXE)

$(OBJSDIR)main.o: main.cpp main.h
	$(CXX) $(COPTS) main.cpp -o $@

$(OBJSDIR)appwindow.o: appwindow.cpp appwindow.h
	$(CXX) $(COPTS) appwindow.cpp -o $@

$(OBJSDIR)button_bar.o: button_bar.cpp button_bar.h
	$(CXX) $(COPTS) button_bar.cpp -o $@

$(OBJSDIR)editview.o: editview.cpp editview.h
	$(CXX) $(COPTS) editview.cpp -o $@

$(OBJSDIR)gotodialog.o: gotodialog.cpp gotodialog.h
	$(CXX) $(COPTS) gotodialog.cpp -o $@

$(OBJSDIR)finddialog.o: finddialog.cpp finddialog.h
	$(CXX) $(COPTS) finddialog.cpp -o $@

$(OBJSDIR)replacedialog.o: replacedialog.cpp replacedialog.cpp
	$(CXX) $(COPTS) replacedialog.cpp -o $@

$(OBJSDIR)settings.o: settings.cpp settings.h
	$(CXX) $(COPTS) settings.cpp -o $@



