#Makefile for Meter.so
COPTS = -c -fexceptions -ffast-math

APPBIN = /system/extensions/dock
OBJS = meter.o

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

all : objs Meter.so

objs:
	@mkdir -p objs

Meter.so: $(OBJS)
	@echo Linking...
	@$(CXX) -shared -Xlinker -soname=Meter.so $(OBJS) -o Meter.so -lsyllable
	@echo Adding resources...
	@rescopy Meter.so -r images/*.png 

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f Meter.so

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/atheos/sys/extensions/dock
	@cp Meter.so $(DIST_DIR)/atheos/sys/extensions/dock/Meter.so

install: all
	@echo Installing...
	@mkdir -p /system/extensions/dock
	@cp Meter.so /system/extensions/dock/Meter.so





