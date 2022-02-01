#!/bin/sh
DEVNAME=MyVito
SECDIR=../_secrets
DEVIP=`cat $SECDIR/devices_ip.txt | grep $DEVNAME | cut -d ' ' -f2`
DEVUSB=/dev/ttyUSB0
DEVPORT=$DEVIP
if [ ! -f ${PWD}/secrets.yaml ]
then
  ln -s $SECDIR/secrets.yaml .
fi
docker run --rm --net=host --name esphome -v "${PWD}":/config -v "${PWD}/`readlink ${PWD}/secrets.yaml`:/config/secrets.yaml" -it esphome/esphome compile $DEVNAME.yaml

