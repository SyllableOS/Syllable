COPTS	= -c

OBJS	= main.o gui/clockview.o
EXE	= aclock

CATALOGS = bg da de el en-gb es fr hu nl pt-br ru sv zh-cn

$(EXE) : catalogs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lm -o $@
	rescopy -r $(EXE) icon48x48.png 
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

install: $(EXE)
	cp -f $(EXE) /system/programs/$(EXE)
	strip --strip-all /system/programs/$(EXE)

upgrade: $(EXE)
	rfs $(EXE) /system/programs/$(EXE)
deps:

dist: $(EXE)
	cp -f $(EXE) $(IMAGE)/system/programs/$(EXE)
	strip --strip-all $(IMAGE)/system/programs/$(EXE)

clean: catalog-clean
	-rm $(OBJS)
#.c.o :
#	$(CC) $(COPTS) $< -o $@
.cpp.o:
	$(CXX) $(COPTS) $< -o $@

catalog-clean:
	-rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;

main.o:	main.cpp gui/clockview.h mywindow.h
gui/clockview.o: gui/clockview.cpp gui/clockview.h
