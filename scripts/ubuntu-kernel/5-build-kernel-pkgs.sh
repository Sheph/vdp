#!/bin/bash
cd ~/kernel-src/linux-`uname -r | cut -f 1 -d-`
fakeroot debian/rules binary-headers binary-generic binary-perarch
cd ~/kernel-src
cat <<EOF > linux-update.sh
#!/bin/bash
sudo apt-get install libdw1 linux-tools-common linux-cloud-tools-common
sudo dpkg -i linux*.deb
EOF
chmod ugo+x linux-update.sh
