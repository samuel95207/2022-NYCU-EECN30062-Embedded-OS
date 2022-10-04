VERSION=patch

cd linux
KERNEL=kernel8
sudo env PATH=$PATH make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=../mnt/ext4 modules_install
cd ..

# sudo cp mnt/fat32/$KERNEL.img mnt/fat32/$KERNEL-backup.img
sudo cp linux/arch/arm64/boot/Image mnt/fat32/$KERNEL-myconfig-${VERSION}.img
sudo cp linux/arch/arm64/boot/dts/broadcom/*.dtb mnt/fat32/
sudo cp linux/arch/arm64/boot/dts/overlays/*.dtb* mnt/fat32/overlays/
sudo cp linux/arch/arm64/boot/dts/overlays/README mnt/fat32/overlays/
sudo umount mnt/fat32
sudo umount mnt/ext4