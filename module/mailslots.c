/*
 * @license GPL
 * @author Laura Trivelloni
 * 
 * This module implements a driver of a special device file 
 * that is accessible according to FIFO style semantic, 
 * posting or delivering data units atomically and in data separation,
 * and that is configurable via ioctl interface.
 */


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
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <asm/atomic.h>

#define DEBUG if(0)

#define DEVICE_NAME "mailslot"

#define DEVICE_FILE_SIZE 256

#define MINOR_MIN 0
#define MINOR_MAX (MINOR_MIN + DEVICE_FILE_SIZE - 1)

#define STORAGE_UPPER_LIMIT 256*1000
static int MAXIMUM_SLOT_STORAGE = 256*100;

static int MAXIMUM_MESSAGE_SIZE = 256;

static int POLICY = 0; // 1 blocking, 0 non-blocking

static int Major;

typedef struct mailslot_msg_t {
        struct mailslot_msg_t *next;
        char *data;
        unsigned int size;
} mailslot_msg_t;

typedef struct mailslot_t {
        mailslot_msg_t *first;
        mailslot_msg_t *last;
        size_t size;
	struct mutex mux;

//	atomic_t reads;
//	atomic_t writes;

	wait_queue_head_t reader_queue;
	wait_queue_head_t writer_queue;
} mailslot_t;

static mailslot_t mailslot[DEVICE_FILE_SIZE];

static ssize_t mailslot_read(struct file *, char __user *, size_t, loff_t *); 
static ssize_t mailslot_write(struct file *, const char __user *, size_t, loff_t *); 
static long mailslot_ioctl(struct file *, unsigned int, unsigned long);
static int mailslot_open(struct inode *, struct file *); 
static int mailslot_release(struct inode *, struct file *); 

void initialize(void) {
   	
	int minor;
	for (minor = MINOR_MIN; minor <= MINOR_MAX; minor++) {

		DEBUG printk(KERN_INFO "Initialize minor %d.\n", minor);

		mutex_init(&mailslot[minor].mux); 
		init_waitqueue_head(&mailslot[minor].reader_queue);
		init_waitqueue_head(&mailslot[minor].writer_queue);
        	mailslot[minor].size = 0;
        	mailslot[minor].first = NULL;
        	mailslot[minor].last = NULL;
        	
//		atomic_set(&mailslot[minor].writes,0);
//        	atomic_set(&mailslot[minor].reads,0);
	}
}

mailslot_msg_t *new_message(char *text, size_t len) {
    
	DEBUG printk(KERN_INFO "New message function.\n");
        
	mailslot_msg_t *new = (mailslot_msg_t *) kmalloc(sizeof(mailslot_msg_t), GFP_KERNEL);

        if (new == NULL) {
                DEBUG printk(KERN_INFO "Error in kmalloc.\n");
                return NULL;
        }

	new->data = (char *) kmalloc(sizeof(char)*MAXIMUM_MESSAGE_SIZE, GFP_KERNEL);
        strcpy(new->data, text);
        new->size = len;
        new->next = NULL;

        DEBUG printk(KERN_INFO "Created new message mailslot.\n");
        return new;
}

int is_empty(int minor) {
        return mailslot[minor].size == 0;
}

int is_full(int minor, size_t len) {
        return mailslot[minor].size + len > MAXIMUM_SLOT_STORAGE;
}

int enqueue(int minor, char *new_msg) {
	
	DEBUG printk(KERN_INFO "Enqueue message '%s' (minor %d).\n", new_msg, minor);

        size_t len = strlen(new_msg);

        if (len > MAXIMUM_MESSAGE_SIZE) {
                DEBUG printk(KERN_INFO "Message too large.\n");
                return -1;
        }

        mailslot_msg_t *new = new_message(new_msg, len);
        
	if (new == NULL)
                return -1;

	mutex_lock(&mailslot[minor].mux);
	
        while (is_full(minor, len)) {
                DEBUG printk(KERN_INFO "Full mailslot.\n");
		
		mutex_unlock(&mailslot[minor].mux);
		
		if (!POLICY) { 	// non-blocking policy
                	kfree(new);
			return -1; 
		} else { 			// blocking policy
			wait_event_interruptible(mailslot[minor].writer_queue, !is_full(minor, len));
			mutex_lock(&mailslot[minor].mux);
		}
        }

        if (!is_empty(minor)) {
                mailslot[minor].last->next = new;
                mailslot[minor].last = new;
        } else {
                DEBUG printk(KERN_INFO "Empty mailslot.\n");
                mailslot[minor].first = mailslot[minor].last = new;
                DEBUG printk(KERN_INFO "Enqueue %s in mailslot.\n", mailslot[minor].first->data);
        }
        mailslot[minor].size += len;
	
	mutex_unlock(&mailslot[minor].mux);

	// wake up sleeping reader waiting for a not-empty queue
	wake_up_interruptible(&mailslot[minor].reader_queue);
	
        return 0;
}

char *dequeue(int minor, mailslot_msg_t *head) {

	DEBUG printk(KERN_INFO "Dequeue message (minor %d).\n", minor);
        
        char *message = head->data;
        mailslot[minor].first = mailslot[minor].first->next;
        mailslot[minor].size -= strlen(message);
        kfree(head);

        // wake up sleeping writer waiting for not-full mailslot
	wake_up_interruptible(&mailslot[minor].writer_queue);
	
        return message;
}

