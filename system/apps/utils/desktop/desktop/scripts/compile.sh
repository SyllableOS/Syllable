#!/bin/sh

kill_all -term launcher
kill_all -term desktop
make
mv ../objs/desktop /bin/

