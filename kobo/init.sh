#!/bin/sh
ifconfig lo 127.0.0.1
# Alias should be set in /etc/profile
#alias ldd="LD_TRACE_LOADED_OBJECTS=1 $1"

/bin/mkdir -p /dev/pts
/bin/mount -t devpts devpts /dev/pts

#Wifi
insmod /drivers/mx6sll-ntx/wifi/mlan.ko
insmod /drivers/mx6sll-ntx/wifi/moal.ko mod_para=nxp/wifi_mod_para_sd8987.conf
insmod /drivers/mx6sll-ntx/wifi/sdio_wifi_pwr.ko
sleep 2
/sbin/ifconfig mlan0 up
/bin/wlarm_le -i mlan0 up
/bin/wpa_supplicant -i mlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf -C /var/run/wpa_supplicant -B -D nl80211
sleep 2
/sbin/udhcpc -S -i mlan0 -s /etc/udhcpc.d/default.script -t15 -T10 -A3 -f -q 
/usr/sbin/telnetd -l /bin/sh &
/usr/bin/tcpsvd -E 0.0.0.0 21 ftpd -w / &


