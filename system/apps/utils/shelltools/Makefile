OPTS   = -c -Wall --no-warn

CC	= g++
EXE	= objs/alert

OBJDIR	:= objs


$(OBJDIR)/%.o : %.c
	$(CC) $(OPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@



OBJS := ./objs/alert.o


$(EXE): objs $(OBJS)
	@echo Building : $(EXE)
	@g++ $(OBJS) -lsyllable -o $(EXE)


$(OBJDIR):
	@echo Making $(OBJDIR) directory
	@mkdir objs
	
dist:
	@echo Coping $(EXE) to $(DIST_DIR)/atheos/sys/bin
	@cp -f ./objs/alert $(DIST_DIR)/atheos/sys/bin

clean:
	@echo Removing all objects and executable.
	@rm $(OBJS)
	@rm $(EXE)

