COPTS	= -Wall -c

OBJS	= ../objs/main.o ../objs/loadbitmap.o
EXE	= ../objs/aview

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	@echo Building : $(EXE)
	@g++ $(OBJS) -latheos -o $(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

deps:
	mkdir ../objs

dist:
	mkdir $(DIST_DIR)/Applications/AView
	mkdir $(DIST_DIR)/Applications/AView/bin
	cp -f  $(EXE) $(DIST_DIR)/Applications/AView/bin/$(EXE)

astyle:
	astyle --style=ansi *.cpp *.h
	rm -rf *.orig

.cpp.o:
	$(CC) $(COPTS) $< -o $@

../objs/main.o : main.cpp mainapp.h window.h
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/loadbitmap.o : loadbitmap.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@





