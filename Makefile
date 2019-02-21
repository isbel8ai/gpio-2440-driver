obj-m	:= gpio2440.o

KDIR := /usr/local/linux-3.1.10_mbti2440/
PWD       := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) SUBDIRS=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) SUBDIRS=$(PWD) clean
