#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "ap3216creg.h"

#define AP3216C_CNT     1
#define AP3216C_NAME    "ap3"

struct ap3216c_dev {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int major;
    struct i2c_client *client;
    unsigned short ir, als, ps;
    struct mutex lock;
};

static int ap3216c_read_regs(struct ap3216c_dev *dev, u8 reg,
                              void *val, int len)
{
    struct i2c_client *client = dev->client;
    struct i2c_msg msg[2];
    int ret;

    if (!client)
        return -ENODEV;

    msg[0].addr  = client->addr;
    msg[0].flags = 0;
    msg[0].buf   = &reg;
    msg[0].len   = 1;

    msg[1].addr  = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf   = val;
    msg[1].len   = len;

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret == 2)
        return 0;

    dev_err(&client->dev, "i2c rd failed=%d reg=0x%02x len=%d\n",
            ret, reg, len);
    return (ret < 0) ? ret : -EREMOTEIO;
}

static int ap3216c_write_regs(struct ap3216c_dev *dev, u8 reg,
                               u8 *buf, u8 len)
{
    u8 b[2];
    struct i2c_msg msg;
    struct i2c_client *client = dev->client;

    if (len > (sizeof(b) - 1))
        return -EINVAL;

    b[0] = reg;
    memcpy(&b[1], buf, len);

    msg.addr  = client->addr;
    msg.flags = 0;
    msg.buf   = b;
    msg.len   = len + 1;

    return i2c_transfer(client->adapter, &msg, 1);
}

static int ap3216c_read_reg(struct ap3216c_dev *dev, u8 reg, u8 *data)
{
    return ap3216c_read_regs(dev, reg, data, 1);
}

static int ap3216c_write_reg(struct ap3216c_dev *dev, u8 reg, u8 data)
{
    return ap3216c_write_regs(dev, reg, &data, 1);
}

static int ap3216c_readdata(struct ap3216c_dev *dev)
{
    unsigned char buf[6];
    int ret;

    /* 一次性读取6个寄存器 (IR/ALS/PS 各2字节) */
    ret = ap3216c_read_regs(dev, AP3216C_IRDATALOW, buf, 6);
    if (ret < 0) {
        dev_err(&dev->client->dev, "read sensor data failed: %d\n", ret);
        return ret;
    }

    /* 解析 IR 数据 */
    if (buf[0] & 0x80)
        dev->ir = 0;
    else
        dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0x03);

    /* 解析 ALS 数据 */
    dev->als = ((unsigned short)buf[3] << 8) | buf[2];

    /* 解析 PS 数据 */
    if (buf[4] & 0x40)
        dev->ps = 0;
    else
        dev->ps = ((unsigned short)(buf[5] & 0x3F) << 4) |
                  (buf[4] & 0x0F);

    return 0;
}

static int ap3216c_open(struct inode *inode, struct file *filp)
{
    struct ap3216c_dev *dev;

    dev = container_of(inode->i_cdev, struct ap3216c_dev, cdev);
    filp->private_data = dev;

    mutex_lock(&dev->lock);
    ap3216c_write_reg(dev, AP3216C_SYSTEMCONFIG, 0x04);
    msleep(50);   /* 等待传感器复位, 使用可睡眠的 msleep 替代忙等的 mdelay */
    ap3216c_write_reg(dev, AP3216C_SYSTEMCONFIG, 0x03);
    mutex_unlock(&dev->lock);

    return 0;
}

static ssize_t ap3216c_read(struct file *filp, char __user *buf,
                            size_t cnt, loff_t *off)
{
    short data[3];
    long err;
    int ret;
    struct ap3216c_dev *dev = (struct ap3216c_dev *)filp->private_data;

    if (cnt < sizeof(data))
        return -EINVAL;

    mutex_lock(&dev->lock);
    ret = ap3216c_readdata(dev);
    if (ret < 0) {
        mutex_unlock(&dev->lock);
        return ret;
    }

    data[0] = dev->ir;
    data[1] = dev->als;
    data[2] = dev->ps;
    mutex_unlock(&dev->lock);

    err = copy_to_user(buf, data, sizeof(data));
    if (err)
        return -EFAULT;

    *off += sizeof(data);
    return sizeof(data);
}

static int ap3216c_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations ap3216c_ops = {
    .owner   = THIS_MODULE,
    .open    = ap3216c_open,
    .read    = ap3216c_read,
    .release = ap3216c_release,
};

static int ap3216c_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    struct ap3216c_dev *dev;
    int ret;

    /* 动态分配设备结构体, 支持多实例 */
    dev = devm_kzalloc(&client->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    mutex_init(&dev->lock);
    dev->client = client;

    ret = alloc_chrdev_region(&dev->devid, 0, AP3216C_CNT, AP3216C_NAME);
    if (ret) {
        dev_err(&client->dev, "alloc_chrdev_region failed: %d\n", ret);
        return ret;
    }
    dev->major = MAJOR(dev->devid);

    cdev_init(&dev->cdev, &ap3216c_ops);
    ret = cdev_add(&dev->cdev, dev->devid, AP3216C_CNT);
    if (ret) {
        dev_err(&client->dev, "cdev_add failed: %d\n", ret);
        goto err_cdev;
    }

    dev->class = class_create(THIS_MODULE, AP3216C_NAME);
    if (IS_ERR(dev->class)) {
        ret = PTR_ERR(dev->class);
        dev_err(&client->dev, "class_create failed: %d\n", ret);
        goto err_class;
    }

    dev->device = device_create(dev->class, NULL, dev->devid, NULL,
                                AP3216C_NAME);
    if (IS_ERR(dev->device)) {
        ret = PTR_ERR(dev->device);
        dev_err(&client->dev, "device_create failed: %d\n", ret);
        goto err_device;
    }

    i2c_set_clientdata(client, dev);
    return 0;

err_device:
    class_destroy(dev->class);
err_class:
    cdev_del(&dev->cdev);
err_cdev:
    unregister_chrdev_region(dev->devid, AP3216C_CNT);
    return ret;
}

static int ap3216c_remove(struct i2c_client *client)
{
    struct ap3216c_dev *dev = i2c_get_clientdata(client);

    cdev_del(&dev->cdev);
    unregister_chrdev_region(dev->devid, AP3216C_CNT);
    device_destroy(dev->class, dev->devid);
    class_destroy(dev->class);

    return 0;
}

static const struct i2c_device_id ap3216c_id[] = {
    {"ap3216c", 0},
    {}
};

static const struct of_device_id ap3216c_of_match[] = {
    {.compatible = "ap3216c"},
    {},
};

static struct i2c_driver ap3216c_driver = {
    .probe   = ap3216c_probe,
    .remove  = ap3216c_remove,
    .driver  = {
        .owner          = THIS_MODULE,
        .name           = "ap3216cf",
        .of_match_table = ap3216c_of_match,
    },
    .id_table = ap3216c_id,
};

static int __init xxx_init(void)
{
    int ret;

    pr_info("ap3216c: module_init\n");
    ret = i2c_add_driver(&ap3216c_driver);
    return ret;
}

static void __exit xxx_exit(void)
{
    pr_info("ap3216c: module_exit\n");
    i2c_del_driver(&ap3216c_driver);
}

module_init(xxx_init);
module_exit(xxx_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("lzy");
MODULE_DESCRIPTION("AP3216C ALS/PS/IR sensor I2C driver");
