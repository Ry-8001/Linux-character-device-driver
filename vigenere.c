#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <asm/switch_to.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */


#include <linux/version.h>  /* in order to check kernel version */


//  check if linux/uaccess.h is required for copy_*_user
//instead of asm/uaccess
//required after linux kernel 4.1+ ?
#ifndef __ASM_ASM_UACCESS_H
    #include <linux/uaccess.h>
#endif


#include "vigenere.h"

#define VIGENERE_MAJOR 0
#define VIGENERE_CAPACITY 4000
#define MOD_DEC_USE_COUNT
#define NUM(minor)    ((minor) & 0xf)

//device Parameters
int vigenere_major = VIGENERE_MAJOR;
int vigenere_minor = 0;
int vigenere_capacity = VIGENERE_CAPACITY;
char *vigenere_key = "A";
int mode=0;
spinlock_t vigenere_s_lock;
//device Parameters
DEFINE_SPINLOCK(vigenere_s_lock);
int vigenere_s_count=0;
module_param(vigenere_major, int, S_IRUGO);
module_param(vigenere_minor, int, S_IRUGO);
module_param(vigenere_capacity, int, S_IRUGO);
module_param(vigenere_key, charp, S_IRUGO);


MODULE_AUTHOR("Ramazan Yetismis, Faruk Orak");
MODULE_LICENSE("Dual BSD/GPL");

int util_lenght(char* string);
int util_cmp(char* s1,char* s2);

struct vigenere_dev {
    char *data;
    char *key;
    int mode;
    int key_length;
    unsigned int size;
    struct semaphore sem;
    struct cdev cdev;
};

struct vigenere_dev *vigenere_device;
int vigenere_trim(struct vigenere_dev *dev)
{
    if (dev->data) {
        kfree(dev->data);
    }
    dev->data = NULL;
    dev->key = "A";
    dev->key_length = 1;
    dev->size = 0;
    return 0;
}


int vigenere_open(struct inode *inode, struct file *filp)
{
    struct vigenere_dev *dev=vigenere_device;
  
    int num = NUM(inode->i_rdev);

    if (!filp->private_data && num > 0)
        return -ENODEV; /* not devfs: allow 1 device only */
    spin_lock(&vigenere_s_lock);
    if (vigenere_s_count) {
        spin_unlock(&vigenere_s_lock);
        return -EBUSY; /* already open */
    }
    vigenere_s_count++;
    spin_unlock(&vigenere_s_lock);
    

    dev = container_of(inode->i_cdev, struct vigenere_dev, cdev);
    filp->private_data = dev;


    /* trim the device if open was write-only */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
        vigenere_trim(dev);
        up(&dev->sem);
    }
    return 0;
}


int vigenere_release(struct inode *inode, struct file *filp)
{
    vigenere_s_count--;
    MOD_DEC_USE_COUNT;
    return 0;
}


ssize_t vigenere_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct vigenere_dev *dev = filp->private_data;
    ssize_t retval = 0;
    char* str = kmalloc(count*sizeof(char), GFP_KERNEL);
    int i;
    memset(str, 0, count*sizeof(char));


    if (down_interruptible(&dev->sem))
    {
      
        return -ERESTARTSYS;
    }
 
    if (*f_pos >= vigenere_capacity)
    {
       
        goto out;
    }

    if (*f_pos >= dev->size)
    {
        
        goto out;
    }
    if (*f_pos + count > vigenere_capacity)
        count = vigenere_capacity - *f_pos;

   

    if (dev->data == NULL)
    {
    
        goto out;
    }

   
    if (count > vigenere_capacity - (long) *f_pos)
        count = vigenere_capacity - (long) *f_pos;

   
    for(i = 0; i < count; i++ )
    {
        str[i] = (((dev->data[i+(long) *f_pos]-65 - (dev->key[i % dev->key_length]-65) + 26)) % 26) + 65;
        if(!(str[i] >= 65 && str[i] <= 90))
        {
            str[i] = 0;
            break;
        }
    }
    if (dev->mode==0)
    {
        if (copy_to_user(buf, dev->data+(long) *f_pos, count)) {
            retval = -EFAULT;
            goto out;
        }
    }
    else{

        if (copy_to_user(buf, str, count)) {
        retval = -EFAULT;
        goto out;
    }
    }
   
    
    *f_pos += count;
    
    retval = count;

  out:
    kfree(str);
    up(&dev->sem);
    return retval;
}


