DRIVER	:= emu10k1
OBJDIR	:= objs

all: $(OBJDIR) $(DRIVER)

deps:

$(OBJDIR):
	mkdir $(OBJDIR)

$(DRIVER):
	@cd src/ && $(MAKE)

clean:
	@cd src/ && $(MAKE) clean

install: all
	cp -f $(DRIVER) /system/drivers/dev/sound/$(DRIVER)

dist: all
	cp -f $(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/sound/$(DRIVER)

