# RPI_IP=192.168.160.138
RPI_IP=raspberrypi.local

scp ./lab3_driver.ko pi@$RPI_IP:Embedded-OS/labs/lab3/lab3_driver.ko
scp ./writer pi@$RPI_IP:Embedded-OS/labs/lab3/writer