#!/bin/sh
#
# Convert a manpage to HTML, rebuild the section index and if
# required, rebuild the main index
#
# First argument is the path to the manpage to add

hd=$(pwd)

f=$1
p=$hd/$f
section=$(echo $f | cut -d . -f 2 );
d=man$section;

base=/atheos/Documentation
rpath=man
out=$base/$rpath

rebuild_index=0;

cd $out

if [ ! -e $d ];then
	mkdir $d;
	rebuild_index=1;
fi;

echo Adding $f to section $section
man2html -r -H file://$base -M $rpath $p > $d/$f.html

echo Rebuilding section index
./mksection.sh $d index_$section.html

if [ $rebuild_index -eq 1 ];then
	echo Rebuilding main index
	./mkindex.sh
fi;

echo Done
cd $hd

