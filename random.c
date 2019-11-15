#include <linux/device.h>  
#include <linux/fs.h>   
#include <linux/init.h>  
#include <linux/kernel.h>  
#include <linux/module.h>
#include <linux/mutex.h>  
#include <linux/random.h>  
#include <linux/uaccess.h> 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phan Son Loc");
MODULE_DESCRIPTION("Character device giving user a randomized number");

static DEFINE_MUTEX(rand_char_mutex); // only 1 program can access

#define DEVICE_NAME "rand_char"
#define CLASS_NAME "rand_char_class"

static int major_number; // device number
static struct class *rand_char_class = NULL;
static struct device *rand_char_device = NULL;

static int rand_open(struct inode *, struct file *);
static int rand_release(struct inode *, struct file *);
static ssize_t rand_read(struct file *, char *, size_t, loff_t *);

static struct file_operations fops = {
    .open = rand_open,
    .release = rand_release,
    .read = rand_read,
};

static int __init rand_init(void)
{
    printk(KERN_INFO "Random module initialized\n");

    // try to allocate major number
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) 
    {
        printk(KERN_ALERT
               "Random device failed to register a major number\n");
        return major_number;
    }
    printk(KERN_INFO "Random device registered a major number %d\n",
           major_number);

    // register class
    rand_char_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(rand_char_class)) 
    {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Rand device failed to create a struct "
                          "pointer class\n");
        return PTR_ERR(rand_char_class); // return error for pointer
    }
    printk(KERN_INFO "rand char device created a struct pointer class\n");

    // register device
    rand_char_device = device_create(rand_char_class, NULL,
                                     MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(rand_char_device)) 
    {
        class_destroy(rand_char_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "rand char device failed to create a struct "
                          "pointer device\n");
        return PTR_ERR(rand_char_device); // return error for pointer
    }
    printk(KERN_INFO "rand char device created a struct pointer device\n");

    // set up mutex
    mutex_init(&rand_char_mutex);

    return 0;
}

static void __exit rand_exit(void)
{
    // cleanup mutex
    mutex_destroy(&rand_char_mutex);

    // cleanup device, class, unregister major number
    device_destroy(rand_char_class, MKDEV(major_number, 0));
    class_unregister(rand_char_class);
    class_destroy(rand_char_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    printk(KERN_INFO "rand char module exit\n");
}

// call each time device is opened
static int rand_open(struct inode *inodep, struct file *filep)
{
    // check if another process is opening device
    if (!mutex_trylock(&rand_char_mutex)) {
        printk(KERN_ALERT "rand char device is opened by another process\n");
        return -EBUSY;
    }

    printk(KERN_INFO "rand char device is opened\n");
    return 0;
}

// call each time device is released
static int rand_release(struct inode *inodep, struct file *filep)
{
    // unlock mutex
    mutex_unlock(&rand_char_mutex);

    printk(KERN_INFO "rand char device is released\n");
    return 0;
}

// call each time device is read
// device send random number to user
static ssize_t rand_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    // random number in kernel space
    int random_num;
    get_random_bytes(&random_num, sizeof(random_num));

    int status = 0;
    // copy_to_user return 0 - success
    // otherwise - fail
    status = copy_to_user(buffer, &random_num, sizeof(random_num));
    if (status == 0) {
        printk(KERN_INFO "rand char sent random number %d\n", random_num);
        return 0;
    } else {
        printk(KERN_INFO "rand char failed to send random number %d\n",
               random_num);
        return -EFAULT;
    }
}

module_init(rand_init);
module_exit(rand_exit);
