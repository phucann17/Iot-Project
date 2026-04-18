obj-m+=bh1750.o

KDIR = /lib/modules/$(shell uname -r)/build

all: 
	make -C $(KDIR) M=$(shell pwd) modules

clean:
	make -C $(KDIR) M=$(shell pwd) clean
# dtc -@ -I dts -O dtb -o bh1750.dtbo bh1750-overlay.dts
# sudo cp bh1750.dtbo /boot/overlays/
#sudo nano /boot/firmware/config.txt
#sudo reboot