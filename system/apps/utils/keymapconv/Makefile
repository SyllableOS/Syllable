COPTS	= -c

OBJS	= keymapconv.o
EXE	= keymapconv

$(EXE) : $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $@

deps:

install: $(EXE)
	cp -f $(EXE) /system/bin/$(EXE)
	strip --strip-all /system/bin/$(EXE)

dist: $(EXE)
	cp -f $(EXE) $(IMAGE)/system/bin/$(EXE)
	strip --strip-all $(IMAGE)/system/bin/$(EXE)

clean:
	-rm $(OBJS)

.cpp.o:
	$(CXX) $(COPTS) $< -o $@

main.o : main.cpp
	$(CXX) $(COPTS) $< -o $@

