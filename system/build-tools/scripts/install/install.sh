#!/bin/sh

INSTALL_DIR=/boot/Install

if [ ! -e $INSTALL_DIR ]; then
  echo This script must be run from the installation CD.
  exit 1
fi;

cd $INSTALL_DIR

clear
cat doc/welcome.txt

echo -n "Do you want to (I)nstall or (Q)uit now? "
read OPTION

case $OPTION in
  "I" | "i" )
    ./do_install.sh;;

  "Q" | "q" | * )
    exit 2
esac;

echo "Syllable has now been installed!  Press Ctrl+Alt+Del to restart your computer"; echo
aterm &

exit 0


