COPTS	= -c

OBJS	= keymapconv.o
EXE	= keymapconv

$(EXE) : $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $@

deps:

install: $(EXE)
	cp -f $(EXE) /system/programs/$(EXE)
	strip --strip-all /system/programs/$(EXE)

dist: $(EXE)
	cp -f $(EXE) $(IMAGE)/system/programs/$(EXE)
	strip --strip-all $(IMAGE)/system/programs/$(EXE)

clean:
	-rm $(OBJS)

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

main.o : main.cpp
	$(CXX) $(COPTS) $< -o $@
