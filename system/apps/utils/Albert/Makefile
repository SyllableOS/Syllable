COPTS	= -Wall -c

OBJS	= calc.o calculator.o stack.o calcbuttons.o calcdisplay.o togglebutton.o remsa.o base-n.o postoffice.o aboutwin.o
EXE	= Albert

CC	= gcc
LL	= gcc

$(EXE) : $(OBJS)
	$(LL) $(OBJS) -lsyllable -lstdc++ -lm -o $@
	rescopy -r $(EXE) *.png

clean:
	-rm $(OBJS)

tar:
	-rm $(OBJS)
	-rm Albert.tar.gz
	tar -cpzvf Albert.tar.gz -C .. Albert

dist:
	mkdir -p $(DIST_DIR)/atheos/Applications/Albert
	mv Albert $(DIST_DIR)/atheos/Applications/Albert/

deps:
	

#.c.o :
#	$(CC) $(COPTS) $< -o $@
.cpp.o:
	$(CC) $(COPTS) $< -o $@

calc.o:		calc.cpp calculator.h stack.h calcbuttons.h calcdisplay.h calc.h togglebutton.h remsa.h base-n.h
calculator.o:	calculator.cpp calculator.h stack.h
stack.o:		stack.cpp stack.h
calcbuttons.o:	calcbuttons.cpp calcbuttons.h togglebutton.h
remsa.o:		remsa.cpp remsa.h
calcdisplay.o:	calcdisplay.cpp calcdisplay.h calc.h
togglebutton.o:	togglebutton.cpp togglebutton.h
base-n.o:			base-n.cpp base-n.h
postoffice.o:		postoffice.cpp postoffice.h
aboutwin.o:		aboutwin.cpp aboutwin.h
