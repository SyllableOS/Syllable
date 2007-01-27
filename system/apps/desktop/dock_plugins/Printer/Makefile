# Makefile for Printer.so

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

install: all
	@echo Installing...
	@cp Printer.so $(APPBIN)/Printer.so

dist: all
	@echo Distribution...
	@cp Printer.so $(IMAGE)/$(APPBIN)/Printer.so

