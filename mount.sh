#!/bin/sh

IMG=qemu-image.img
DIR=mount_point

mkdir mount_point
mount -o loop $IMG $DIR

