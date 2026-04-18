#include <linux/module.h>      // Support for writing Linux Kernel Module (MODULE_LICENSE, module_init, module_exit,…)
#include <linux/init.h>        // Support for function __init, __exit cho init/exit function
#include <linux/kernel.h>      // Kernel function: printk(), KERN_INFO, container_of(), …
#include <linux/fs.h>          // File operations: struct file_operations, register_chrdev, open/read/write/ioctl
#include <linux/kdev_t.h>      // Create major/minor number: MAJOR(), MINOR(), MKDEV()
#include <linux/device.h>      // Create device node in /dev: class_create(), device_create()
#include <linux/cdev.h>        // Character device: struct cdev, cdev_init(), cdev_add()
#include <linux/delay.h>       // delay: msleep(), udelay(), mdelay()
#include <linux/i2c.h>         // I2C driver: struct i2c_client, i2c_master_send(), i2c_master_recv(), probe/remove
#include <linux/slab.h>        // Memory management in kernel: kmalloc(), kfree(), kzalloc()
#include <linux/uaccess.h>     // Communicate user-space: copy_to_user(), copy_from_user()

#define MEM_SIZE 1024
#define BH1750_ADDR 0x23
#define DEVICE_NAME "BH1750"
#define DRIVER_NAME "bh1750_driver"
#define MAX_LUX 1000
// Power
#define POWER_DOWN        0x00
#define POWER_ON          0x01
#define RESET             0x07

// Continuous mode
#define CONTINUOUS_H_RES  0x10 //resolution = 1 120ms
#define CONTINUOUS_H_RES2 0x11 //resolution = 0.5 120ms
#define CONTINUOUS_L_RES  0x13 //resolution = 4 16ms

// One-time mode
#define ONETIME_H_RES     0x20
#define ONETIME_H_RES2    0x21
#define ONETIME_L_RES     0x23

struct bh1750_data{
    struct i2c_client *client;
    dev_t devt;
    struct cdev cdev;
    struct device *device;
};

static struct class *bh1750_class;
//file operation
static int bh1750_open(struct inode* inode, struct file* file);
static int bh1750_release(struct inode* inode, struct file* file);
static ssize_t bh1750_read(struct file* file, char __user *buff, size_t count, loff_t *offset);
static ssize_t bh1750_write(struct file* file, const char __user *buff, size_t count, loff_t *offset);

//i2c driver
static int bh1750_probe(struct i2c_client *client);
static void bh1750_remove(struct i2c_client *client);

//logic function
static int bh1750_device_init(struct i2c_client *client);
static int bh1750_read_data(struct i2c_client *client, uint8_t *buf);

static const struct file_operations bh1750_fops = {
    .owner = THIS_MODULE,
    .open = bh1750_open,
    .release = bh1750_release,
    .read = bh1750_read,
    .write = bh1750_write
};

static const struct of_device_id bh1750_of_match[] = {
    {.compatible = "bh1750"},
    { }
};

MODULE_DEVICE_TABLE(of, bh1750_of_match);

static int bh1750_probe(struct i2c_client *client)
{
    pr_info("BH1750 probe called\n");
    int ret;
    struct bh1750_data *data = kmalloc(sizeof(*data), GFP_KERNEL);;

    if (!data) {
        pr_err("Cannot allocate memory for dht20_data\n");
        return -1;
    }
        
    data->client = client;
    i2c_set_clientdata(client, data);

    // alloc major/minor
    ret = alloc_chrdev_region(&data->devt, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        pr_err("alloc_chrdev_region failed\n");
        goto err_alloc;
    }

    pr_info("MAJOR=%d MINOR=%d\n", MAJOR(data->devt), MINOR(data->devt));

    // init cdev
    cdev_init(&data->cdev, &bh1750_fops);
    data->cdev.owner = THIS_MODULE;

    ret = cdev_add(&data->cdev, data->devt, 1);
    if (ret < 0) {
        pr_err("cdev_add failed\n");
        goto err_chrdev;
        return -1;
    }

    // create struct class
    bh1750_class = class_create("bh1750_class");
    if (IS_ERR(bh1750_class)){
        pr_err("Cannot create the struct class for dht20\n");
        return -1;
    }

    // create device
    data->device = device_create(bh1750_class, NULL,
                                 data->devt, NULL,
                                 DEVICE_NAME);

    if (IS_ERR(data->device)) {
        ret = PTR_ERR(data->device);
        pr_err("device_create failed\n");
        goto err_device;
        return -1;
    }

    // init sensor
    ret = bh1750_device_init(client);
    if (ret < 0) {
        pr_err("BH1750 init failed\n");
        goto err_device;
        return -1;
    }

    pr_info("BH1750 probe success\n");
    return 0;

/* ================= ERROR HANDLING ================= */

err_device:
    device_destroy(bh1750_class, data->devt);

err_cdev:
    cdev_del(&data->cdev);

err_chrdev:
    unregister_chrdev_region(data->devt, 1);

err_alloc:
    // i2c_set_clientdata(client, NULL);
    kfree(data);

err_class:
    class_destroy(bh1750_class);

    return ret;
}

