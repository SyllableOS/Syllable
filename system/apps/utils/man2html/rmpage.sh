#!/bin/sh
#
# $1 : manpage to remove (The manpage filename, E.g. cvsbug.8 )

hd=$(pwd)

f=$1
p=$hd/$f
section=$(echo $f | cut -d . -f 2 );
d=man$section;

base=/atheos/Documentation
rpath=man
out=$base/$rpath

if [ ! -e $out/$d/$f.html ];then
	echo Manpage $f does not exist
	exit 1;
fi;

cd $out

echo Removing manpage $f
rm -f $d/$f.html

if [ $(ls -a -1 $d | wc -l) -eq 2 ];then
	rmdir man$section
	rm -f index_$section.html
	echo Rebuilding main index
	./mkindex.sh
else
	echo Rebuilding section index
	./mksection.sh $d index_$section.html
fi;


echo Done
cd $hd
