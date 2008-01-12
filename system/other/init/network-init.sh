#!/bin/sh
ifconfig eth0 down
echo "Enabling DHCP on interface eth0" >> /var/log/kernel
dhcpc -d eth0 2>>/var/log/dhcpc
