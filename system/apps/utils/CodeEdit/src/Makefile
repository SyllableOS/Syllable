OBJS = ../objs/App.o ../objs/EditWin.o ../objs/StatusBar.o ../objs/KeyMap.o ../objs/Dialogs.o ../objs/SettingsDialog.o ../objs/SettingsTab_Key.o ../objs/SettingsTab_Formats.o ../objs/FavoriteMenu.o ../objs/ToolBar.o ../objs/ImageItem.o ../objs/LoadBitmap.o ../objs/Version.o ../objs/SettingsTab_Font.o

COPTS = -Wall --no-warn
LOPTS = -latheos -lcodeview
CC= g++
LL = g++

EXE = ../objs/CodeEdit

$(EXE): $(OBJS)
	@echo Building : $(EXE)
	@$(LL) $(OBJS) $(LOPTS) -o $(EXE)
	@echo Adding images into : $(EXE)
	@rescopy -r $(EXE) ./images/*.png

.PHONY : clean install uninstall
clean:
	@echo  Removing all objects and executable.
	@-rm -f $(OBJS)
	@-rm -f $(EXE)
	@-rmdir ../objs

astyle:
	@echo Running astyle on all source files.
	@astyle --style=ansi *.cpp
	@astyle --style=ansi *.h
	@echo Removing all original files.
	@rm -rf *.orig

../objs/%.o: %.cpp
	@mkdir -p ../objs
	@echo Compiling:  $<
	@$(CC) $< $(COPTS) -c -o $@

../objs/App.o: App.h App.cpp EditWin.h SettingsDialog.h KeyMap.h Dialogs.h
../objs/EditWin.o: EditWin.h EditWin.cpp App.h StatusBar.h KeyMap.h Dialogs.h
../objs/StatusBar.o: StatusBar.h StatusBar.cpp
../objs/KeyMap.o: KeyMap.h KeyMap.cpp
../objs/Dialogs.o: Dialogs.h Dialogs.cpp KeyMap.h
../objs/SettingsDialog.o: SettingsDialog.h SettingsDialog.cpp App.h KeyMap.h SettingsTab_Key.h SettingsTab_Formats.h
../objs/SettingsTab_Key.o: SettingsTab_Key.h SettingsTab_Key.cpp EditWin.h App.h KeyMap.h
../objs/SettingsTab_Formats.o: SettingsTab_Formats.h SettingsTab_Formats.cpp App.h KeyMap.h
../objs/FavoriteMenu.o: FavoriteMenu.h FavoriteMenu.cpp
../objs/ToolBar.o: ToolBar.cpp ToolBar.h
../objs/Version.o: Version.cpp Version.h
../objs/ImageItem: ImageItem.cpp ImageItem.h
../objs/LoadBitmap.o: LoadBitmap.cpp LoadBitmap.h
../objs/SettingsTab_Font.o:  SettingsTab_Font.h SettingsTab_Font.cpp App.h



















