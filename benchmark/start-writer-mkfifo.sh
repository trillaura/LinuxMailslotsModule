#!/bin/bash
echo "sudo rm /dev/fifo"
sudo rm /dev/fifo
echo "taskset 0x2 sudo ./worker writer mkfifo fifo"
taskset 0x2 sudo ./worker writer mkfifo fifo
