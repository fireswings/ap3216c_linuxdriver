CROSS_COMPILE := arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc
KERNELDIR := /home/lzy/linux/linux_4.1.15
OBJ = ap3216creg
CURRENT_PATH := $(shell pwd)

ifneq ($(KERNELRELEASE),)
obj-m := $(OBJ).o

else
build : kernel_modules
all : kernel_modules app_build

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

app_build: 
	$(CC) $(OBJ)app.c -o $(OBJ)app

cp_all : cp_app cp_modules

cp_app: app_build
	sudo cp $(OBJ)app ~/linux/nfs/rootfs/lib/modules/4.1.15/ -f
	
cp_modules: kernel_modules
	sudo cp $(OBJ).ko ~/linux/nfs/rootfs/lib/modules/4.1.15/ -f

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm -f $(OBJ)app
endif