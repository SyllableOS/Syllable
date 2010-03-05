COPTS	= -c -fexceptions -ffast-math

OBJS	= main.o tview.o settings.o swindow.o
EXE	= aterm.new

$(EXE): $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) *.png

install: $(EXE)
	-mkdir -p "/applications/system"
	cp -f $(EXE) "/applications/system/Terminal"
	addattrib "/applications/system/Terminal" "os::Category" "System Tools"
	ln -sf "/applications/system/Terminal" /system/programs/aterm

dist: $(EXE)
	-mkdir -p "$(IMAGE)/applications/system"
	cp -f $(EXE) "$(IMAGE)/applications/system/Terminal"
	addattrib "$(IMAGE)/applications/system/Terminal" "os::Category" "System Tools"
	ln -sf "/applications/system/Terminal" $(IMAGE)/system/programs/aterm

clean:
	-rm $(OBJS)
	-rm $(EXE)

deps:

%.o : %.c
	$(CC) $(COPTS) $< -o $@

%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@

settings.o:	settings.cpp settings.h
main.o:		main.cpp tview.h messages.h
tview.o:	tview.cpp tview.h swindow.h messages.h
swindow.o:	swindow.cpp swindow.h messages.h
