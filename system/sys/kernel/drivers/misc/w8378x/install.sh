#!/bin/sh

srcdir=$(dirname $0)

mimeset w8378x

mimetype=$(catattr BEOS:TYPE $srcdir/w8378x)
if echo $mimetype | grep -q application/x-vnd.Be-elfexecutable ; then
  targetlplatform=x86
elif echo $mimetype | grep -q application/x-be-executable ; then
  targetlplatform=ppc
else
  alert --stop "The file w8378x has an unknown mimetype, and can not be installed" "Damn"
  return
fi

uname_m=$(uname -m)
if [ $uname_m = "BePC" ] ; then
  platform=x86
elif [ $uname_m = "BeBox" ] ; then
  platform=ppc
else
  alert --stop "Unknown platform:$uname_m
  (I don't think this driver will work on Mac's)" "Damn"
  return
fi

if [ $platform = $targetlplatform ] ; then
  cp -f $srcdir/w8378x ~/config/add-ons/kernel/drivers/bin/

  mkdir -p ~/config/add-ons/kernel/drivers/dev/misc
  ln -sf ../../bin/w8378x ~/config/add-ons/kernel/drivers/dev/misc/w8378x
fi


