#!/bin/sh

make clean -C src

if [ -e bin ]; then
	rm -rf bin
fi;

if [ -e objs ]; then
	rm -rf objs
fi;

if [ -e root ]; then
	rm -rf root
fi;

if [ -e miniroot ]; then
	rm -rf miniroot
fi;

if [ -e base ]; then
	rm -rf base
fi;

if [ -e *.iso ]; then
	rm *.iso
fi;

