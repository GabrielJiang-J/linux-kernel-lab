```
mkdir rootfs
cd rootfs
cp cp ../busybox-1.33.0/_install/* .
ln -s bin/busybox init
mkdir -pv {bin,sbin,etc,proc,sys,usr/{bin,sbin},dev}

cd etc
cat <<EOF > inittab
> ::sysinit:/etc/init.d/rcS
> ::askfirst:-/bin/sh
> ::restart:/sbin/init
> ::ctrlaltdel:/sbin/reboot
> ::shutdown:/bin/umount -a -r
> ::shutdown:/sbin/swapoff -a
> EOF
chmod +x inittab

mkdir init.d
cd init.d
cat <<EOF > rcS
> #!/bin/sh
> mount proc
> mount -o remount,rw /
> mount -a
> clear
> echo "My Tiny Linux Starting, press enter to active"
> EOF

chmod +x rcS

touch fstab
cat <<EOF > fstab
> #/etc/fstab
> proc            /proc        proc    defaults          0       0
> sysfs           /sys         sysfs   defaults          0       0
> devtmpfs        /dev         devtmpfs  defaults          0       0
> EOF

cd rootfs

find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.img

qemu-system-x86_64 -kernel ./bzImage -initrd ./initramfs.img -hdc disk.img -append console=ttyS0 -nographic -S -s
```
