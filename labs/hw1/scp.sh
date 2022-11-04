# RPI_IP=192.168.160.138
RPI_IP=raspberrypi.local

scp ./seg7_driver.ko pi@$RPI_IP:Embedded-OS/labs/hw1/seg7_driver.ko
scp ./led8_driver.ko pi@$RPI_IP:Embedded-OS/labs/hw1/led8_driver.ko
scp ./hw1 pi@$RPI_IP:Embedded-OS/labs/hw1/hw1
