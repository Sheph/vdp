#!/bin/bash
sudo apt-get install fakeroot kernel-wedge build-essential makedumpfile kernel-package libncurses5 libncurses5-dev libdw-dev
sudo apt-get build-dep --no-install-recommends linux-image-$(uname -r)
