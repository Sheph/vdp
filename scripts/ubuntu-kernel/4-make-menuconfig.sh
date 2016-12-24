#!/bin/bash
cd ~/kernel-src/linux-`uname -r | cut -f 1 -d-`
fakeroot debian/rules editconfigs
