
if test "$boot_mode" = normal; then
    export PATH=/usr/bin:/usr/local/bin:/boot/atheos/sys/bin/:./
    export DLL_PATH=@bindir@/lib:./:/boot/atheos/sys/libs:/boot/atheos/sys:/atheos/autolnk/lib
else
    export PATH=/usr/bin:/usr/local/bin:/boot/atheos/sys_safe/bin/:./
    export DLL_PATH=@bindir@/lib:./:/boot/atheos/sys_safe/:/boot/atheos/sys_safe/libs
fi

export SYSTEM=AtheOS
export CC=gcc
export HOME=/home/root
export SHELL=bash
export TEMP=/tmp

