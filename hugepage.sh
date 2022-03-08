#!/bin/bash
mkdir -p /dev/hugepages
mountpoint -q /dev/hugepages || mount -t hugetlbfs nodev /dev/hugepages
sudo sh -c 'echo 64 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages'