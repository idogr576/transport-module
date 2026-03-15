/*
 * chardev.c: Creates a read-only char device that says how many times
 * you have read from the dev file
 */

#include <linux/atomic.h>
#include <linux/cdev.h>

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>

#include <linux/init.h>
#include <linux/kernel.h> /* for sprintf() */

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/sprintf.h>
#include <linux/types.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/socket.h>
#include <linux/uio.h>

#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/version.h>
#include <linux/inet.h>
#include <linux/byteorder/generic.h>

#include <asm/errno.h>

/*  Prototypes - this would normally go in a .h file */

static int device_open(struct inode *, struct file *);

static int device_release(struct inode *, struct file *);

static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);

static ssize_t device_write(struct file *, const char __user *, size_t,
                            loff_t *);

#define DEVICE_NAME "udp" /* Dev name as it appears in /proc/devices   */

#define BUF_LEN 256 /* Max length of the message from the device */

/* Global variables are declared as static, so are global within the file. */

static int major; /* major number assigned to our device driver */

// static char msg[BUF_LEN + 1]; /* The msg the device will give when asked */
static struct class *cls;

static struct socket *sock;

static size_t counter = 0;

static struct file_operations chardev_fops = {

    .read = device_read,
    .write = device_write,
    .open = device_open,

    .release = device_release,
};

static int __init chardev_init(void)
{

    major = register_chrdev(0, DEVICE_NAME, &chardev_fops);

    if (major < 0)
    {
        pr_alert("Registering char device failed with %d\n", major);
        return major;
    }

    pr_info("I was assigned major number %d.\n", major);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    cls = class_create(DEVICE_NAME);

#else
    cls = class_create(THIS_MODULE, DEVICE_NAME);
#endif

    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

    pr_info("Device created on /dev/%s\n", DEVICE_NAME);

    pr_info("Initilizing UDP socket\n");
    int err;
    err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
    if (err < 0)
    {
        /* handle error */
        pr_alert("cannot create a udp kernel socket. err = %d\n", err);
    }

    return 0;
}

static void __exit chardev_exit(void)
{
    device_destroy(cls, MKDEV(major, 0));

    class_destroy(cls);

    /* Unregister the device */
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("releasing the socket\n");

    sock_release(sock);
}

/* Methods */

/* Called when a process tries to open the device file, like

 * "sudo cat /dev/chardev"
 */
static int device_open(struct inode *inode, struct file *file)
{
    pr_info("open the device\n");
    return 0;
}

/* Called when a process closes the device file. */

static int device_release(struct inode *inode, struct file *file)
{
    pr_info("close the device\n");
    return 0;
}

/* Called when a process, which already opened the dev file, attempts to

 * read from it.
 */
static ssize_t device_read(struct file *filp,   /* see include/linux/fs.h   */
                           char __user *buffer, /* buffer to fill with data */
                           size_t length,       /* length of the buffer     */
                           loff_t *offset)
{
    pr_alert("Sorry, this operation is not supported.\n");
    return -EPERM;
}

/* Called when a process writes to dev file: echo "hi" | sudo tee /dev/chardev */

static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{
    struct msghdr msg;
    struct sockaddr_in address;
    char user_buff_in_kernel[BUF_LEN] = {0};

    strncpy_from_user(user_buff_in_kernel, buff, len < BUF_LEN ? len : BUF_LEN);
    printk("sending data on the udp socket: %s\n", user_buff_in_kernel);

    char buff_address[BUF_LEN] = {0};
    int buff_port = 0;
    char buff_message[BUF_LEN] = {0};
    int num = sscanf(user_buff_in_kernel, "%s %d %255[^\\n]%*c", buff_address, &buff_port, buff_message);
    printk("buff_address = %s, buff_port = %d, buff_message = %s\n", buff_address, buff_port, buff_message);
    if (num != 3)
    {
        printk("WARNING: got invalid data, required \"<ipv4> <port> <data>\"\n");
        return -EINVAL;
    }
    in4_pton(buff_address, -1, (u8 *)&address.sin_addr.s_addr, -1, NULL);
    address.sin_family = AF_INET;
    address.sin_port = htons(buff_port);

    struct kvec vec;
    vec.iov_base = buff_message;
    vec.iov_len = BUF_LEN;

    printk("reached msg assingment\n");
    msg.msg_name = (struct sockaddr *)&address;
    msg.msg_namelen = sizeof(address);

    printk("reached sock_sendmsg: %d\n", ++counter);
    int sent = kernel_sendmsg(sock, &msg, &vec, BUF_LEN, strlen(buff_message));
    if (sent != strlen(buff_message))
    {
        printk("WARNING: message was sent but may be incomplete!");
    }
    return len;
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");