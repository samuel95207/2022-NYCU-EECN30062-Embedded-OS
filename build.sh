CONFIG=bcm2711_defconfig
# CONFIG=menuconfig

cd linux
KERNEL=kernel8
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- ${CONFIG}
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image modules dtbs