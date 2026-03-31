obj-m+=dht20.o

KDIR = /lib/modules/$(shell uname -r)/build

all: 
	make -C $(KDIR) M=$(shell pwd) modules

clean:
	make -C $(KDIR) M=$(shell pwd) clean
# dtc -@ -I dts -O dtb -o dht20.dtbo dht20-overlay.dts
# sudo cp dht20.dtbo /boot/overlays/
#sudo reboot