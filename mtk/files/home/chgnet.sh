#!/bin/sh
echo "Supportted network modes are:"
echo "1.3G USB mode."
echo "2.Wan DHCP mode."
read -p "Please choose network mode:" mode
if [ $mode -eq 1 ]; then
	rm -f /overlay/etc/config/*
	mount -o remount /
	cp -f /home/tcfg/gcfg/* /etc/config/
	/etc/init.d/network restart
	echo "Changed to 3G USB."
	exit 0
elif [ $mode -eq 2 ]; then
	rm -f /overlay/etc/config/*
	mount -o remount /
	cp -f /home/tcfg/wancfg/* /etc/config/
	/etc/init.d/network restart
	echo "Changed to Wan DHCP."
	exit 0
else
	printf "Wrong mode:%s\n" $mode
	exit 1
fi
