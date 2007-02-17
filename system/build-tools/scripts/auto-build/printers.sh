#!/bin/bash
#
# The resulting (sorted) file goes in /usr/cups/share/cups/model/models.list
# and must be symlinked from /atheos/autolnk/share/cups/model/models.list

if [ -z $1 ]
then
  PPD_PATH=/usr/cups/share/cups/model
else
  PPD_PATH=$1
fi

if [ ! -z $2 ]
then
  FILE=$2/models.list
else
  FILE=$PPD_PATH/models.list
fi
FILE_UNSORTED=$FILE.unsorted

if [ -e $FILE_UNSORTED ]
then
  rm $FILE_UNSORTED
fi

echo "Building driver list from $PPD_PATH"

cd $PPD_PATH
for f in `find -name \*ppd*`
do
  if [[ $f == *.gz ]]
  then
    # Compressed
    MANF=`zgrep Manufacturer: $f | cut -d \" -f 2`
    MODEL=`zgrep ModelName: $f | cut -d \" -f 2`
  else
    # Uncompressed
    MANF=`grep Manufacturer: $f | cut -d \" -f 2`
    MODEL=`grep ModelName: $f | cut -d \" -f 2`
  fi
  printf "%s\t%s\t%s\n" "$MANF" "$MODEL" "$f" >> $FILE_UNSORTED
  printf "."
done

cat $FILE_UNSORTED | sort -u > $FILE
COUNT=`wc -l $FILE | cut -d' ' -f 1`

rm $FILE_UNSORTED

printf "\nFinished.  $COUNT drivers found.\n"
exit 0

