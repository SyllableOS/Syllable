CXXFLAGS = -Wall -c -I.
OBJS = main.o users_window.o user_propertiesdlg.o changepwddlg.o group_propertiesdlg.o
EXE = Users
TARGET_EXE = Users\ \&\ Groups

#Define TEST if you're testing the Preferences; doesn't overwrite system files
#CXXFLAGS += -DTEST

$(EXE): $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lcrypt -o $(EXE)
	strip --strip-all $(EXE)
	rescopy $(EXE) -r *.png

install: dist

dist: $(EXE)
	install -s $(EXE) $(DIST_DIR)/atheos/Applications/Preferences/$(TARGET_EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

