# Makefile for Clock.so

COPTS = -c -fexceptions -ffast-math

APPBIN = /system/extensions/dock
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

install: all
	@echo Installing...
	@cp Clock.so $(IMAGE)/$(APPBIN)/Clock.so

dist: install

