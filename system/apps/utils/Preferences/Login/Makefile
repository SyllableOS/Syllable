CXXFLAGS += -c -I.
OBJS = main.o login_window.o
EXE = Login
TARGET_EXE = Login\ Details

#Define TEST if you're testing the Preferences; doesn't overwrite system files
#CXXFLAGS += -DTEST

$(EXE): $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lcrypt -o $(EXE)
	strip --strip-all $(EXE)
	rescopy $(EXE) -r *.png

install: dist

dist: $(EXE)
	install -s $(EXE) $(DIST_DIR)/atheos/Applications/Preferences/$(TARGET_EXE)
	chmod +s $(DIST_DIR)/atheos/Applications/Preferences/$(TARGET_EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@
