COPTS	= -c

OBJS	= calc.o calculator.o stack.o calcbuttons.o calcdisplay.o togglebutton.o remsa.o base-n.o postoffice.o aboutwin.o
EXE	= Calculator

CATALOGS = bg da de el es fr hu nl pt-br ru sv

$(EXE) : catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lm -o $@
	rescopy -r $(EXE) *.png
	rescopy -a $(EXE) resources/$(EXE).catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(EXE) $$CATALOG/$(EXE).catalog=resources/$$CATALOG/$(EXE).catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c $(EXE).cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t $(EXE).cd; \
		cd ..; \
	done;

clean: catalog-clean
	-rm $(OBJS)

catalog-clean:
	-rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;

tar:
	-rm $(OBJS)
	-rm Albert.tar.gz
	tar -cpzvf Albert.tar.gz -C .. Albert

install: $(EXE)
	mkdir -p $(IMAGE)/Applications/Calculator
	mv Calculator $(IMAGE)/Applications/Calculator/

dist: install

deps:
	

#.c.o :
#	$(CC) $(COPTS) $< -o $@
.cpp.o:
	$(CXX) $(COPTS) $< -o $@

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

