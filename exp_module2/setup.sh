#!/bin/sh

module="scull"
device="scull"
mode="664"
major="260"
path="/dev"
tester="main"
logs="/var/log/kern.log"

help()
{
	echo ""
	echo "this shell is used to install scull module to linux"
	echo "rm_mod	"
	echo "setup_mod	"
	echo "print_dev	"
	echo "print_mod	"
	echo "test_mod	"
	echo "cat_log	"
	echo ""
}

print_dev()
{
	
	ls -l ${path}/${module}0
	ls -l ${path}/${module}1
}

print_mod()
{
	lsmod
}

cat_log()
{
	cat $logs
}

rm_mod()
{

	sudo rm -f ${path}/${device}0
	sudo rm -f ${path}/${device}1
	sudo rmmod ${module}
}

setup_mod()
{
	sudo insmod ${module}.ko

	sudo mknod ${path}/${module}0 c ${major} 0
	sudo mknod ${path}/${module}1 c ${major} 1

	sudo chmod $mode ${path}/${module}0
	sudo chmod $mode ${path}/${module}1

	print_dev
}

test_mod()
{
	sudo ./${tester}
}
