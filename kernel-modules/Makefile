#Makefile
obj-m   += hermeslink.o
obj-m   += mycelium.o
obj-m   += cansim.o

KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
#        ../../scripts/sign-file sha256 /lib/modules/5.15.0-52-generic/build/certs/priv.key /lib/modules/5.15.0-52-generic/build/certs/pubkey.der ./hermeslink.ko
clean:
	rm -rf *.o *.ko *.mod.* *.cmd .module* modules* Module* .*.cmd .tmp*
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
