#!/bin/sh

kill_all -term launcher
kill_all -term desktop
cd ../src/
make clean
make
mv ../objs/desktop /bin
