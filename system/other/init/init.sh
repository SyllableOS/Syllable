#! /bin/sh

boot_mode=$1
sys_path=$2
usr_path=$3

echo "sys_path:" $sys_path
sys_path=/boot/atheos

export PATH=/usr/bin:/bin

. $sys_path/sys/setup_environ.sh

ln -s usr/glibc2/include /include

if [ -e /atheos/src/include ]; then
    ln -s atheos/src/include /ainc
else
    ln -s system/include  /ainc
fi
ln -s atheos/Applications /Applications

if test "$1" = safe; then
    aterm &
else
    if [ -e /bin/dlogin ]; then
	. /etc/profile
	/bin/dlogin </dev/null >>/var/log/desktop.log 2>&1 &
    else
	aterm &
    fi
fi

. /system/user_init.sh
