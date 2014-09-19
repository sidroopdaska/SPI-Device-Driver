MODULE_1 = spi1
MODULE_2 = spi2
KERNEL_SRC = /opt/LEC_3517-LINUX_BSP_V2.1/sm3517-psp-kernel 
CCPREFIX = arm-arago-linux-gnueabi-

COMMON_OBJS = spi_core.o spi_protocol.o spi_fops.o circular_buffer.o

obj-m += $(MODULE_1).o
obj-m += $(MODULE_2).o

module_upload_1=$(MODULE_1).ko
module_upload_2=$(MODULE_2).ko

$(MODULE_1)-objs := $(COMMON_OBJS) spi_1.o
$(MODULE_2)-objs := $(COMMON_OBJS) spi_2.o

all: clean compile install

compile:
	make ARCH=arm CROSS_COMPILE=$(CCPREFIX) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	make ARCH=arm CROSS_COMPILE=$(CCPREFIX) -C $(KERNEL_SRC) M=$(PWD) clean

# this just copies a file to LEC-3517 BOARD
install:
	scp $(module_upload_1) root@192.168.186.90:/home/root
	scp $(module_upload_2) root@192.168.186.90:/home/root
