#!/bin/sh

add_directory()
{
	cd $1
	for current in `ls $1`;do
		if [ -d $current ];then
			echo "Adding directory $current"
			cvs add $current
			add_directory $1/$current
		else
			echo "Adding file $current"
			cvs add $current
		fi
	done
	cd ..
}

add_directory `pwd`
	