#!/bin/sh

qemu-system-x86_64 	-curses \
					-kernel linux-4.9.27/arch/x86_64/boot/bzImage \
					-append "root=/dev/sda single" \
					-hda qemu-image.img \
					-m 2G \
					-serial file:log
