#!/bin/sh

IMG=qemu-image.img
DIR=tmp

mkdir tmp
mount -o loop $IMG $DIR

