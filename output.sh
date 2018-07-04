#!/bin/sh

qemu-system-x86_64 	-nographic \
					-kernel ./linux-4.9.27/arch/x86_64/boot/bzImage \
					-append "root=/dev/sda single console=ttyS0" \
					-hda qemu-image.img
#-initrd obj/initramfs-busybox-x86.cpio.gz
