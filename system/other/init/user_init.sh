# Log local kernel output into /var/log/kernel
dbterm 8 >>/var/log/kernel &

# Mount any additional drives
#mkdir /windows
#mount /dev/disk/bios/hda/0 /windows

ln -s home/root /root

# Check for network changes
netprefs --detect

# Configure the network
net_init.sh

# Start various daemons if they are installed

if [ -e /usr/bind/sbin/named ]; then
    /usr/bind/sbin/named
fi

if [ -e /usr/inetutils/sbin/inetd ]; then
    /usr/inetutils/sbin/inetd
fi
if [ -e /usr/sbin/crond ]; then
    /usr/sbin/crond
fi
if [ -e /usr/apache/bin/apachectl ]; then
    /usr/apache/bin/apachectl start
fi

