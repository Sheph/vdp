#!/bin/bash
mkdir ~/kernel-src
cd ~/kernel-src
apt-get source linux-image-$(uname -r)
