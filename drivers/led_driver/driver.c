/*****************************************************************************
 **
 * \file led_driver.c
 * \details Simple GPIO driver explanation
 * \author EmbeTronicX
 * \Tested with Linux
 * raspberrypi 5.4.51-v7l+
 ******************************************************************************
 */

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>  //GPIO
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>  //copy_to/from_user()
// LED is connected to this GPIO
#define GPIO_21 (21)
#define GPIO_22 (22)
#define GPIO_23 (23)
#define GPIO_24 (24)

int gpioArr[4] = {GPIO_21, GPIO_22, GPIO_23, GPIO_24};

dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
/*************** Driver functions **********************/
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t etx_write(struct file *filp, const char *buf, size_t len, loff_t *off);
/******************************************************/
// File operation structure
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = etx_read,
    .write = etx_write,
    .open = etx_open,
    .release = etx_release,
};
/*
** This function will be called when we open the Device file
*/
static int etx_open(struct inode *inode, struct file *file) {
    pr_info("Device File Opened...!!!\n");
    return 0;
}
/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file) {
    pr_info("Device File Closed...!!!\n");
    return 0;
}
/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    uint8_t gpio_state = 0;
    // reading GPIO value
    // write to user
    for(int i = 0;i < 4;i++){
        int gpio = gpioArr[i];
        gpio_state = gpio_get_value(gpio);
        len = 1;
        if (copy_to_user(buf, &gpio_state, len) > 0) {
            pr_err("ERROR: Not all the bytes have been copied to user\n");
        }
        pr_info("Read function : GPIO_%d = %d \n", gpio, gpio_state);
    }
    return 0;
}
/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    uint8_t rec_buf[20] = {0};
    if (copy_from_user(rec_buf, buf, len) > 0) {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
    }
    
    for (int i = 0; rec_buf[i] != '\0'; i++) {
        char c = rec_buf[i];
        int num = c - '0';
        int bin[4] = {0, 0, 0, 0};
        if (num >= 0 && num < 10) {
            pr_info("%d\n", num);

            for (int j = 0; num > 0; j++) {
                bin[3 - j] = num % 2;
                num = num / 2;
            }
            pr_info("%d%d%d%d\n", bin[0], bin[1], bin[2], bin[3]);

            for (int j = 0; j < 4; j++) {
                gpio_set_value(gpioArr[j], bin[j]);
            }
            mdelay(1000);
            for (int j = 0; j < 4; j++) {
                gpio_set_value(gpioArr[j], 0);
            }
            mdelay(100);
        }else{
            pr_err("Unknown char: %c is not a digit\n", c);
        }
        for (int j = 0; j < 4; j++) {
            gpio_set_value(gpioArr[j], 0);
        }
    }

    return len;
}
/*
** Module Init function
*/
static int __init etx_driver_init(void) {
    /*Allocating Major number*/
    if ((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) < 0) {
        pr_err("Cannot allocate major number\n");
        goto r_unreg;
    }
    pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));
    /*Creating cdev structure*/
    cdev_init(&etx_cdev, &fops);
    /*Adding character device to the system*/
    if ((cdev_add(&etx_cdev, dev, 1)) < 0) {
        pr_err("Cannot add the device to the system\n");
        goto r_del;
    }
    /*Creating struct class*/
    if ((dev_class = class_create(THIS_MODULE, "etx_class")) == NULL) {
        pr_err("Cannot create the struct class\n");
        goto r_class;
    }
    /*Creating device*/
    if ((device_create(dev_class, NULL, dev, NULL, "etx_device")) == NULL) {
        pr_err("Cannot create the Device \n");
        goto r_device;
    }
    // Checking the GPIO is valid or not
    for (int i = 0; i < 4; i++) {
        if (gpio_is_valid(gpioArr[i]) == false) {
            pr_err("GPIO_%d is not valid\n", gpioArr[i]);
            goto r_device;
        }
    }
    // Requesting the GPIO
    for (int i = 0; i < 4; i++) {
        char label[] = "GPIO_%d!";
        printk(label, gpioArr[i]);
        if (gpio_request(gpioArr[i], label) < 0) {
            pr_err("ERROR: GPIO_%d request\n", gpioArr[i]);
            goto r_gpio;
        }
    }
    // configure the GPIO as output
    for (int i = 0; i < 4; i++) {
        gpio_direction_output(gpioArr[i], 0);
    }
    /* Using this call the GPIO 21 will be visible in /sys/class/gpio/
    ** Now you can change the gpio values by using below commands also.
    ** echo 1 > /sys/class/gpio/gpio21/value (turn ON the LED)
    ** echo 0 > /sys/class/gpio/gpio21/value (turn OFF the LED)
    ** cat /sys/class/gpio/gpio21/value (read the value LED)
    **
    ** the second argument prevents the direction from being changed.
    */
    for (int i = 0; i < 4; i++) {
        gpio_export(gpioArr[i], false);
    }

    pr_info("Device Driver Insert...Done!!!\n");
    return 0;
r_gpio:
    for (int i = 0; i < 4; i++) {
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
/*
** Module exit function
*/
static void __exit etx_driver_exit(void) {
    for (int i = 0; i < 4; i++) {
        gpio_unexport(gpioArr[i]);
    }
    for (int i = 0; i < 4; i++) {
        gpio_free(gpioArr[i]);
    }
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("Device Driver Remove...Done!!\n");
}
module_init(etx_driver_init);
module_exit(etx_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeTronicX <embetronicx@gmail.com>");
MODULE_DESCRIPTION("A simple device driver - GPIO Driver");
MODULE_VERSION("1.32");