#!/bin/sh
/usr/local/share/PolhemusUsb/fxload -v -t fx2lp -D $(/usr/local/share/PolhemusUsb/getdevpath -v 0f44 -p ff21) -I /usr/local/share/PolhemusUsb/LbtyUsbHS.hex -s /usr/local/share/PolhemusUsb/a3load.hex
sleep 4
/usr/local/share/PolhemusUsb/PiTrkPerm
