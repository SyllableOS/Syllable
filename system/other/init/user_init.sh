# Log local kernel output into /var/log/kernel
dbterm 8 >>/var/log/kernel &

# Create /root for the convience of the root user
ln -s home/root /root

# Check for network changes
/Applications/Preferences/Network --detect

# Configure the network
/system/net_init.sh

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

