CC = g++
LL = g++

COPTS += -Wall -Wno-deprecated -c -DENABLE_UNDO

OBJSDIR = objs/

OBJS = $(OBJSDIR)main.o $(OBJSDIR)appwindow.o $(OBJSDIR)button_bar.o $(OBJSDIR)editview.o $(OBJSDIR)aboutdialog.o $(OBJSDIR)gotodialog.o $(OBJSDIR)finddialog.o $(OBJSDIR)replacedialog.o $(OBJSDIR)buffer.o $(OBJSDIR)status_bar.o $(OBJSDIR)mytabview.o $(OBJSDIR)icons.o
EXE = aedit

all: $(OBJSDIR) $(EXE)

$(OBJSDIR):
	mkdir $(OBJSDIR)

$(EXE): $(OBJS)
	$(LL) $(OBJS) -lsyllable -o $(EXE)
	rescopy -r $(EXE) resources/aedit.catalog
	rescopy -a $(EXE) sv/aedit.catalog=resources/sv/aedit.catalog
	rescopy -a $(EXE) de/aedit.catalog=resources/de/aedit.catalog
	rescopy -a $(EXE) nl/aedit.catalog=resources/nl/aedit.catalog
	rescopy -a $(EXE) it/aedit.catalog=resources/it/aedit.catalog
	rescopy -a $(EXE) es/aedit.catalog=resources/es/aedit.catalog
	rescopy -a $(EXE) fr/aedit.catalog=resources/fr/aedit.catalog
	rescopy -a $(EXE) da/aedit.catalog=resources/da/aedit.catalog
	rescopy -a $(EXE) stock_copy_16=pixmaps/stock_copy_16.png
	rescopy -a $(EXE) stock_copy_24=pixmaps/stock_copy_24.png
	rescopy -a $(EXE) stock_cut_16=pixmaps/stock_cut_16.png
	rescopy -a $(EXE) stock_cut_24=pixmaps/stock_cut_24.png
	rescopy -a $(EXE) stock_exit_16=pixmaps/stock_exit_16.png
	rescopy -a $(EXE) stock_exit_24=pixmaps/stock_exit_24.png
	rescopy -a $(EXE) stock_jump_to_16=pixmaps/stock_jump_to_16.png
	rescopy -a $(EXE) stock_jump_to_24=pixmaps/stock_jump_to_24.png
	rescopy -a $(EXE) stock_left_arrow_16=pixmaps/stock_left_arrow_16.png
	rescopy -a $(EXE) stock_left_arrow_24=pixmaps/stock_left_arrow_24.png
	rescopy -a $(EXE) stock_new_16=pixmaps/stock_new_16.png
	rescopy -a $(EXE) stock_new_24=pixmaps/stock_new_24.png
	rescopy -a $(EXE) stock_open_16=pixmaps/stock_open_16.png
	rescopy -a $(EXE) stock_open_24=pixmaps/stock_open_24.png
	rescopy -a $(EXE) stock_paste_16=pixmaps/stock_paste_16.png
	rescopy -a $(EXE) stock_paste_24=pixmaps/stock_paste_24.png
	rescopy -a $(EXE) stock_redo_16=pixmaps/stock_redo_16.png
	rescopy -a $(EXE) stock_redo_24=pixmaps/stock_redo_24.png
	rescopy -a $(EXE) stock_right_arrow_16=pixmaps/stock_right_arrow_16.png
	rescopy -a $(EXE) stock_right_arrow_24=pixmaps/stock_right_arrow_24.png
	rescopy -a $(EXE) stock_save_16=pixmaps/stock_save_16.png
	rescopy -a $(EXE) stock_save_24=pixmaps/stock_save_24.png
	rescopy -a $(EXE) stock_save_as_16=pixmaps/stock_save_as_16.png
	rescopy -a $(EXE) stock_save_as_24=pixmaps/stock_save_as_24.png
	rescopy -a $(EXE) stock_search_16=pixmaps/stock_search_16.png
	rescopy -a $(EXE) stock_search_24=pixmaps/stock_search_24.png
	rescopy -a $(EXE) stock_search_replace_16=pixmaps/stock_search_replace_16.png
	rescopy -a $(EXE) stock_search_replace_24=pixmaps/stock_search_replace_24.png
	rescopy -a $(EXE) stock_undo_16=pixmaps/stock_undo_16.png
	rescopy -a $(EXE) stock_undo_24=pixmaps/stock_undo_24.png
	rescopy -a $(EXE) icon24x24.png=pixmaps/icon24x24.png
	rescopy -a $(EXE) icon48x48.png=pixmaps/icon48x48.png
	rescopy -a $(EXE) about=pixmaps/about.png

deps:

install: all
	strip --strip-all $(EXE)
	mkdir -p /Applications/AEdit
	cp $(EXE) /Applications/AEdit/AEdit
	ln -sf /Applications/AEdit/AEdit /system/bin/$(EXE)

dist: all
	strip --strip-all $(EXE)
	install -s $(EXE) $(DIST_DIR)/atheos/Applications/AEdit/AEdit

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

$(OBJSDIR)aboutdialog.o: aboutdialog.cpp aboutdialog.h
	$(CC) $(COPTS) aboutdialog.cpp -o $@

$(OBJSDIR)gotodialog.o: gotodialog.cpp gotodialog.h
	$(CC) $(COPTS) gotodialog.cpp -o $@

$(OBJSDIR)finddialog.o: finddialog.cpp finddialog.h
	$(CC) $(COPTS) finddialog.cpp -o $@

$(OBJSDIR)replacedialog.o: replacedialog.cpp replacedialog.cpp
	$(CC) $(COPTS) replacedialog.cpp -o $@

$(OBJSDIR)buffer.o: buffer.cpp buffer.h
	$(CC) $(COPTS) buffer.cpp -o $@

$(OBJSDIR)status_bar.o: status_bar.cpp status_bar.h
	$(CC) $(COPTS) status_bar.cpp -o $@

$(OBJSDIR)mytabview.o: mytabview.cpp mytabview.h
	$(CC) $(COPTS) mytabview.cpp -o $@

$(OBJSDIR)icons.o: icons.cpp icons.h
	$(CC) $(COPTS) icons.cpp -o $@

