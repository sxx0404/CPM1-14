#! /bin/sh
rm -f /overlay/etc/config/*
mount -o remount /
cp -f /home/tcfg/wancfg/* /etc/config/
MAC=CPM1-`ifconfig | grep "br-lan    Link encap:Ethernet  HWaddr" | cut -b 39-55`
echo "$MAC"
sed -i "s/CPM1/$MAC/g" /etc/config/wireless
/etc/init.d/network restart
echo "Changed to Default Config."
exit 0

