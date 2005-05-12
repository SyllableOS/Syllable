#Makefile for Mixer.so
COPTS = -c -fexceptions -ffast-math

APPBIN = /system/drivers/dock
OBJS = Mixer.o

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

all : objs Mixer.so

objs:
	@mkdir -p objs

Mixer.so: $(OBJS)
	@echo Linking...
	@$(CXX) -shared -Xlinker -soname=Mixer.so $(OBJS) -o Mixer.so -lsyllable -lmedia
	@echo Adding resources...
	@rescopy Mixer.so -r *.png 

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f Mixer.so

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/atheos/sys/drivers/dock
	@cp Mixer.so $(DIST_DIR)/atheos/sys/drivers/dock/Mixer.so

install: all
	@echo Installing...
	@mkdir -p /system/drivers/dock
	@cp Mixer.so /system/drivers/dock/Mixer.so

