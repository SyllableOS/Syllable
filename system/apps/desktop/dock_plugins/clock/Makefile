#Makefile for Clock.so
COPTS = -c -fexceptions -ffast-math

APPBIN = /system/drivers/dock
OBJS = ColorSelector.o Settings.o Clock.o colorbutton.o

OBJDIR := objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

# Rules
$(OBJDIR)/%.o : %.c
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.s
	@echo Assembling : $<
	@$(CC) $(COPTS) -x assembler-with-cpp $< -o $@

all : objs Clock.so

objs:
	@mkdir -p objs

Clock.so: $(OBJS)
	@echo Linking...
	@$(CXX) -shared -Xlinker -soname=Clock.so $(OBJS) -o Clock.so -lsyllable
	@echo Adding resources...
	@rescopy Clock.so -r icon48x48.png 

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f Clock.so

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/atheos/sys/drivers/dock
	@cp Clock.so $(DIST_DIR)/atheos/sys/drivers/dock/Clock.so

install: all
	@echo Installing...
	@mkdir -p /system/drivers/dock
	@cp Clock.so /system/drivers/dock/Clock.so

