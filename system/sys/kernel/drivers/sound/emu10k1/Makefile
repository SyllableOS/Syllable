DRIVER	:= emu10k1
OBJDIR	:= objs

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

all: $(OBJDIR) $(DRIVER)

deps:

$(OBJDIR):
	mkdir $(OBJDIR)

$(DRIVER):
	@cd src/ && $(MAKE)

clean:
	@cd src/ && $(MAKE) clean

install: all
	cp -f $(DRIVER) $(IMAGE)/system/drivers/dev/audio/$(DRIVER)

dist: install

