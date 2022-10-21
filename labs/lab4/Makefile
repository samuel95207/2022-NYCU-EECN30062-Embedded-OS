ARCH?=arm64 
CROSS_COMPILE?=aarch64-linux-gnu-

obj-m += mydev.o
 
KDIR = ../../linux

ccflags-y := -std=gnu11 -Wno-declaration-after-statement

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(shell pwd) modules
	aarch64-linux-gnu-gcc writer.c -o writer
	aarch64-linux-gnu-gcc reader.c -o reader

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(shell pwd) clean
	rm writer
	rm reader

zip:
	rm -rf 311511034_eos_lab4
	rm -rf 311511034_eos_lab4.zip
	mkdir 311511034_eos_lab4
	cp Makefile 311511034_eos_lab4
	cp mydev.c 311511034_eos_lab4
	cp writer.c 311511034_eos_lab4
	cp reader.c 311511034_eos_lab4
	cp seg.py 311511034_eos_lab4
	cp demo.sh 311511034_eos_lab4
	zip -r 311511034_eos_lab4.zip 311511034_eos_lab4
	rm -rf 311511034_eos_lab4

