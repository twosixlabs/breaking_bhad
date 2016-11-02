#!/bin/bash

# http://wiki.emacinc.com/wiki/Mounting_JFFS2_Images_on_a_Linux_PC
## Script to mount jffs2 filesystem using mtd kernel modules.
## EMAC, Inc. 2009

if [[ $# -lt 2 ]]
then
    echo "Usage: $0 FSNAME.JFFS2 MOUNTPOINT [SIZE] [ERASEBLOCK_SIZE]"
    exit 1
fi

if [ "$(whoami)" != "root" ]
then
    echo "$0 must be run as root!"
    exit 1
fi

if [[ ! -e $1 ]]
then
    echo "$1 does not exist"
    exit 1
fi

if [[ ! -d $2 ]]
then
    echo "$2 is not a valid mount point"
    exit 1
fi

if [[ "$3" == "" ]]
then
        size="32768"
else
        size="$3"
fi

if [[ "$4" == "" ]]
then
        esize="128"
else
        esize="$4"
fi

# cleanup if necessary
umount /dev/mtdblock0 &>/dev/null
modprobe -r mtdram &>/dev/null
modprobe -r mtdblock &>/dev/null

modprobe mtdram total_size=$size erase_size=$esize || exit 1
modprobe mtdblock || exit 1
dd if="$1" of=/dev/mtdblock0 || exit 1
mount -t jffs2 -o compr=zlib /dev/mtdblock0 $2 || exit 1

echo "Successfully mounted $1 on $2"
exit 0
