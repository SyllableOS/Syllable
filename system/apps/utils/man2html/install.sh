#!/bin/sh

base=/atheos/Documentation
rpath=man
out=$base/$rpath

install=/usr/man2html

hd=$(pwd)

if [ -z $1 ]; then
	manpages=/atheos/autolnk/man
else
	manpages=$1
fi;

if [ ! -e $install ]; then
	mkdir $install
	mkdir $install/bin
fi;

echo Building man2html
make -C src/man2html/
cp -f src/man2html/man2html $install/bin/

echo Converting manpages
if [ ! -e $out ]; then
	mkdir $out;
fi;

for d in $(ls $manpages);do
	if [ ! -e $out/$d ]; then
		mkdir $out/$d;
	fi;

	echo Entering $d
	cd $manpages/$d

	for f in $(ls);do
		echo Processing $f
		man2html -r -H file://$base -M $rpath $f > $out/$d/$f.html
	done;

	cd $hd
done;

cp -f rebuild_all.sh $install/bin
cp -f mksection.sh $install/bin
cp -f mkindex.sh $install/bin
cp -f addpage.sh $install/bin
cp -f manaddpackage.sh $install/bin
pkgmanager -a $install

cd $out
rebuild_all.sh
cd $hd

