#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/module.h>
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
} ap3216cdev;

static int ap3216c_read_regs(struct ap3216c_dev *dev, u8 reg,
                              void *val, int len)
{
    struct i2c_client *client = dev->client;  // ← 就这一句，别碰 private_data
    struct i2c_msg msg[2];
    int ret;

    if (!client) return -ENODEV;

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

static u32 ap3216c_write_regs(struct ap3216c_dev *dev, u8 reg,
                                u8 *buf, u8 len)
{
    u8 b[256];
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client *)
                                    dev->client;
    
    b[0] = reg;
    memcpy(&b[1],buf,len);

    msg.addr = client->addr;
    msg.flags = 0;

    msg.buf = b;
    msg.len = len + 1;

    return i2c_transfer(client->adapter, &msg, 1);
}

static unsigned char ap3216c_read_reg(struct ap3216c_dev *dev,
                                        u8 reg)
{
    u8 data = 0;

    ap3216c_read_regs(dev, reg, &data, 1);

    return data;
}

static void ap3216c_write_reg(struct ap3216c_dev *dev, u8 reg,
                                u8 data)
{
    u8 buf = 0;
    buf = data;
    ap3216c_write_regs(dev, reg, &buf, 1);
}
void ap3216c_readdata(struct ap3216c_dev *dev)
{
    unsigned char i = 0;
    unsigned char buf[6];

    for(i = 0; i < 6; i++)
        buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i);
    if(buf[0] & 0x80)
        dev->ir = 0;
    else
        dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0x03);
    
    dev->als = ((unsigned short)buf[3] << 8) | buf[2];

    if(buf[4] & 0x40)
        dev->ps = 0;
    else
        dev->ps = ((unsigned short)(buf[5] & 0x3F) << 4) |
                                    (buf[4] & 0x0F);
}

static int ap3216c_open(struct inode *inode, struct file* filp)
{
    filp->private_data = &ap3216cdev;

    ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x04);
    mdelay(50);
    ap3216c_write_reg(&ap3216cdev, AP3216C_SYSTEMCONG, 0x03);
    return 0;
}

static ssize_t ap3216c_read(struct file *filp, char __user *buf,
                            size_t cnt, loff_t *off)
{
    short data[3];
    long err = 0;
    
    struct ap3216c_dev *dev = (struct ap3216c_dev *)filp->private_data;

    ap3216c_readdata(dev);

    data[0] = dev->ir;
    data[1] = dev->als;
    data[2] = dev->ps;
    err = copy_to_user(buf, data, sizeof(data));
    return 0;
}

static int ap3216c_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations ap3216c_ops = {
    .owner =    THIS_MODULE,
    .open =     ap3216c_open,
    .read =     ap3216c_read,
    .release =  ap3216c_release,
};

static int ap3216c_probe(struct i2c_client *client,
                     const struct i2c_device_id *id)
{
    alloc_chrdev_region(&ap3216cdev.devid, 0, AP3216C_CNT,
                        AP3216C_NAME);
    ap3216cdev.major = MAJOR(ap3216cdev.devid);
    cdev_init(&ap3216cdev.cdev, &ap3216c_ops);
    cdev_add(&ap3216cdev.cdev, ap3216cdev.devid, AP3216C_CNT);
    
    ap3216cdev.class =class_create(THIS_MODULE, AP3216C_NAME);
    if(IS_ERR(ap3216cdev.class))
        return PTR_ERR(ap3216cdev.class);
    
    ap3216cdev.device = device_create(ap3216cdev.class, NULL,
                                        ap3216cdev.devid, NULL, AP3216C_NAME);
    if(IS_ERR(ap3216cdev.device))
        return PTR_ERR(ap3216cdev.device);

    ap3216cdev.client = client;
    return 0;
}

static int ap3216c_remove(struct i2c_client *client)
{
    cdev_del(&ap3216cdev.cdev);
    unregister_chrdev_region(ap3216cdev.devid, AP3216C_CNT);

    device_destroy(ap3216cdev.class, ap3216cdev.devid);
    class_destroy(ap3216cdev.class);
    return 0;
}

static const struct i2c_device_id ap3216c_id[] = {
    {"ap3216c", 0},
    {}
};

static const struct of_device_id ap3216c_of_match[] = {
    {.compatible = "hy,ap3216c"},
    {},
};

static struct i2c_driver ap3216c_driver = {
    .probe = ap3216c_probe,
    .remove = ap3216c_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ap3216c",
        .of_match_table = ap3216c_of_match,
    },
    .id_table = ap3216c_id,
};

static int __init xxx_init(void)
{
    int ret = 0;
    printk("module_init\n");
    ret = i2c_add_driver(&ap3216c_driver);
    return ret;
}

static void __exit xxx_exit(void)
{
    printk("module_exit\n");
    i2c_del_driver(&ap3216c_driver);
}

module_init(xxx_init);
module_exit(xxx_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("lzy");