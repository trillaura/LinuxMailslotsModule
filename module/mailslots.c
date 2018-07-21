//#include "mailslots.h"

#define EXPORT_SYMTAB
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/pid.h>    
#include <linux/tty.h> 
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/string.h>
//#include "queue.h"

#define DEBUG if(1)

#define DEVICE_NAME "mailslot"

#define DEVICE_FILE_SIZE 256

//module_param(MAXIMUM_MESSAGE_SIZE, int, 0666);
//static int NEXT_MESSAGE_SIZE = 424;
//module_param(NEXT_MESSAGE_SIZE, int, 0666);

#ifndef MAXIMUM_SLOT_STORAGE
#define MAXIMUM_SLOT_STORAGE (256*100)
#endif

//module_param(MAXIMUM_SLOT_STORAGE, int, 0666);

#define MINOR_MIN 0

//module_param(MINOR_MIN, int, 0666);

#define MINOR_MAX (MINOR_MIN + DEVICE_FILE_SIZE - 1)

static int MAXIMUM_MESSAGE_SIZE = 256;
static int POLICY = 0; // 0 blocking, 1 non-blocking

static int Major;

typedef struct mailslot_msg_t {
        struct mailslot_msg_t *next;
        char data[MAXIMUM_MESSAGE_SIZE];
        unsigned int size;
} mailslot_msg_t;

typedef struct mailslot_t {
        mailslot_msg_t *first;
        mailslot_msg_t *last;
        size_t size;
} mailslot_t;

static mailslot_t mailslot[DEVICE_FILE_SIZE];

static ssize_t mailslot_read(struct file *, char __user *, size_t, loff_t *); 
static ssize_t mailslot_write(struct file *, const char __user *, size_t, loff_t *); 
static int mailslot_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
static int mailslot_open(struct inode *, struct file *); 
static int mailslot_release(struct inode *, struct file *); 

static mailslot_t mailslot[DEVICE_FILE_SIZE];

void print_queue(int i) {
    
        mailslot_msg_t *head = mailslot[i].first;
        mailslot_msg_t *tail = mailslot[i].last;
    
        //printf("Queue:\n");
        printk("Queue:\t");
        while (head != tail) {
                printk(", "); 
                printk("msg: %s size: %u", head->data, head->size);
                head = head->next;
        }
        printk("\n");
}

void initialize(int minor) {
    
        mailslot[minor].size = 0;
        mailslot[minor].first = NULL;
        mailslot[minor].last = NULL;
}

mailslot_msg_t *new_message(char *text, size_t len) {
    
        mailslot_msg_t *new = (mailslot_msg_t *) kmalloc(sizeof(mailslot_msg_t), GFP_KERNEL);

        if (new == NULL) {
                DEBUG printk("Error in kmalloc.\n");
                return NULL;
        }

        strcpy(new->data, text);
        new->size = len;
        new->next = NULL;

        return new;
}

int is_empty(int minor) {
        return mailslot[minor].last == NULL;
}

int is_full(int minor, size_t len) {
        return mailslot[minor].size + len > MAXIMUM_SLOT_STORAGE;
}
int enqueue(int minor, char *new_msg) {

        size_t len = strlen(new_msg);

        if (len > MAXIMUM_MESSAGE_SIZE) {
                DEBUG printk("Message too large.\n");
                return -1;
        }

        if (is_full(minor, len)) {
                DEBUG printk("Full mailslot.\n");
                return -1;
        }

        mailslot_msg_t *new = new_message(new_msg, len);

        if (new == NULL)
                return -1;

        if (!is_empty(minor)) {
                mailslot[minor].last->next = new;
                mailslot[minor].last = new;
        } else {
                DEBUG printk("Empty mailslot.\n");
                mailslot[minor].first = mailslot[minor].last = new;
        }
        mailslot[minor].size += len;

        DEBUG print_queue(minor);

        return 0;
}

char *dequeue(int minor, mailslot_msg_t *head) {

        DEBUG printk("Before...");
        DEBUG print_queue(minor);

        char *message = head->data;
        mailslot[minor].first = mailslot[minor].first->next;
        mailslot[minor].size--;
        kfree(head);

        // TODO sveglia writer bloccati perché full

        DEBUG printk("After...");
        DEBUG print_queue(minor);

        return message;
}

char *dequeue_by_len(int minor, ssize_t len) {

        if (is_empty(minor)) {
                DEBUG printk("Empty mailslot.\n");
                return "\0";
        }

        mailslot_msg_t *head = mailslot[minor].first;

        if (strlen(head->data) <= len) {
                return dequeue(minor, head);
        } else {
                DEBUG print_queue(minor);
                return NULL;
        }
}


int get_minor(struct file *filp) {
	return MINOR(filp->f_path.dentry->d_inode->i_rdev);
}

static ssize_t mailslot_read(struct file *filp, char __user *buff, size_t len, loff_t *off) {
	
	int Minor = get_minor(filp); 
	
	DEBUG printk("Called a read on mailslot with minor number %d\n", Minor);
	
        char *next = dequeue_by_len(Minor, len);
    
        if (next == NULL) {
                printk(KERN_INFO "Buffer %u too small for message size.\n", len);
                return -1; 
        }

	if (copy_to_user(buff, next, strlen(next)) != 0) {
                printk(KERN_INFO "copy to user err\n");
                return -EFAULT;
        } else {
                printk(KERN_INFO "copy to user OK\n");
        }
	
	return strlen(next);
}
 
static ssize_t mailslot_write(struct file *filp, const char *buff, size_t len, loff_t *off) {

	DEBUG printk("Called a write on mailslot with minor number %d\n", get_minor(filp));

	char new_msg[MAXIMUM_MESSAGE_SIZE];
	// copy_from_user from buff to message data
	copy_from_user(new_msg, buff, len);

	// return written bytes (message lenght)
	return len;
} 

static int mailslot_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
	// todo
	DEBUG printk("Called an ioctl on mailslot with minor number %d\n", get_minor(filp));
	
	switch (cmd) {
		case 0: 
			DEBUG printk("Set blocking(0) or non-blocking(1) policy command.\n");	
			POLICY = (int) arg
			break;	
		/*case 1:	
			DEBUG printk("Set non-blocking policy command.\n");	
			// todo
			break;
		*/
		case 1: 
                        DEBUG printk("Change maximum message size command.\n");  
                        MAXIMUM_MESSAGE_SIZE = (int) arg;
                        break;
		default:	
			DEBUG printk("Not supported command.\n");
	}	

	return 0;
}

static int mailslot_open(struct inode *inode, struct file *file) {
	
	int Minor = get_minor(file);
	
	DEBUG printk(KERN_INFO "Opened mailslot instance with minor %d\n", Minor);

	if (Minor < MINOR_MIN || Minor > MINOR_MAX) { 
		DEBUG printk(KERN_INFO "Minor %d out of range.\n", Minor);
		return -1;
	} 

	initialize(Minor);
	
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
          DEBUG printk("Registering noiser device failed\n");
          return Major;
        }

        DEBUG printk(KERN_INFO "Mailslot device registered, it is assigned major number %d\n", Major);

        return 0;
}

void cleanup_module(void) {

        unregister_chrdev(Major, DEVICE_NAME);

        DEBUG printk(KERN_INFO "Mailslot device unregistered, it was assigned major number %d\n", Major);
} 
