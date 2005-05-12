COPTS	= -c

OBJS	= main.o
EXE	= memmon

$(EXE) : $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $@

deps:

install: $(EXE)
	cp -f $(EXE) /bin/$(EXE)
	strip --strip-all /bin/$(EXE)

dist: $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)
	strip --strip-all $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:
	-rm $(OBJS)

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

main.o : main.cpp
	$(CXX) $(COPTS) $< -o $@

