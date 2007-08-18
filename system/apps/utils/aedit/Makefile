COPTS = -Wno-deprecated -c

OBJSDIR = objs/

CATALOGS = bg da de el es fr fy hu it nl pl pt-br ro ru sv

OBJS = $(OBJSDIR)main.o $(OBJSDIR)appwindow.o $(OBJSDIR)editview.o $(OBJSDIR)aboutdialog.o $(OBJSDIR)gotodialog.o $(OBJSDIR)finddialog.o $(OBJSDIR)replacedialog.o $(OBJSDIR)buffer.o $(OBJSDIR)mytabview.o $(OBJSDIR)tabview.o $(OBJSDIR)icons.o $(OBJSDIR)dialog.o
EXE = aedit

all: $(OBJSDIR) $(EXE)

$(OBJSDIR):
	mkdir $(OBJSDIR)

$(EXE): catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	rescopy -r $(EXE) resources/aedit.catalog
	rescopy -a $(EXE) stock_copy_16=pixmaps/stock_copy_16.png
	rescopy -a $(EXE) stock_copy_24=pixmaps/stock_copy_24.png
	rescopy -a $(EXE) stock_cut_16=pixmaps/stock_cut_16.png
	rescopy -a $(EXE) stock_cut_24=pixmaps/stock_cut_24.png
	rescopy -a $(EXE) stock_exit_16=pixmaps/stock_exit_16.png
	rescopy -a $(EXE) stock_jump_to_16=pixmaps/stock_jump_to_16.png
	rescopy -a $(EXE) stock_left_arrow_16=pixmaps/stock_left_arrow_16.png
	rescopy -a $(EXE) stock_new_16=pixmaps/stock_new_16.png
	rescopy -a $(EXE) stock_new_24=pixmaps/stock_new_24.png
	rescopy -a $(EXE) stock_open_16=pixmaps/stock_open_16.png
	rescopy -a $(EXE) stock_open_24=pixmaps/stock_open_24.png
	rescopy -a $(EXE) stock_paste_16=pixmaps/stock_paste_16.png
	rescopy -a $(EXE) stock_paste_24=pixmaps/stock_paste_24.png
	rescopy -a $(EXE) stock_redo_16=pixmaps/stock_redo_16.png
	rescopy -a $(EXE) stock_redo_24=pixmaps/stock_redo_24.png
	rescopy -a $(EXE) stock_right_arrow_16=pixmaps/stock_right_arrow_16.png
	rescopy -a $(EXE) stock_save_16=pixmaps/stock_save_16.png
	rescopy -a $(EXE) stock_save_24=pixmaps/stock_save_16.png
	rescopy -a $(EXE) stock_save_as_16=pixmaps/stock_save_as_16.png
	rescopy -a $(EXE) stock_search_16=pixmaps/stock_search_16.png
	rescopy -a $(EXE) stock_search_24=pixmaps/stock_search_24.png
	rescopy -a $(EXE) stock_search_replace_16=pixmaps/stock_search_replace_16.png
	rescopy -a $(EXE) stock_undo_16=pixmaps/stock_undo_16.png
	rescopy -a $(EXE) stock_undo_24=pixmaps/stock_undo_24.png
	rescopy -a $(EXE) stock_select_all_16=pixmaps/stock_select_all_16.png
	rescopy -a $(EXE) stock_close_16=pixmaps/stock_close_16.png
	rescopy -a $(EXE) stock_font_16=pixmaps/stock_font_16.png
	rescopy -a $(EXE) icon24x24.png=pixmaps/icon24x24.png
	rescopy -a $(EXE) icon48x48.png=pixmaps/icon48x48.png
	rescopy -a $(EXE) text_plain.png=pixmaps/text_plain.png
	rescopy -a $(EXE) about=pixmaps/about.png
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(EXE) $$CATALOG/$(EXE).catalog=resources/$$CATALOG/$(EXE).catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c $(EXE).cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t $(EXE).cd; \
		cd ..; \
	done;

deps:

install: all
	strip --strip-all $(EXE)
	mkdir -p /Applications/AEdit
	cp $(EXE) /Applications/AEdit/AEdit
	ln -sf /Applications/AEdit/AEdit /system/bin/$(EXE)

dist: all
	strip --strip-all $(EXE)
	mkdir -p $(IMAGE)/Applications/AEdit
	install -s $(EXE) $(IMAGE)/Applications/AEdit/AEdit
	ln -sf /Applications/AEdit/AEdit $(IMAGE)/system/bin/$(EXE)

clean: catalog-clean
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

$(OBJSDIR)aboutdialog.o: aboutdialog.cpp aboutdialog.h
	$(CXX) $(COPTS) aboutdialog.cpp -o $@

$(OBJSDIR)gotodialog.o: gotodialog.cpp gotodialog.h
	$(CXX) $(COPTS) gotodialog.cpp -o $@

$(OBJSDIR)finddialog.o: finddialog.cpp finddialog.h
	$(CXX) $(COPTS) finddialog.cpp -o $@

$(OBJSDIR)replacedialog.o: replacedialog.cpp replacedialog.cpp
	$(CXX) $(COPTS) replacedialog.cpp -o $@

$(OBJSDIR)buffer.o: buffer.cpp buffer.h
	$(CXX) $(COPTS) buffer.cpp -o $@

$(OBJSDIR)status_bar.o: status_bar.cpp status_bar.h
	$(CXX) $(COPTS) status_bar.cpp -o $@

$(OBJSDIR)mytabview.o: mytabview.cpp mytabview.h
	$(CXX) $(COPTS) mytabview.cpp -o $@

$(OBJSDIR)tabview.o: tabview.cpp tabview.h
	$(CXX) $(COPTS) tabview.cpp -o $@

$(OBJSDIR)icons.o: icons.cpp icons.h
	$(CXX) $(COPTS) icons.cpp -o $@

$(OBJSDIR)dialog.o: dialog.cpp dialog.h
	$(CXX) $(COPTS) dialog.cpp -o $@

catalog-clean:
	-rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;

