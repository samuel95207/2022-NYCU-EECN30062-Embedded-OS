#!/bin/sh

set -x
# set -e

rmmod -f mydev
insmod mydev.ko

./writer abcd &
./reader 192.168.0.12 8688 /dev/mydev
