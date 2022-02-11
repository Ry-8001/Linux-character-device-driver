obj-m := vigenere.o

run:compile_c
	./main

compile_c:
	gcc main.c -o main
all: init
	mknod /dev/vigenere0 c 241 0

init: compile
	insmod ./vigenere.ko vigenere_key='LINUX' 

compile:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules


delete: remove_node
	rm /dev/vigenere0
remove_node:
	rmmod vigenere
