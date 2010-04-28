ifconfig eth0 down
echo "Activating DHCP on network interface eth0" >> /var/log/kernel
dhcpc -d eth0 2>> /var/log/dhcpc
