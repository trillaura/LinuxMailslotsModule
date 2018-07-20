
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/pid.h>    
#include <linux/tty.h> 
#include <linux/version.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>

#define DEVICE_NAME "mailslot"

#define DEVICE_FILE_SIZE 256

#define MAXIMUM_MESSAGE_SIZE 424
//module_param(MAXIMUM_MESSAGE_SIZE, int, 0666);
//static int NEXT_MESSAGE_SIZE = 424;
//module_param(NEXT_MESSAGE_SIZE, int, 0666);

#define MAXIMUM_SLOT_STORAGE (424*100)
//module_param(MAXIMUM_SLOT_STORAGE, int, 0666);

#define MINOR_MIN 0
//module_param(MINOR_MIN, int, 0666);

#define MINOR_MAX (MINOR_MIN + DEVICE_FILE_SIZE - 1)

static int Major;

typedef struct mailslot_msg {
        char *data;
        unsigned long size;
} mailslot_msg_t;

typedef struct mailslot {
	struct mailslot_msg_t *first;
	int msg_size;
	int mailslot_size;
	// synchr
} mailslot_t;

static mailslot_t mailslot[DEVICE_FILE_SIZE];

static ssize_t mailslot_read(struct file *, char __user *, size_t, loff_t *); 
static ssize_t mailslot_write(struct file *, const char __user *, size_t, loff_t *); 
static int mailslot_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
static int mailslot_open(struct inode *, struct file *); 
static int mailslot_release(struct inode *, struct file *); 

static ssize_t mailslot_read(struct file *filp, char __user *buff, size_t len, loff_t *off) {

	printk("Called a read on mailslot with minor number %d\n", filp->f_path.dentry->d_inode->i_rdev);
	
	// get first message 

	// if number of bytes to read is less than message size return -1

	// if fifo is empty return 0

	//copy_to_user message in buff
	char *hello = "Hello, world\n";
	copy_to_user(buff, hello, sizeof(hello));

	// return number of bytes read (message lenght)
	return (ssize_t) 0;
}
 
static ssize_t mailslot_write(struct file *filp, const char *buff, size_t len, loff_t *off) {

	printk("Called a write on mailslot with minor number %d\n", filp->f_path.dentry->d_inode->i_rdev);

	// if mailslot full return -1

	// copy_from_user from buff to message data

	// set new message at tail

	// return written bytes (message lenght)
	return 0;
} 

static int mailslot_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
	// todo
	return 0;
}

static int mailslot_open(struct inode *inode, struct file *file) {
	/*
	int Minor = file->f_path.dentry->d_inode->i_rdev
	if (Minor < MINOR_MIN || Minor > MINOR_MAX) { 
		printk(KERN_INFO "Minor %d out of range.\n", Minor);
		return -1;
	} 
	*/
	
	return 0;
}

static int mailslot_release(struct inode *inode, struct file *file) {
	// to do deallocation
	return 0;
} 

static struct file_operations fops = { 
	.owner   = THIS_MODULE,
	.read    = mailslot_read,
        .write   = mailslot_write,
//      .ioctl   = mailslot_ioctl,
        .open    = mailslot_open,
        .release = mailslot_release    
};

int init_module(void) {

        Major = register_chrdev(0, DEVICE_NAME, &fops);

        if (Major < 0) {
          printk("Registering noiser device failed\n");
          return Major;
        }

        printk(KERN_INFO "Mailslot device registered, it is assigned major number %d\n", Major);

        return 0;
}

void cleanup_module(void) {

        unregister_chrdev(Major, DEVICE_NAME);

        printk(KERN_INFO "Mailslot device unregistered, it was assigned major number %d\n", Major);
} 
