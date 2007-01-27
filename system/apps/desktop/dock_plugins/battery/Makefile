# Makefile for Battery.so

COPTS = -c -fexceptions -ffast-math -I../include

APPBIN = /system/extensions/dock
OBJS = Battery.o

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

all : objs Battery.so

objs:
	@mkdir -p objs

Battery.so: $(OBJS)
	@echo Linking...
	@$(CXX) -shared -Xlinker -soname=Battery.so $(OBJS) -o Battery.so -lsyllable -lmedia
	@echo Adding resources...
	@rescopy Battery.so -r *.png 

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f Battery.so

deps:

install: all
	@echo Installing...
	@cp Battery.so $(APPBIN)/Battery.so

dist: all
	@echo Distribution...
	@cp Battery.so $(IMAGE)/$(APPBIN)/Battery.so

