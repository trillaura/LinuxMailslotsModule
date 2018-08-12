#!/bin/bash
echo "sudo rm /dev/mailslot"
sudo rm /dev/mailslot
echo "taskset 0x2 sudo ./worker writer mailslot mailslot 242 1"
taskset 0x2 sudo ./worker writer mailslot mailslot 242 1
