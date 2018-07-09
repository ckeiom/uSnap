#!/bin/sh

IMG=qemu-image.img
DIR=img

qemu-img create $IMG 1G
mkfs.ext2 $IMG
mkdir $DIR
mount -o loop $IMG $DIR
debootstrap --arch amd64 xenial $DIR
umount $DIR
