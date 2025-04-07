KERNELDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
obj-m = main.o

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@