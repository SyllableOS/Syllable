#Makefile for SimpleBurn
COPTS = -Wall -c -fexceptions

APPBIN = /Applications/SimpleBurn
OBJS = main.o application.o mainwindow.o splash.o erasewindow.o

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

all : translations objs "SimpleBurn"

$(OBJDIR)/mainwindow.o: mainwindowLayout.cpp

translations:

objs:
	@mkdir -p objs

"SimpleBurn": $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o "SimpleBurn" -lsyllable -lmedia
	@echo Adding resources...
	@rescopy "SimpleBurn" -r images/add-audio.png images/add-video.png images/burn.png images/icon24x24.png images/icon48x48.png images/menu-audio.png images/menu-backup.png images/menu-data.png images/menu-video.png images/move-down.png images/move-up.png images/remove.png images/menu-burn.png images/logo.png 
	@addattrib "SimpleBurn" os::Category Other

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f "SimpleBurn"

deps:

install: all
	@echo Installing...
	@mkdir -p $(IMAGE)/Applications/SimpleBurn
	@cp "SimpleBurn" "$(IMAGE)/Applications/SimpleBurn/SimpleBurn"

dist: install
