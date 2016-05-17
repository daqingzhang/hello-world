mod_name=scull
tester_name="main"
tester_srcs="main.c"

ifneq ($(KERNELRELEASE),)
	obj-m := ${mod_name}.o
else

	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD	:= $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	gcc -o ${tester_name} ${tester_srcs}
#	gcc -o main main.c
clean:
	rm *.o *.ko .${mod_name}.ko.cmd ${mod_name}.mod.c .${mod_name}.o.cmd .${mod_name}.mod.o.cmd
	rm modules.order Module.symvers
	rm ${tester_name}
main:
	gcc -o ${tester_name} ${tester_srcs}
#	gcc -o main main.c
mclean:
	rm ${tester_name}

endif
