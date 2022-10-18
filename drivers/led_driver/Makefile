ARCH?=arm64 
CROSS_COMPILE?=aarch64-linux-gnu-

obj-m += lab3_driver.o
 
KDIR = ../../linux

ccflags-y := -std=gnu11 -Wno-declaration-after-statement

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(shell pwd) modules
	aarch64-linux-gnu-gcc writer.c -o writer
clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(shell pwd) clean
	rm writer
zip:
	rm -rf 311511034_eos_lab3
	rm -rf 311511034_eos_lab3.zip
	mkdir 311511034_eos_lab3
	cp Makefile 311511034_eos_lab3
	cp writer.c 311511034_eos_lab3
	cp lab3_driver.c 311511034_eos_lab3
	zip -r 311511034_eos_lab3.zip 311511034_eos_lab3
	rm -rf 311511034_eos_lab3

