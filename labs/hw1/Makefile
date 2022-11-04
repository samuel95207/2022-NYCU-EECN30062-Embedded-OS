ARCH?=arm64 
CROSS_COMPILE?=aarch64-linux-gnu-

obj-m += led8_driver.o
obj-m += seg7_driver.o
 
KDIR = ../../linux

ccflags-y := -std=gnu11 -Wno-declaration-after-statement

all: drivers hw1

hw1:
	aarch64-linux-gnu-gcc hw1.c -o hw1

drivers:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(shell pwd) modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(shell pwd) clean
	rm -f hw1

zip:
	rm -rf 311511034_eos_hw1
	rm -rf 311511034_eos_hw1.zip
	mkdir 311511034_eos_hw1
	cp Makefile 311511034_eos_hw1
	cp hw1.c 311511034_eos_hw1
	cp led8_driver.c 311511034_eos_hw1
	cp seg7_driver.c 311511034_eos_hw1
	zip -r 311511034_eos_hw1.zip 311511034_eos_hw1
	rm -rf 311511034_eos_hw1
