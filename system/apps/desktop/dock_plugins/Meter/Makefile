Meter: meter.o
	g++ -o meter.so -shared meter.o -lsyllable
	rescopy meter.so -r images/*.png
	if [ -e /system/drivers/dock/meter.so ]; then rm /system/drivers/dock/meter.so; fi
	cp meter.so /system/drivers/dock/

meter.o: meter.cpp meter.h
	g++ -Wall -c meter.cpp