static void bh1750_remove(struct i2c_client *client)
{
    struct bh1750_data *data = i2c_get_clientdata(client);

    device_destroy(bh1750_class, data->devt);
    class_destroy(bh1750_class);
    cdev_del(&data->cdev);
    unregister_chrdev_region(data->devt, 1);

    kfree(data);

    pr_info("BH1750 removed\n");
}

static struct i2c_driver bh1750_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = bh1750_of_match,
    },
    .probe = bh1750_probe,
    .remove = bh1750_remove,
};
static int __init bh1750_init(void){
    pr_info("BH1750 module inserted successfully\n");
    return i2c_add_driver(&bh1750_driver);
}

static void __exit bh1750_exit(void){
    i2c_del_driver(&bh1750_driver);
    pr_info("BH1750 module removed successfully.\n");
}

static ssize_t bh1750_write(struct file* file, const char __user *buff, size_t count, loff_t *offset){
    pr_info("Driver BH1750 Written!\n");
    return 0;
}

static int bh1750_device_init(struct i2c_client *client)
{
    int ret;
    uint8_t cmd;

    msleep(1000);

    // POWER ON
    cmd = POWER_ON;
    ret = i2c_master_send(client, &cmd, 1);
    if (ret != 1) {
        pr_err("POWER_ON failed ret=%d\n", ret);
        return -EIO;
    }

    msleep(200);

    // RESET
    cmd = RESET;
    ret = i2c_master_send(client, &cmd, 1);
    if (ret != 1) {
        pr_err("RESET failed ret=%d\n", ret);
        return -EIO;
    }

    msleep(50);

    // SET MODE
    cmd = CONTINUOUS_H_RES;
    ret = i2c_master_send(client, &cmd, 1);
    if (ret != 1) {
        pr_err("SET MODE failed ret=%d\n", ret);
        return -EIO;
    }

    msleep(180);

    pr_info("BH1750 init OK\n");
    return 0;
}

static int bh1750_read_data(struct i2c_client *client, uint8_t *buf)
{
    int ret;
    msleep(200);
    ret = i2c_master_recv(client, buf, 2);
    if (ret < 0) {
        pr_err("BH1750 read failed\n");
        return ret;
    }

    return 0;
}
static ssize_t bh1750_read(struct file *file, char __user *buf,
                           size_t count, loff_t *offset)
{
    struct bh1750_data *data = file->private_data;
    uint8_t raw[2];
    int ret;
    int lux;
    char out[32];
    int len;

    pr_info("BH1750 read called\n");

    // raw
    ret = bh1750_read_data(data->client, raw);
    if (ret < 0) return ret;
    int percent;
    // convert raw lux
    lux = ((raw[0] << 8) | raw[1]) * 10 / 12; //avoid overflow

    // convert to string
    if (lux > MAX_LUX) lux = MAX_LUX;

    percent = (lux * 100) / MAX_LUX;
    len = snprintf(out, sizeof(out), "%d\n", percent);

    // copy to user
    if (copy_to_user(buf, out, len))
        return -EFAULT;

    *offset += len;
    return len;
}

static int bh1750_open(struct inode *inode, struct file *file){
    struct bh1750_data *data;

    data = container_of(inode->i_cdev, struct bh1750_data, cdev);
    file->private_data = data;
    pr_info("device opened\n");
    return 0;
}

static int bh1750_release(struct inode* init, struct file* file){
    pr_info("BH1750 released!\n");
    return 0;
}

module_init(bh1750_init);
module_exit(bh1750_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("AN NGUYEN");
MODULE_DESCRIPTION("simple linux device driver for bh1750");
MODULE_VERSION("1.0");