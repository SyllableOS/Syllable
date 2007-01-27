# Makefile for Eyes.so

COPTS = -c -fexceptions -ffast-math

APPBIN = /system/extensions/dock
OBJS = eyewidget.o main.o

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

all : objs Eyes.so

objs:
	@mkdir -p objs

Eyes.so: $(OBJS)
	@echo Linking...
	@$(CXX) -shared -Xlinker -soname=Eyes.so $(OBJS) -o Eyes.so -lsyllable

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f Eyes.so

deps:

install: all
	@echo Installing...
	@cp Eyes.so $(APPBIN)/Eyes.so

dist: all
	@echo Distribution...
	@cp Eyes.so $(IMAGE)/$(APPBIN)/Eyes.so

