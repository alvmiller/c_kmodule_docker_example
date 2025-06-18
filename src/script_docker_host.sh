#!/bin/bash

clear
reset

echo "Host Part..."
sleep 1
echo -e "\n"

make clean
make
sleep 1
echo -e "\n"

gcc -o client client.c
sleep 1
echo -e "\n"

xxd -i example_dev.ko >example_dev.ko.h
gcc -o init_drv_client init_drv_client.c
sleep 1
echo -e "\n"

sudo docker run -it --privileged --hostname docker --mount "type=bind,src=$PWD,dst=/root" ubuntu
