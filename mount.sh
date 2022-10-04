DEVICE=sdb

sudo umount /dev/${DEVICE}1
sudo umount /dev/${DEVICE}2
rm -rf mnt
mkdir mnt
mkdir mnt/fat32
mkdir mnt/ext4
sudo mount /dev/${DEVICE}1 mnt/fat32
sudo mount /dev/${DEVICE}2 mnt/ext4