MODULE_NAME := multifiles

PWD := $(shell pwd)
KDIR := /lib/modules/`uname -r`/build

ccflags-y := -Iexample8_multiple_files/inc

obj-y += syscalls.o
obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-y := src/main.o src/proc.o src/thread.o src/syscall.o

default:
	echo $(MODULE)
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -f *.o *.ko *.mod.* Module.* modules.* src/*.o
