#!/bin/sh

set -x
# set -e

rmmod -f mydev
insmod mydev.ko

./writer samuel &
./reader 127.0.0.1 8080 /dev/mydev
