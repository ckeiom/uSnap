#!/bin/sh

IMG=qemu_new.img
DIR=tmp

qemu-img create $IMG 90m
mkfs.ext2 $IMG
mkdir $DIR
mount -o loop $IMG $DIR
debootstrap --arch amd64 jessie $DIR
umount $DIR
rmdir $DIR
