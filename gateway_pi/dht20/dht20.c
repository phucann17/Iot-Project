#include <linux/module.h> //module
#include <linux/init.h>  
#include <linux/kernel.h> //printk
#include <linux/i2c.h> //i2c driver
#include <linux/fs.h> //file operation
#include <linux/kdev_t.h> //major&minor
#include <linux/device.h> //device node/ device file
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/slab.h> //kenel malloc
#include <linux/uaccess.h> //copy user space

#define MEM_SIZE 1024
#define DHT20_ADDR 0x38
#define DRIVER_NAME "dht20_driver"
#define DEVICE_NAME "dht20"
#define CLASS_NAME "dht20"

struct dht20_data{
    struct i2c_client *client;
    dev_t devt; //have major and minor number
    struct cdev cdev;
    struct device *device;
};

static struct class *dht20_class;

//file operation
static int dht20_open(struct inode* inode, struct file* file);
static int dht20_release(struct inode* inode, struct file* file);
static ssize_t dht20_read(struct file* file, char __user *buff, size_t count, loff_t *offset);

//i2c binding
static int dht20_probe(struct i2c_client *client);
static void dht20_remove(struct i2c_client *client);

//logic sensor function
static int dht20_device_init(struct i2c_client *client);
static int dht20_device_read(struct i2c_client *client, uint8_t *buf);

static const struct file_operations dht20_fops = {
    .owner = THIS_MODULE,
    .open = dht20_open,
    .release = dht20_release,
    .read = dht20_read,
};

/*
 Device Tree match table
 This table is used to match the driver with devices described in the Device Tree.
 
 The kernel compares the "compatible" string in the Device Tree with this table.
 If a match is found, the probe() function will be called.
*/
static const struct of_device_id dht20_of_match[] = {
    { .compatible = "aosong,dht20" },
    { }
};

/*
 Export the Device Tree match table to userspace.
 This allows tools like udev/modprobe to automatically load the driver
 when a matching device is detected.
*/
MODULE_DEVICE_TABLE(of, dht20_of_match);

/*
 I2C driver structure
 This structure defines how the driver interacts with the I2C subsystem.
*/
static struct i2c_driver dht20_driver = {
    .driver = {
        .name = DRIVER_NAME, // Name of the driver (visible in /sys/bus/i2c/drivers/)
        .of_match_table = dht20_of_match, // Device Tree matching table
    },
    .probe = dht20_probe,
    .remove = dht20_remove,
};

// Probe function is called when the I2C device matches this driver
// (based on device tree or id_table)
static int dht20_probe(struct i2c_client *client){
    pr_info("DHT20 probe function was called!\n");
    int ret;
    struct dht20_data *data = kmalloc(sizeof(*data), GFP_KERNEL);

    if (!data){
        pr_err("Cannot allocate memory for dht20_data!\n");
        return -ENOMEM;
    }

    data->client = client;
    //assign data to client
    i2c_set_clientdata(client, data);

    //alloc major and minor number
    ret = alloc_chrdev_region(&data->devt, 0, 1, DRIVER_NAME);
    if (ret < 0){
        pr_err("Allocating major and minor number failed!\n");
        goto err_alloc;
    }

    pr_info("MAJOR=%d MINOR=%d\n", MAJOR(data->devt), MINOR(data->devt));

    //cdev for add file operations
    // Initialize cdev structure and bind it with file_operations
    // => tells the kernel which functions to call (read/write/open/...)
    cdev_init(&data->cdev, &dht20_fops);
    // Set module owner (prevents module from being unloaded while in use)
    data->cdev.owner = THIS_MODULE;
    // Add cdev to the kernel and associate it with devt (major/minor)
    // => kernel now maps: (major, minor) -> your file_operations
    ret = cdev_add(&data->cdev, data->devt, 1);
    if(ret < 0){
        pr_err("DHT20 cdev_add failed!\n");
        goto err_chrdev;
    }

    // Create a class under /sys/class/dht20_class/
    // => used by udev to automatically create device nodes in /dev
    dht20_class = class_create("dht20_class");
    if (IS_ERR(dht20_class)){
        pr_err("Cannot create the struct class for dht20!\n");
        ret = PTR_ERR(dht20_class);
        goto err_cdev;
    }

    // Create device node /dev/DHT20
    // => user-space can access it (e.g., cat /dev/DHT20)
    // => linked with devt (major/minor) registered above
    // => when accessed, kernel routes to your file_operations
    data->device = device_create(dht20_class, NULL, data->devt, NULL, DEVICE_NAME);

    if (IS_ERR(data->device)){
        pr_err("Cannot create device file!\n");
        ret = PTR_ERR(data->device);
        goto err_class;
    }

    ret = dht20_device_init(client);
    if (ret < 0) {
        pr_err("DHT20 init failed\n");
        goto err_device;
    }
    return 0;

err_device:
    device_destroy(dht20_class, data->devt);

err_class:
    class_destroy(dht20_class);

err_cdev:
    cdev_del(&data->cdev);

err_chrdev:
    unregister_chrdev_region(data->devt, 1);

err_alloc:
    kfree(data);

    return ret;
}

