#!/bin/sh
cd Templates
rm *.zip
for i in *; do
if [ -d "$i" ]
then
echo Building $i.zip
cd "$i"
zip "../$i.zip" *
cd ..
fi
done
rm CVS.zip
cd ..
