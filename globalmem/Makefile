obj-m += globalmem.o

build: kernel_modules

#EXTRA_CFLAGES=-g -O0

kernel_modules:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
