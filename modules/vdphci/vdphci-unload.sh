#!/bin/bash
MODULE_NAME="vdphci"
DEVICE_NAME="vdphcidev"

sudo rmmod ./$MODULE_NAME.ko || exit 1

sudo rm -f /dev/${DEVICE_NAME}*
