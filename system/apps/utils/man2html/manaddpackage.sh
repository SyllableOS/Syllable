#!/bin/sh

hd=$(pwd)

base=/atheos/Documentation
rpath=man
out=$base/$rpath

rebuild_index=0

if [ -z $1 ]; then
	echo You must pass the path to the manpage directory
	exit 1;
fi;

if [ ! -e $out ]; then
	mkdir $out;
	rebuild_index=1;
fi;

for d in $(ls $1);do
	section=$(echo $d | sed -e s/man// );

	if [ ! -e $out/$d ]; then
		mkdir $out/$d;
		rebuild_index=1;
	fi;

	cd $1/$d

	for f in $(ls);do
		echo Adding $f to section $section
		man2html -r -H file://$base -M $rpath $1/$d/$f > $out/$d/$f.html
	done;

	echo Rebuilding section index
	mksection.sh $out/$d $out/index_$section.html

	cd $hd
done;

if [ $rebuild_index -eq 1 ];then
	echo Rebuilding main index
	mkindex.sh
fi;

