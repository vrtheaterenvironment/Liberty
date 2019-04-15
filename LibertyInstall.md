# Liberty サーバーのインストール

```sh
su
```

## ダウンロード

```sh
cd ~
yum install get
git clone https://github.com/vrtheaterenvironment/Liberty.git
```

## インストール

### LibertyのDriver

```
cd ~/Liberty/polhemusliberty-1.0.0/firmware_load/
.install_udev_firmware.sh
systemctl restart systemd-udevd.service
systemctl status systemd-udevd.service
```

### Liberty

```sh
cd ~/Liberty/polhemusliberty-1.0.0/src/
yum install libusb-devel
make
```

### libusb-1.0.0

```sh
cd ~/Liberty/libusb-1.0.0/
./configure
make install
ldconfig
```

### LibertyServer

```sh
cd ~/Liberty/LibertyServer/
make
mv ./server /usr/local/bin/
```

## 動作確認

### Libertyの実行（ドライバの動作確認）

```sh
~/Liberty/polhemusliberty-1.0.0/src/liberty
```

### LibertyServerの実行

```sh
systemctl stop firewalld
server
```
