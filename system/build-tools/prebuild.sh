#!/bin/sh

echo "Cleaning out CVS dirs (WARNING!  This will clobber your CVS repository!)"

for file in $(find * | grep -w CVS )
do
	echo "Removing $file"
	rm -r $file
done

#Finished
exit 0

