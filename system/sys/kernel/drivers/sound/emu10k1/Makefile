DRIVER := emu10k1

all:
	@cd src/ && $(MAKE)

deps:

clean:
	@cd src/ && $(MAKE) clean

install:
	@cd src/ && $(MAKE)
	cp -f $(DRIVER) /system/drivers/dev/sound/$(DRIVER)

dist:
	cp -f $(DRIVER) $(DIST_DIR)/atheos/sys/drivers/dev/sound/$(DRIVER)

