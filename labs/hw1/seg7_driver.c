#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#define CHRDEV_NAME "seg7_Dev"
#define CLASS_NAME "seg7_class"
#define DEVICE_NAME "seg7"

#define GPIO_10 (10)
#define GPIO_11 (11)
#define GPIO_12 (12)
#define GPIO_13 (13)
#define GPIO_14 (14)
#define GPIO_15 (15)
#define GPIO_16 (16)
#define GPIO_17 (17)

#define TABLE_SIZE 11
#define BIT_COUNT 8

int gpioArr[BIT_COUNT] = {GPIO_17, GPIO_10, GPIO_11, GPIO_12, GPIO_13, GPIO_14, GPIO_15, GPIO_16};

int seg_for_c[TABLE_SIZE] = {0b01111110,  // 0
                             0b00110000,  // 1
                             0b01101101,  // 2
                             0b01111001,  // 3
                             0b00110011,  // 4
                             0b01011011,  // 5
                             0b01011111,  // 6
                             0b01110000,  // 7
                             0b01111111,  // 8
                             0b01111011,  // 9
                             0b00000000};


dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;




// File Operations
static ssize_t my_read(struct file *fp, char *buf, size_t count, loff_t *fpos) {
    pr_info("call read\n");
    return 0;
}


static ssize_t my_write(struct file *fp, const char *buf, size_t count, loff_t *fpos) {
    pr_info("call write\n");
    // for (int i = 0; i < BIT_COUNT; i++) {
    //     // gpio_set_value(gpioArr[i], bin[i]);
    //     gpio_set_value(gpioArr[i], 1);
    // }
    uint8_t rec_buf[32] = {0};
    if (copy_from_user(rec_buf, buf, count) > 0) {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
    }

    for (int i = 0; rec_buf[i] != 0; i++) {
        char c = rec_buf[i];

        int num = c - '0';

        int code;
        if (c == 'x') {
            code = seg_for_c[11];
        } else if (num >= 0 && num < 10) {
            code = seg_for_c[num];
        }


        int bin[BIT_COUNT];
        for (int i = 0; i < BIT_COUNT; i++) {
            bin[i] = 0;
        }

        for (int i = 0; code > 0; i++) {
            bin[BIT_COUNT - 1 - i] = code % 2;
            code = code / 2;
        }

        for (int i = 0; i < BIT_COUNT; i++) {
            gpio_set_value(gpioArr[i], bin[i]);
        }

        mdelay(1000);
    }

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
    // Checking the GPIO is valid or not
    for (int i = 0; i < BIT_COUNT; i++) {
        if (gpio_is_valid(gpioArr[i]) == false) {
            pr_err("GPIO_%d is not valid\n", gpioArr[i]);
            goto r_device;
        }
    }
    // Requesting the GPIO
    for (int i = 0; i < BIT_COUNT; i++) {
        char label[] = "GPIO_%d!";
        printk(label, gpioArr[i]);
        if (gpio_request(gpioArr[i], label) < 0) {
            pr_err("ERROR: GPIO_%d request\n", gpioArr[i]);
            goto r_gpio;
        }
    }
    // configure the GPIO as output
    for (int i = 0; i < BIT_COUNT; i++) {
        gpio_direction_output(gpioArr[i], 0);
    }
    for (int i = 0; i < BIT_COUNT; i++) {
        gpio_export(gpioArr[i], false);
    }

    pr_info("Device Driver Insert...Done!!!\n");
    return 0;
r_gpio:
    for (int i = 0; i < BIT_COUNT; i++) {
        gpio_free(gpioArr[i]);
    }
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
    for (int i = 0; i < BIT_COUNT; i++) {
        gpio_unexport(gpioArr[i]);
    }
    for (int i = 0; i < BIT_COUNT; i++) {
        gpio_free(gpioArr[i]);
    }
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
MODULE_DESCRIPTION("Seven Segment Display Driver");
MODULE_VERSION("1.00");