# Liberty サーバーのインストール

## Driverのダウンロード
1. tar xzvf polhemusliberty-1.0.0.tar.gz
    - https://sourceforge.net/projects/polhemuslilberty/ からダウンロードできる

1. `yum install libusb-devel`
1. `cd polhemusliberty-1.0.0.tar.gz/firmware_load/`
1. ルールファイルを書き換える
    - `BUS` → `SUBSYSTEM`
    - `SYSFS` → `ATTRS`
1. install_udev_firmware.shを書き換える
    - 変更前: `udevcontrol reload_rules`
    - 変更後: `udevadm control --reload-rules`
1. install_udev_firmware.sh を実行
1. `systemctl restart systemd-udevd.service`
1. `systemctl status systemd-udevd.service`

## Libertyのコンパイル
1. `cd ../src`
1. main.c に `#include <stdint.h>`を追加
1. `make`
1. `./liberty`で値を取れることを確認

## libusb-1.0.0のコンパイル
1. `tar xjvf libusb-1.0.0.tar.bz2`
1. `cd libusb-1.0.0`
1. `.configure`
1. `make install`
1. `ldconfig`

## LibertyServerのインストール
1. `tar xzvf LibertyServer.tar.gz`
1. `cd LiberyServer`
1. `make`
1. `mv ./server /usr/local/bin/

## LibertyServerの実行
1. `systemctl stop firewalld`
1. `server`
