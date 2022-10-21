# RPI_IP=192.168.160.138
RPI_IP=raspberrypi.local

scp ./mydev.ko pi@$RPI_IP:Embedded-OS/labs/lab4/mydev.ko
scp ./writer pi@$RPI_IP:Embedded-OS/labs/lab4/writer
scp ./reader pi@$RPI_IP:Embedded-OS/labs/lab4/reader
