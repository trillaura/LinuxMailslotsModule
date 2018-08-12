#!/bin/bash
echo "sudo rm /dev/mailslot"
sudo rm /dev/mailslot
echo "sudo ./worker reader mailslot mailslot 242 1"
sudo ./worker reader mailslot mailslot 242 1
