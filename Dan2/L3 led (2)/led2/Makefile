ifneq ($(KERNELRELEASE),)
obj-m := led.o
led-objs := ledmain.o utils.o
else
KDIR := ../../../../src/linux
all:
	$(MAKE) -C $(KDIR) M=$$PWD
clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
cleanall:
	$(MAKE) clean
	@rm -f *~
endif
