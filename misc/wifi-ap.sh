#!/bin/sh

SSID="SyncParty"
PASS="0987654321"
IFACE=wlan0

if [ $(id -u) -ne 0 ]; then
	echo This must be run as root!
	exit 1
fi

ACTION=$1
shift

case $ACTION in
	off)
		create_ap --stop $IFACE
		systemctl start netctl-auto@$IFACE
	;;
	*)
		systemctl stop netctl-auto@$IFACE
		create_ap --daemon -n $IFACE "$SSID" "$PASS"
	;;
esac


