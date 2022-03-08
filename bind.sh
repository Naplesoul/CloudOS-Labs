#!/bin/bash
sudo ifconfig ens33 down
sudo modprobe uio
cd /home/shen/Projects/DPDK/dpdk/kernel/linux/igb_uio
make
sudo insmod igb_uio.ko
cd ../../..
sudo usertools/dpdk-devbind.py --bind=igb_uio ens33
sudo usertools/dpdk-devbind.py -s
