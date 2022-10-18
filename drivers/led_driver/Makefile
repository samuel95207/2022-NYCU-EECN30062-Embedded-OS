ARCH?=arm64 
CROSS_COMPILE?=aarch64-linux-gnu-

obj-m += driver.o
 
KDIR = ../../linux

ccflags-y := -std=gnu11 -Wno-declaration-after-statement

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(shell pwd) modules
	aarch64-linux-gnu-gcc writer.c -o writer
clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(shell pwd) clean
	rm writer