ssize_t vigenere_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos)
{
    struct vigenere_dev *dev = filp->private_data;
 
    ssize_t retval = -ENOMEM;
    int i;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    if (*f_pos >= vigenere_capacity) {
       
        retval = 0;
        goto out;
    }

   


    if (!dev->data) {
        dev->data = kmalloc(vigenere_capacity, GFP_KERNEL);
        if (!dev->data)
            goto out;
    }
   
    if (count > vigenere_capacity - (long) *f_pos)
    {
        
        count = vigenere_capacity - (long) *f_pos;    
    }
    

    if (copy_from_user(&dev->data[(long) *f_pos], buf, count)) {
        retval = -EFAULT;
        goto out;
    }
    
    for(i = 0; i < count; i++)
    {

        dev->data[(long) *f_pos + i] = ((dev->data[(long) *f_pos + i] - 65 + dev->key[i % dev->key_length] - 65) % 26) + 65;
    } 
    *f_pos += count;
    retval = count;
    /* update the size */
    if (dev->size < *f_pos)
        dev->size = *f_pos;

  out: 
    up(&dev->sem);
    return retval;
}

long vigenere_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct vigenere_dev *dev = filp->private_data;
	int err = 0;
	int retval = 0;

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != VIGENERE_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > VIGENERE_IOC_MAXNR) return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
    
	if (_IOC_DIR(cmd) & _IOC_READ)
    
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
            err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
        #else
            err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
        #endif
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
            err =  !access_ok((void __user *)arg, _IOC_SIZE(cmd));
        #else
            err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
        #endif
	if (err) return -EFAULT;

	switch(cmd) {
	  case VIGENERE_MODE_SIMPLE:
		
       
        if(util_cmp(dev->key, (char*)arg))
        {
            dev->mode=0;
            mode=0;
           
        }


		break;

	    case VIGENERE_MODE_DECRYPT: /* Set: arg points to the value */
		

        if(util_cmp(dev->key, (char*)arg))
        {
            dev->mode=1;
            mode=1;
        
        }
		break;

	 
	  default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}
	return retval;
}


loff_t vigenere_llseek(struct file *filp, loff_t off, int whence)
{
    struct vigenere_dev *dev = filp->private_data;
    loff_t newpos;

    switch(whence) {
        case 0: /* SEEK_SET */
            newpos = off;
            break;

        case 1: /* SEEK_CUR */
            newpos = filp->f_pos + off;
            break;

        case 2: /* SEEK_END */
            newpos = dev->size + off;
            break;

        default: /* can't happen */
            return -EINVAL;
    }
    if (newpos < 0)
        return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}


struct file_operations vigenere_fops = {
    .owner =    THIS_MODULE,
    .llseek =   vigenere_llseek,
    .read =     vigenere_read,
    .write =    vigenere_write,
    .unlocked_ioctl =  vigenere_ioctl,
    .open =     vigenere_open,
    .release =  vigenere_release,
};


void vigenere_cleanup_module(void)
{
    dev_t devno = MKDEV(vigenere_major, vigenere_minor);

    if (vigenere_device) {
        vigenere_trim(vigenere_device);
        cdev_del(&vigenere_device->cdev);
        kfree(vigenere_device);
    }

    unregister_chrdev_region(devno, 1);
}


int vigenere_init_module(void)
{
    int result;
    int err;
    dev_t devno = 0;
    struct vigenere_dev *dev;


    if (vigenere_major) {
        devno = MKDEV(vigenere_major, vigenere_minor);
        result = register_chrdev_region(devno, 1, "vigenere");
    } else {
        result = alloc_chrdev_region(&devno, vigenere_minor, 1,
                                     "vigenere");
        vigenere_major = MAJOR(devno);
    }
    if (result < 0) {
        printk(KERN_WARNING "vigenere: can't get major %d\n", vigenere_major);
        return result;
    }

    vigenere_device = kmalloc(sizeof(struct vigenere_dev),
                            GFP_KERNEL);
    if (!vigenere_device) {
        result = -ENOMEM;
        goto fail;
    }
    memset(vigenere_device, 0, sizeof(struct vigenere_dev));

    /* Initialize each device. */
    dev = vigenere_device;
    //dev->quantum = scull_quantum;
    //dev->qset = scull_qset;
    dev->key = vigenere_key;
    dev->key_length = util_lenght(dev->key);
    sema_init(&dev->sem,1);
    dev->mode=0;
    devno = MKDEV(vigenere_major, vigenere_minor);
    cdev_init(&dev->cdev, &vigenere_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &vigenere_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
        printk(KERN_NOTICE "Error %d adding vigenere%d", err, 0);

    return 0; /* succeed */

  fail:
    vigenere_cleanup_module();
    return result;
}

int util_lenght(char* string)
{
    int i;
    for(i = 0; string[i] != '\0'; i++);
    return i;
}

int util_cmp(char* s1,char* s2)
{
    int i;
    int l1=util_lenght(s1),l2=util_lenght(s2);
    if (l1!=l2)return 0;

    for(i = 0; i<l1; i++)
    {
        if(s1[i]!=s2[i])
            return 0;
    }
    return 1;
}

module_init(vigenere_init_module);
module_exit(vigenere_cleanup_module);