// Remove function is called when the device is removed
// (driver unloaded or device disconnected)
static void dht20_remove(struct i2c_client *client){
    struct dht20_data *data = i2c_get_clientdata(client);
    device_destroy(dht20_class, data->devt);
    class_destroy(dht20_class);
    cdev_del(&data->cdev);
    unregister_chrdev_region(data->devt, 1);

    kfree(data);
    pr_info("DHT20 removed\n");
}
/*
  Module initialization function
  This function is called when the driver is inserted into the kernel.
 
  i2c_add_driver():
    - Registers this driver with the I2C subsystem
    - The kernel will scan all I2C devices
    - Match devices using of_match_table (Device Tree)
    - If a match is found, the probe() function is invoked
*/
static int __init dht20_init(void){
    pr_info("DHT20 module inserted successfully!\n");
    return i2c_add_driver(&dht20_driver);
}

/*
 Module exit function
 This function is called when the driver is removed from the kernel.
 
 i2c_del_driver():
    - Unregisters the driver from the I2C subsystem
    - Calls remove() for all matched devices
    - Frees associated resources
*/
static void __exit dht20_exit(void){
    i2c_del_driver(&dht20_driver);
    pr_info("DHT20 module removed successfully!\n");
}

// Open function is called when user-space opens the device file
// (e.g., open("/dev/DHT20") or cat /dev/DHT20)
static int dht20_open(struct inode *inode, struct file *file){
    struct dht20_data *data;

    // Retrieve the driver data structure from inode
    // inode->i_cdev points to the cdev we registered in probe()
    // container_of is used to get the parent structure (dht20_data)
    data = container_of(inode->i_cdev, struct dht20_data, cdev);
    // Store driver data in file->private_data
    // => this allows read/write functions to access device context
    file->private_data = data;
    pr_info("Device opened!\n");
    return 0;
}

// Release function is called when user-space closes the device file
// (e.g., close() or when cat finishes reading)
static int dht20_release(struct inode *inode, struct file *file){
    // No special cleanup needed here
    // (resources are freed in remove(), not here)
    pr_info("DHT20 released!\n");
    return 0;
}

static int dht20_device_init(struct i2c_client *client){
    int ret;
    uint8_t cmd = 0x71;   // read status
    uint8_t status;

    // wait after power-on
    msleep(100);

    // send command 0x71 to read status
    ret = i2c_master_send(client, &cmd, 1);
    if (ret != 1) return -EIO;

    // read 1 byte status
    ret = i2c_master_recv(client, &status, 1);
    if (ret != 1) return -EIO;

    // check calibration bit (bit[3])
    if ((status & 0x18) != 0x18) {
        pr_warn("DHT20 not calibrated (status=0x%x)\n", status);
        // use init sequence (0x1B, 0x1C, 0x1E)
    }

    return 0;
}
static int dht20_device_read(struct i2c_client *client, uint8_t *buf){
    int ret;
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    uint8_t status;
    // trigger measurement
    ret = i2c_master_send(client, cmd, 3);
    if (ret != 3) return -EIO;

    // wait measurement (~80ms)
    msleep(80);
    // check busy bit (bit7)
    do {
        msleep(10);
        ret = i2c_master_recv(client, &status, 1);
        if (ret != 1)
            return -EIO;
    } while (status & 0x80);
    // read 7 bytes (status + 6 data + crc)
    ret = i2c_master_recv(client, buf, 7);
    if (ret != 7) return -EIO;

    return 0;
}
static ssize_t dht20_read(struct file *file, char __user *buf, size_t count, loff_t *offset){
    struct dht20_data *data = file->private_data;
    uint8_t raw[7];
    int ret, temp, hum;

    char out[64];
    int len = 0;

    // read raw data
    ret = dht20_device_read(data->client, raw);
    if (ret < 0) return ret;

    // parse humidity (20-bit)
    uint32_t raw_hum  = ((raw[1] << 16) | (raw[2] << 8) | raw[3]) >> 4;

    // parse temperature (20-bit)
    uint32_t raw_temp = ((raw[3] & 0x0F) << 16) | (raw[4] << 8) | raw[5];

    // convert 
    hum  = (raw_hum * 100) / (1 << 20);
    temp = ((raw_temp * 200) / (1 << 20)) - 50;

    // format output
    len = snprintf(out, sizeof(out), "Temp=%dC Hum=%d%%\n", temp, hum);

    // copy to user
    if (copy_to_user(buf, out, len)) return -EFAULT;

    return len;
}
module_init(dht20_init);
module_exit(dht20_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("AN NGUYEN");
MODULE_DESCRIPTION("SIMPLE DRIVER FOR TEMPERATURE DHT20");
MODULE_VERSION("1.0");