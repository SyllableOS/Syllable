COPTS	= -c -I../../kernel/drivers/misc/w8378x/

OBJS	= main.o graphview.o barview.o
EXE	= cpu

$(EXE) : $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $@

install: $(EXE)
	strip --strip-all $(EXE)
	cp -f $(EXE) /bin/$(EXE)

dist: $(EXE)
	strip --strip-all $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

main.o : main.cpp
	$(CXX) $(COPTS) $< -o $@
barview.o : barview.cpp barview.h
	$(CXX) $(COPTS) $< -o $@
graphview.o : graphview.cpp graphview.h
	$(CXX) $(COPTS) $< -o $@

