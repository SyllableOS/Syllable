#!/bin/bash

for file in $(find $1/*)
do
	link_file=$(ls -l "$file" | grep ">" | grep "$1")
	if [ "$link_file" ];then
		echo $link_file
	fi
done

