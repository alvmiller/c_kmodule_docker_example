#!/bin/bash

clear
reset

echo "Starting..."
sleep 1
echo -e "\n"

make clean
make
sleep 1
echo -e "\n"

gcc -o client client.c
sleep 1
echo -e "\n"

gcc -o client_host -DHOST_DEV_PROC_DRV client.c
sleep 1
echo -e "\n"

xxd -i example_dev.ko >example_dev.ko.h
gcc -o init_drv_client init_drv_client.c
sleep 1
echo -e "\n"
