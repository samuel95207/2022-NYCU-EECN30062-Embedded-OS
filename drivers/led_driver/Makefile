ARCH?=arm64 
CROSS_COMPILE?=aarch64-linux-gnu-

obj-m += driver.o
 
KDIR = ../../linux

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(shell pwd) modules
clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(shell pwd) clean