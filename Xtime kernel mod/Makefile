ifneq ($(KERNELRELEASE),)
	obj-m := xtime_proc.o
else
	KERNELDIR ?= /lib/modules/`uname -r`/build/
	PWD := `pwd`
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif

clean:
	rm -f *.ko *.o Module* *mod*
