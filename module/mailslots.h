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
#include "queue.h"

#define DEBUG if(1)

#define DEVICE_NAME "mailslot"

#define DEVICE_FILE_SIZE 256

#ifndef MAXIMUM_MESSAGE_SIZE
#define MAXIMUM_MESSAGE_SIZE 256
#endif

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

static int Major;

//static mailslot_t mailslot[DEVICE_FILE_SIZE];
//EXPORT_SYMBOL(mailslot);

static ssize_t mailslot_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t mailslot_write(struct file *, const char __user *, size_t, loff_t *);
static int mailslot_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
static int mailslot_open(struct inode *, struct file *);
static int mailslot_release(struct inode *, struct file *);

//extern int initialize(int minor);
