#include <linux/fs.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "bf.h"

MODULE_LICENSE("GPL");

#define DRIVER_NAME "bf"
static const unsigned int MINOR_BASE = 0;
static const unsigned int MINOR_NUM = 1;
static unsigned int bf_machine_major;
static struct cdev bf_machine_cdev;
static struct class *bf_machine_class = NULL;

static bf_machine_t m;

// '>'
static void gt(bf_machine_t *m)
{
    printk("gt\n");
    (m->data_head)++;
}

// '<'
static void lt(bf_machine_t *m)
{
    printk("lt\n");
    (m->data_head)--;
}

// '+'
static void plus(bf_machine_t *m)
{
    printk("plus\n");
    (*m->data_head)++;
}

// '-'
static void minus(bf_machine_t *m)
{
    printk("minus\n");
    (*m->data_head)--;
}

// '.'
static void dot(bf_machine_t *m)
{
    printk("dot\n");
    sprintf(m->obuf_head, "%c", *m->data_head);
    m->obuf_head++;
}

// ','
static void comma(bf_machine_t *m)
{
    printk("comma not implemented\n");
    //*m->data_head = getchar();
}

// '['
static void left_bracket(bf_machine_t *m)
{
    printk("left_bracket\n");
    if (!*m->data_head)
    {
        int depth = 1;
        while (depth)
        {
            (m->code_head)++;
            if (*m->code_head == ']')
                depth--;
            else if (*m->code_head == '[')
                depth++;
        }
    }
}

// ']'
static void right_bracket(bf_machine_t *m)
{
    printk("right_bracket\n");
    if (*m->data_head)
    {
        int depth = 1;
        while (depth)
        {
            (m->code_head)--;
            if (*m->code_head == '[')
                depth--;
            else if (*m->code_head == ']')
                depth++;
        }
    }
}

static int bf_machine_open(struct inode *inode, struct file *file)
{
	printk("bf_machine opened\n");
	return 0;
}

static int bf_machine_close(struct inode *inode, struct file *file)
{
    printk("bf_machine closed\n");
    return 0;
}

static ssize_t bf_machine_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t size;
    printk("bf_machine_read\n");
    while (*(m.code_head))
    {
        switch (*(m.code_head))
        {
        case '>':
            gt(&m);
            break;
        case '<':
            lt(&m);
            break;
        case '+':
            plus(&m);
            break;
        case '-':
            minus(&m);
            break;
        case '.':
            dot(&m);
            break;
        case ',':
            comma(&m);
            break;
        case '[':
            left_bracket(&m);
            break;
        case ']':
            right_bracket(&m);
            break;
        }
        m.code_head++;
    }
    if (copy_to_user(buf, m.obuf_offset, count))
        return -EFAULT;
    printk("%s\n", m.obuf_offset);
    if (m.obuf_offset + count >= m.obuf_head)
    {
        size = m.obuf_head - m.obuf_offset;
        m.obuf_offset = m.obuf_head;
        return size;
    }
    m.obuf_offset += count;
    return count;
}

static ssize_t bf_machine_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    printk("bf_machine_write\n");
    m.data_arr = kmalloc(m.machine_size, GFP_KERNEL);
    m.data_head = m.data_arr;
    m.code_arr = kmalloc(m.code_size, GFP_KERNEL);
    m.code_head = m.code_arr;
    m.obuf = kmalloc(m.obuf_size, GFP_KERNEL);
    m.obuf_head = m.obuf;
    m.obuf_offset = m.obuf_head;
    memset(m.data_arr, 0, m.machine_size);
    memset(m.code_arr, 0, m.code_size);
    memset(m.obuf, 0, m.obuf_size);
    if (copy_from_user(m.code_arr, buf, count))
        return -EFAULT;
    return count;
}

// not thread safe
static long bf_machine_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
    case BF_INIT_MACHINE:
        printk("BF_INIT_MACHINE\n");
        if (!arg)
            return -EACCES;
        kfree(m.data_arr);
        kfree(m.code_arr);
        kfree(m.obuf);
        if (copy_from_user(&m , (bf_machine_t __user *)arg, sizeof(bf_machine_t)))
        {
            printk(KERN_ERR "copy from user\n");
            return -EFAULT;
        }
        m.data_arr = NULL;
        m.code_arr = NULL;
        m.obuf = NULL;
        return 0;
    default:
        return -EINVAL;
    }
}

static struct file_operations bf_machine_fops = {
    .open = bf_machine_open,
    .release = bf_machine_close,
    .read = bf_machine_read,
    .write = bf_machine_write,
    .unlocked_ioctl = bf_machine_ioctl,
    .compat_ioctl = bf_machine_ioctl,
};

static int bf_machine_init(void)
{
    dev_t dev;
    int status = 0;
    int alloc, err;
    int minor;
    printk("bf_machine init\n");
    if ((alloc = alloc_chrdev_region(&dev, MINOR_BASE, MINOR_NUM, DRIVER_NAME))) {
        printk(KERN_ERR "%s:%d : alloc_chrdev_region = %d\n", __FILE__, __LINE__ - 1, alloc);
        return -1;
    }
    bf_machine_major = MAJOR(dev);
    dev = MKDEV(bf_machine_major, MINOR_BASE);
    cdev_init(&bf_machine_cdev, &bf_machine_fops);
    bf_machine_cdev.owner = THIS_MODULE;
    if ((err = cdev_add(&bf_machine_cdev, dev, MINOR_NUM))) {
        printk(KERN_ERR "%s:%d : cdev_add = %d\n", __FILE__, __LINE__ - 1, err);
        status = -1;
        goto unregister;
    }
    bf_machine_class = class_create(THIS_MODULE, "bf_machine");
    if (IS_ERR(bf_machine_class))
    {
        printk(KERN_ERR "%s:%d : class create\n", __FILE__, __LINE__ - 3);
        status = -1;
        goto del;
    }
    for (minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++)
        device_create(bf_machine_class, NULL, MKDEV(bf_machine_major, minor), NULL, "bf_machine%d", minor);
    goto ret;
del:
    cdev_del(&bf_machine_cdev);
unregister:
    unregister_chrdev_region(dev, MINOR_NUM);
ret:
    return status;
}

static void bf_machine_exit(void)
{
    int minor;
    dev_t dev;
    printk("bf_machine exit\n");
    dev = MKDEV(bf_machine_major, MINOR_BASE);
    for (minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM;minor++)
        device_destroy(bf_machine_class, MKDEV(bf_machine_major, minor));
    class_destroy(bf_machine_class);
    cdev_del(&bf_machine_cdev);
    unregister_chrdev_region(dev, MINOR_NUM);
    kfree(m.code_arr);
    kfree(m.data_arr);
    kfree(m.obuf);
}

module_init(bf_machine_init);
module_exit(bf_machine_exit);
