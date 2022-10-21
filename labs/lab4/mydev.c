#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define CHRDEV_NAME "seg16_Dev"
#define CLASS_NAME "seg16_class"
#define DEVICE_NAME "mydev"

#define TABLE_SIZE 27
#define BIT_COUNT 16
int seg_for_c[TABLE_SIZE] = {0b1111001100010001,  // A
                             0b0000011100000101,  // b
                             0b1100111100000000,  // C
                             0b0000011001000101,  // d
                             0b1000011100000001,  // E
                             0b1000001100000001,  // F
                             0b1001111100010000,  // G
                             0b0011001100010001,  // H
                             0b1100110001000100,  // I
                             0b1100010001000100,  // J
                             0b0000000001101100,  // K
                             0b0000111100000000,  // L
                             0b0011001110100000,  // M
                             0b0011001110001000,  // N
                             0b1111111100000000,  // O
                             0b1000001101000001,  // P
                             0b0111000001010000,  // q
                             0b1110001100011001,  // R
                             0b1101110100010001,  // S
                             0b1100000001000100,  // T
                             0b0011111100000000,  // U
                             0b0000001100100010,  // V
                             0b0011001100001010,  // W
                             0b0000000010101010,  // X
                             0b0000000010100100,  // Y
                             0b1100110000100010,  // Z
                             0b0000000000000000};

int output = 0;

dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;




// File Operations
static ssize_t my_read(struct file *fp, char *buf, size_t count, loff_t *fpos) {
    printk("call read\n");


    char output_buf[BIT_COUNT];
    for (int i = 0; i < BIT_COUNT; i++) {
        output_buf[i] = 0;
    }

    int num = output;
    for (int j = 0; num > 0; j++) {
        output_buf[BIT_COUNT - 1 - j] = num % 2;
        num = num / 2;
    }


    int datalen = sizeof(output_buf);
    if (copy_to_user(buf, &output_buf, count) > 0) {
        pr_err("ERROR: Not all the bytes have been copied to user\n");
    }
    

    pr_info("Read: %s\tSize: %d\n", output_buf, (int)datalen);

    return datalen;
}


static ssize_t my_write(struct file *fp, const char *buf, size_t count, loff_t *fpos) {
    printk("call write\n");
    uint8_t rec_buf[20] = {0};
    if (copy_from_user(rec_buf, buf, count) > 0) {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
    }
    char c = rec_buf[0];
    int index = TABLE_SIZE - 1;
    if (65 <= c && c <= 90) {
        index = c - 'A';
    } else if (97 <= c && c <= 122) {
        index = c - 'a';
    }

    output = seg_for_c[index];
    pr_info("Write: seg_for_c[%d] = %d\n", index, output);

    return count;
}
static int my_open(struct inode *inode, struct file *fp) {
    printk("call open\n");
    return 0;
}

struct file_operations my_fops = {read : my_read, write : my_write, open : my_open};

static int my_init(void) {
    printk("call init\n");
    if ((alloc_chrdev_region(&dev, 0, 1, CHRDEV_NAME)) < 0) {
        pr_err("Cannot allocate major number\n");
        goto r_unreg;
    }
    pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));
    /*Creating cdev structure*/
    cdev_init(&etx_cdev, &my_fops);
    /*Adding character device to the system*/
    if ((cdev_add(&etx_cdev, dev, 1)) < 0) {
        pr_err("Cannot add the device to the system\n");
        goto r_del;
    }
    /*Creating struct class*/
    if ((dev_class = class_create(THIS_MODULE, CLASS_NAME)) == NULL) {
        pr_err("Cannot create the struct class\n");
        goto r_class;
    }
    /*Creating device*/
    if ((device_create(dev_class, NULL, dev, NULL, DEVICE_NAME)) == NULL) {
        pr_err("Cannot create the Device \n");
        goto r_device;
    }

    pr_info("Device Driver Insert...Done!!!\n");

    output = seg_for_c[TABLE_SIZE - 1];


    return 0;

r_device:
    device_destroy(dev_class, dev);
r_class:
    class_destroy(dev_class);
r_del:
    cdev_del(&etx_cdev);
r_unreg:
    unregister_chrdev_region(dev, 1);
    return -1;
}

static void my_exit(void) {
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);
    printk("call exit\n");
}

module_init(my_init);
module_exit(my_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samuel Hunag <lspss95207@gmail.com>");
MODULE_DESCRIPTION("Virtual Segment Display Driver");
MODULE_VERSION("1.00");