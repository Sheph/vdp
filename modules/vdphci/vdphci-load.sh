#!/bin/bash
MODULE_NAME="vdphci"
DEVICE_NAME="vdphcidev"
DEVICE_LOWER=0
DEVICE_UPPER=10

sudo insmod ./$MODULE_NAME.ko $* || exit 1

# Retrieve major number
DEVICE_MAJOR=`awk "\\$2==\"$MODULE_NAME\" {print \\$1}" /proc/devices`

sudo rm -f /dev/${DEVICE_NAME}*

for (( I=$DEVICE_LOWER; I<=$DEVICE_UPPER; I++ )) do
    sudo mknod /dev/${DEVICE_NAME}${I} c $DEVICE_MAJOR $I
    sudo chmod 0666 /dev/${DEVICE_NAME}${I}
done
