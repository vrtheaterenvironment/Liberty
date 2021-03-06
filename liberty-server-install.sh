#!/bin/sh

# Liberty サーバーのインストール

# su

## ダウンロード

cd ~
yum install git libusb-devel -y
git clone https://github.com/vrtheaterenvironment/Liberty.git

## インストール

### LibertyのDriver

cd ~/Liberty/polhemusliberty-1.0.0/firmware_load/
./install_udev_firmware.sh
systemctl restart systemd-udevd.service
systemctl status systemd-udevd.service

### Liberty

cd ~/Liberty/polhemusliberty-1.0.0/src/
make

### libusb-1.0.0

cd ~/Liberty/libusb-1.0.0/
./configure
make install
ldconfig

### LibertyServer

cd ~/Liberty/LibertyServer/
make
mv ./server /usr/local/bin/

## 動作確認

### Libertyの実行（ドライバの動作確認）

# ~/Liberty/polhemusliberty-1.0.0/src/liberty

### LibertyServerの実行

# systemctl stop firewalld
# server
