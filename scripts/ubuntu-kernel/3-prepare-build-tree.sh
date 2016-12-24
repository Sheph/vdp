#!/bin/bash
cd ~/kernel-src/linux-`uname -r | cut -f 1 -d-`
chmod a+x debian/rules
chmod a+x debian/scripts/*
chmod a+x debian/scripts/misc/*
fakeroot debian/rules clean
