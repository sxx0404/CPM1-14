#!/bin/sh
printf "Supported devices:\n1,CPM1-14       2,CPL2-01       3,EPT-WCT-32\n"
read -p "Please choose(1-3):" mode
if [ "$mode" -eq "1" ]; then
	mode="CPM1-14"
elif [ "$mode" -eq "2" ]; then
	mode="CPL2-01"
elif [ "$mode" -eq "3" ]; then
	mode="EPT-WCT-32"
else
	printf "Wrong option:%s\n" $mode
	exit 1
fi
printf "Device will be set to %s\n" $mode
read -p "Please input device mac(AABBCCDDEEFF):" mac
if /home/mac w $mac; then
	rm -f /overlay/etc/config/*
	mount -o remount /
	if [ $mode = "EPT-WCT-32" ]; then
		sed -i "s/^ProtVer=.*/ProtVer=22/" /home/syscfg.ini
	else
		sed -i "s/^ProtVer=.*/ProtVer=32/" /home/syscfg.ini
	fi
	sed -i "s/^Model=.*/Model=$mode/" /home/syscfg.ini
	if [ $mode = "CPM1-14" -o $mode = "CPL2-01" ]; then
		cp -f /home/tcfg/wancfg/* /etc/config/
		printf "Set device to %s successfully\n" $mode
		/etc/init.d/network restart
	else
		cp -f /home/tcfg/gcfg/* /etc/config/
		printf "Set device to %s successfully\n" $mode
	fi
	rm -f /home/init/init.0
else
	echo "Failed to setup device"
fi
