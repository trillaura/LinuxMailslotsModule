#!/bin/bash
echo "sudo rm /dev/fifo"
sudo rm /dev/fifo
echo "sudo ./worker reader mkfifo fifo"
sudo ./worker reader mkfifo fifo
