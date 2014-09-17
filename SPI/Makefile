MODULE = spi4
KERNEL_SRC = /opt/LEC_3517-LINUX_BSP_V2.1/sm3517-psp-kernel 
CCPREFIX = arm-arago-linux-gnueabi-

obj-m += $(MODULE).o
module_upload=$(MODULE).ko

spi4-objs := spi_core.o spi_protocol.o spi_fops.o circular_buffer.o

all: clean compile install

compile :
	make ARCH=arm CROSS_COMPILE=$(CCPREFIX) -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	make ARCH=arm CROSS_COMPILE=$(CCPREFIX) -C $(KERNEL_SRC)  M=$(PWD) clean

# this just copies a file to LEC-3517 BOARD
install:
	scp $(module_upload) root@192.168.186.90:/home/root
