Meter: meter.o
	$(CXX) -o meter.so -shared meter.o -lsyllable
	rescopy meter.so -r images/*.png

meter.o: meter.cpp meter.h
	$(CXX) -c meter.cpp

install: Meter
	if [ -e /system/drivers/dock/Meter.so ]; then rm /system/drivers/dock/Meter.so; fi
	cp meter.so /system/drivers/dock/Meter.so

dist: Meter
	cp -f meter.so $(DIST_DIR)/atheos/sys/drivers/dock/Meter.so

clean:
	rm -f meter.o meter.so
