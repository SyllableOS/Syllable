Meter: meter.o
	$(CC) -o meter.so -shared meter.o -lsyllable
	rescopy meter.so -r images/*.png

meter.o: meter.cpp meter.h
	$(CC) -Wall -c meter.cpp

install: Meter
	if [ -e /system/drivers/dock/meter.so ]; then rm /system/drivers/dock/meter.so; fi
	cp meter.so /system/drivers/dock/

dist: Meter
	cp -f meter.so $(DIST_DIR)/atheos/sys/drivers/dock/

