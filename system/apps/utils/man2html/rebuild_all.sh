#!/bin/sh

for d in $(ls | grep man); do
	mksection.sh $d $(echo $d | sed -e s/man/index_/).html
done;

mkindex.sh

