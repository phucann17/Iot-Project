#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/i2c.h>
#include <linux/delay.h>


#define MEM_SIZE 1024
#define DHT20_ADDR 0x38
#define DEVICE_NAME "DHT20"
#define DRIVER_NAME "dht20_driver"
struct dht20_data {
    struct i2c_client *client;   // I2C handle

    dev_t devt;                  // device number
    struct cdev cdev;            // char device
    struct class *class;         // sysfs class
    struct device *device;       // device node

    uint8_t *buffer;             // kernel buffer
};
static struct class *dht20_class;   // class
//file operating
static int dht20_open(struct inode* inode, struct file* file);
static int dht20_release(struct inode* inode, struct file* file);
static ssize_t dht20_read(struct file* file, char __user *buff, size_t count, loff_t *offset);
static ssize_t dht20_write(struct file* file, const char __user *buff, size_t count, loff_t *offset);
static long dht20_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
//i2c driver
static int dht20_probe(struct i2c_client *client);
static void dht20_remove(struct i2c_client *client);
//logic function
static int dht20_device_init(struct i2c_client *client);
static int dht20_trigger_measurement(struct i2c_client *client);
static int dht20_read_data(struct i2c_client *client, uint8_t *buf);
static void dht20_parse(uint8_t *data, int *temp, int *hum);
static const struct file_operations dht_fops = {
	.owner = THIS_MODULE,
	.open = dht20_open,
	.release = dht20_release,
	.unlocked_ioctl = dht20_ioctl,
	.read = dht20_read,
	.write = dht20_write
};
static const struct of_device_id dht20_of_match[] = {
    { .compatible = "dht20" },
    { }
};
MODULE_DEVICE_TABLE(of, dht20_of_match);


static int dht20_probe(struct i2c_client* client){
	pr_info("DHT20 probe called\n");
	//allocate memory for struct data
	struct dht20_data *data = kmalloc(sizeof(*data), GFP_KERNEL);

	if (!data) {
    	pr_err("Cannot allocate memory for dht20_data\n");
    	return -1;
	}	

	data->client = client;
	i2c_set_clientdata(client, data);
	//allocate major and minor number
	if (alloc_chrdev_region(&data->devt, 0, 1, DRIVER_NAME) < 0){
		printk(KERN_INFO "Cannot allocate major number for dht20!\n");
		kfree(data);
		return -1;
	}
	printk(KERN_INFO "MAJOR = %d MINOR = %d\n", MAJOR(data->devt), MINOR(data->devt));
	//create cdev
	cdev_init(&data->cdev, &dht_fops);
	//add character device to the system
    if((cdev_add(&data->cdev, data->devt, 1)) < 0){
        pr_err("Cannot add the device to the system\n");
		unregister_chrdev_region(data->devt, 1);
        kfree(data);
		return -1;
    }
	//create struct class
	dht20_class = class_create("dht_class");
	if(IS_ERR(dht20_class)){
    	pr_err("Cannot create the struct class for dht20\n");
		cdev_del(&data->cdev);
        unregister_chrdev_region(data->devt, 1);
        kfree(data);
        class_destroy(dht20_class);
		return -1;
    }
	//create device
	if(IS_ERR(device_create(dht20_class, NULL, data->devt, NULL, DEVICE_NAME))){
        pr_err("Cannot create the Device\n");
		class_destroy(dht20_class);
        cdev_del(&data->cdev);
        unregister_chrdev_region(data->devt, 1);
        kfree(data);
		return -1;
    }
	pr_info("DHT20 probe success\n");
	return 0;
}

static void dht20_remove(struct i2c_client *client)
{
    struct dht20_data *data = i2c_get_clientdata(client);

    device_destroy(dht20_class, data->devt);
    class_destroy(dht20_class);
    cdev_del(&data->cdev);
    unregister_chrdev_region(data->devt, 1);
    kfree(data);

    pr_info("DHT20 removed\n");
}
static struct i2c_driver dht20_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = dht20_of_match,
    },
    .probe = dht20_probe,
    .remove = dht20_remove,
};
static int __init dht20_init(void){
	pr_info("DHT20 module inserted successfully.\n");
	return i2c_add_driver(&dht20_driver);
}

static void __exit dht20_exit(void){
	i2c_del_driver(&dht20_driver);   
	pr_info("DHT20 module removed successfully.\n");
}

static int dht20_open(struct inode* inode, struct file* file) {
    struct dht20_data *data;

    // take struct device
    data = container_of(inode->i_cdev, struct dht20_data, cdev);

    // assign file descriptor
    file->private_data = data;

    pr_info("DHT20 device opened!\n");
    return 0;
}

static int dht20_release(struct inode* inode, struct file* file){
	pr_info("DHT20 released!\n");
	return 0;
}
static long dht20_ioctl(struct file *file, unsigned int cmd, unsigned long arg){

    return 0;
}

static int dht20_trigger_measurement(struct i2c_client *client){
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    int ret;

    ret = i2c_master_send(client, cmd, 3);
    if(ret != 3){
        pr_err("DHT20 trigger measurement failed\n");
        return -EIO;
    }

    return 0;
}

static int dht20_read_data(struct i2c_client *client, uint8_t *buf){
    int ret;

    ret = i2c_master_recv(client, buf, 7);
    if(ret != 7){
        pr_err("DHT20 read data failed\n");
        return -EIO;
    }

    return 0;
}

static void dht20_parse(uint8_t *data, int *temp, int *hum){
    uint32_t raw_hum, raw_temp;

    raw_hum  = ((data[1] << 16) | (data[2] << 8) | data[3]) >> 4;
    raw_temp = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];

    *hum  = (raw_hum  * 100) / (1 << 20);
    *temp = ((raw_temp * 200) / (1 << 20)) - 50;
}
static ssize_t dht20_read(struct file* file, char __user *buff, size_t count, loff_t *offset){
    struct dht20_data *data = file->private_data;
    uint8_t raw[7];
    int temp, hum;
    char out[50];

    // trigger
    if(dht20_trigger_measurement(data->client) < 0)
        return -EIO;

    msleep(80); // wait

    if(dht20_read_data(data->client, raw) < 0)
        return -EIO;

    dht20_parse(raw, &temp, &hum);

    int len = sprintf(out, "Temp=%dC Hum=%d%%\n", temp, hum);

    if(copy_to_user(buff, out, len))
        return -EFAULT;

    return len;
}
static ssize_t dht20_write(struct file* file, const char __user *buff, size_t count, loff_t *offset){
	pr_info("DHT20 wrote!\n");
	/*
	if (copy_from_user(kern_buff, buff, count)){
		pr_err("Data Write: Err!\n");
	}
	pr_info("Data Write: Successfully!\n");*/
	return 0;
}
module_init(dht20_init);
module_exit(dht20_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("AN NGUYEN");
MODULE_DESCRIPTION("simple linux device driver for temperature and humidity dht20");
MODULE_VERSION("1.0");