char *dequeue_by_len(int minor, ssize_t len) {

	DEBUG printk(KERN_INFO "Dequeue by len function.\n");

	mutex_lock(&mailslot[minor].mux);
		        
	while (is_empty(minor)) {

		mutex_unlock(&mailslot[minor].mux);

                DEBUG printk(KERN_INFO "Empty mailslot.\n");
                
		if (!POLICY) { 	// non-blocking 
			DEBUG printk("Empty non blocking return\n");
			return "\0";
		} else {
			DEBUG printk("Empty blocking waiting\n");
			wait_event_interruptible(mailslot[minor].reader_queue, !is_empty(minor));
			mutex_lock(&mailslot[minor].mux);	
		}	
        }

        mailslot_msg_t *head = mailslot[minor].first;

        if (strlen(head->data) <= len) {
                char *read = dequeue(minor, head);
		mutex_unlock(&mailslot[minor].mux);
		return read;
        }
	mutex_unlock(&mailslot[minor].mux);

	// wake up other sleeping readers waiting for a not-empty queue
	wake_up_interruptible(&mailslot[minor].reader_queue);
        
	return NULL;        
}


int get_minor(struct file *filp) {
	return MINOR(filp->f_path.dentry->d_inode->i_rdev);
}

static ssize_t mailslot_read(struct file *filp, char __user *buff, size_t len, loff_t *off) {
	
	int Minor = get_minor(filp); 
	
//	atomic_inc(&mailslot[Minor].reads);

	DEBUG printk(KERN_INFO "Called read on mailslot with minor number %d\n", Minor);
	
        char *next = dequeue_by_len(Minor, len);
    
        if (next == NULL) {
		
                printk(KERN_INFO "Buffer %u too small for message size.\n", len);
                return -1; 
        }

	if (copy_to_user(buff, next, strlen(next)) != 0) {
                printk(KERN_INFO "ERROR Copy to user.\n");
                return -EFAULT;
        } 

	DEBUG printk("Copied to user '%s'\n", next);	
	
	return strlen(next);
}
 
static ssize_t mailslot_write(struct file *filp, const char *buff, size_t len, loff_t *off) {

	int Minor = get_minor(filp);

//	atomic_inc(&mailslot[Minor].writes);
	
	DEBUG printk(KERN_INFO "Called write on mailslot with minor number %d\n", Minor);

	char new_msg[MAXIMUM_MESSAGE_SIZE];
	
	if (copy_from_user(new_msg, buff, len) != 0) {
		DEBUG printk(KERN_INFO "ERROR Copy from user.\n");
		return -EFAULT;
	}
	DEBUG printk("Copied from user '%s'\n", new_msg);
	
	enqueue(get_minor(filp), new_msg);
	
	return len;
} 

static long mailslot_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {

	DEBUG printk(KERN_INFO "Called an ioctl on mailslot with minor number %d\n", get_minor(filp));
	
	if (cmd == 0) {
		int policy = (int) arg;
		if (!policy) 
			DEBUG printk(KERN_INFO "Set non-blocking (0) policy command.\n");	
			if (policy) 
			DEBUG printk(KERN_INFO "Set blocking (1) policy command.\n");	
		POLICY = policy;
		return 0;
        }
	if (cmd == 1) {  
              	DEBUG printk("Change maximum message size command.\n");  
		MAXIMUM_MESSAGE_SIZE = (int) arg;
		return 0;
	}
	if (cmd == 2) {  
              	DEBUG printk("Change maximum mailslot storage command.\n");  
		if ((int) arg > STORAGE_UPPER_LIMIT)
			MAXIMUM_SLOT_STORAGE = STORAGE_UPPER_LIMIT;
		MAXIMUM_SLOT_STORAGE = (int) arg;
		return 0;
	}
	DEBUG printk("Not supported command.\n");
	return -1;
}

static int mailslot_open(struct inode *inode, struct file *file) {
	
	int Minor = get_minor(file);
	
	DEBUG printk(KERN_INFO "Opened mailslot instance with minor %d\n", Minor);

	if (Minor < MINOR_MIN || Minor > MINOR_MAX) { 
		DEBUG printk(KERN_INFO "Minor %d out of range.\n", Minor);
		return -1;
	} 
	return 0;
}

static int mailslot_release(struct inode *inode, struct file *file) {
	int Minor = get_minor(file);
	
	DEBUG printk(KERN_INFO "Closed mailslot instance with minor %d\n", Minor);
	
//	printk(KERN_INFO "Called %d writes and %d reads on mailslot with minor number %d\n", 
//		atomic_read(&mailslot[Minor].writes), atomic_read(&mailslot[Minor].reads), Minor);
	
	return 0;
} 

static struct file_operations fops = { 
	.read   	= mailslot_read,
        .write   	= mailslot_write,
	.unlocked_ioctl = mailslot_ioctl,
        .open    	= mailslot_open,
        .release 	= mailslot_release    
};

int init_module(void) {

        Major = register_chrdev(0, DEVICE_NAME, &fops);

        if (Major < 0) {
          DEBUG printk("Registering device failed\n");
          return Major;
        }

        printk(KERN_INFO "Mailslot device registered, it is assigned major number %d\n", Major);

	initialize();
	
        return 0;
}

void cleanup_module(void) {

        unregister_chrdev(Major, DEVICE_NAME);

        printk(KERN_INFO "Mailslot device unregistered, it was assigned major number %d\n", Major);
} 
