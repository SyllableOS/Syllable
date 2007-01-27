COPTS	= -c -fexceptions -ffast-math

OBJS	= main.o tview.o settings.o swindow.o
EXE	= aterm.new

$(EXE): $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) *.png

install: $(EXE)
	-mkdir -p "/Applications/System Tools"
	cp -f $(EXE) "/Applications/System Tools/Terminal"
	addattrib "/Applications/System Tools/Terminal" "os::Category" "System Tools"
	ln -sf "/Applications/System Tools/Terminal" /system/bin/aterm

dist: $(EXE)
	-mkdir -p "$(IMAGE)/Applications/System Tools"
	cp -f $(EXE) "$(IMAGE)/Applications/System Tools/Terminal"
	addattrib "$(IMAGE)/Applications/System Tools/Terminal" "os::Category" "System Tools"
	ln -sf "/Applications/System Tools/Terminal" $(IMAGE)/system/bin/aterm

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

