# Log local kernel output into /var/log/kernel
dbterm 8 >>/var/log/kernel &

# Mount any additional drives
#mkdir /windows
#mount /dev/disk/bios/hda/0 /windows

ln -s home/root /root

#Configure the first adaptor (IP address, netmask and gateway)
#ifconfig eth0 192.168.1.1 255.255.255.0

#Add gateway route(s)
#route add -i eth0 -g 192.168.1.1 192.168.1.0 255.255.255.0

#Configure the second adaptor
#ifconfig eth1 192.168.1.2 255.255.255.0
#route add -i eth1 -g 192.168.1.2 192.168.1.0 255.255.255.0

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

