#!/bin/sh

boot_mode=$1
sys_path=$2
usr_path=$3

sys_path=/boot/atheos
echo "sys_path:" $sys_path

export PATH=/usr/bin:/boot/atheos/sys/bin/
export DLL_PATH=@bindir@/lib:./:/boot/atheos/sys/libs:/boot/atheos/sys
export SYSTEM=AtheOS
export CC=gcc
export HOME=/home/root
export SHELL=bash
export TEMP=/tmp

aterm /boot/Install/install.sh &

