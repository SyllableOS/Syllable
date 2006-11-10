#Makefile for Printer.so
COPTS = -c -fexceptions -ffast-math

APPBIN = /system/extensions/dock
OBJS = Printer.o

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

all : objs Printer.so

objs:
	@mkdir -p objs

Printer.so: $(OBJS)
	@echo Linking...
	@$(CXX) -shared -Xlinker -soname=Printer.so $(OBJS) -o Printer.so -lsyllable
	@echo Adding resources...
	@rescopy Printer.so -r *.png 

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f Printer.so

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/atheos/sys/extensions/dock
	@cp Printer.so $(DIST_DIR)/atheos/sys/extensions/dock/Printer.so

install: all
	@echo Installing...
	@mkdir -p /system/extensions/dock
	@cp Printer.so /system/extensions/dock/Printer.so

