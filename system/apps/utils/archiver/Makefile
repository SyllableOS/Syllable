COPTS = -Wall -c

OBJS =  main.o
EXE	= archiver

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	$(LL) -Wall  $(OBJS)  -latheos  -o $@

dist: $(EXE)
	-mkdir $(DIST_DIR)/usr/archiver
	-mkdir $(DIST_DIR)/usr/archiver/bin
	cp -f $(EXE) $DIST_DIR)/usr/archiver/bin/$(EXE)
	
	

clean:
	-rm -rf $(OBJS)
	-rm -rf $(EXE)

main.o:		    main.cpp archiver.cpp new.cpp extract.cpp  messages.h imagebutton.cpp exe.cpp options.cpp  
