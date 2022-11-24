lsb_release -a

sudo apt update

yes | sudo -S apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu

riscv64-linux-gnu-gcc --version

if [ $? -eq 0 ];then
    echo "Success!"
    tar xvf xv6-riscv.tar
    cd xv6-riscv
    make qemu
else
    echo "Err in riscv64-linux-gnu-gcc"
fi