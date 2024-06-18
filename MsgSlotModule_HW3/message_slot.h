#undef __KERNEL__
#define __KERNEL__

#undef __MODULE__
#define __MODULE__
 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/init.h> 

#define MAJOR_NUM 235
#define MAX_MSG_SIZE 128
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM,0,unsigned int)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oday");
MODULE_DESCRIPTION("Assignment 3 in Operating Systems Course!!");

static int is_minor_exists(int minor_num);
static int add_msg_slot(int minor_num);
static struct Message_Slot* get_msg_slot(int minor_num);
static struct Message_Channel* add_chann_to_slot(unsigned int id, struct Message_Slot *slot);

static int __init message_slot_initialize(void);
static int device_open(struct inode *inode,struct file *file);
static int device_release(struct inode *inode,struct file *file);
long device_ioctl(struct file *filp,unsigned int cmd, unsigned long arg);
ssize_t device_read(struct file *filp,char __user *buf,size_t count,loff_t *f_pos);
ssize_t device_write(struct file *filp,const char __user *buf,size_t count,loff_t *f_pos);


/*
the data structure i defined is:
a linked list for the message_slots,
each message slot has a linked list of its channels
*/
struct Message_Slot
{
    unsigned int minor_number;
    struct Message_Slot *next_msg_slot;
    struct Message_Channel *channels;
    
};

struct Message_Channel{
    unsigned int channel_id;
    struct Message_Channel *next_channel;
    char *msg;
    size_t msg_size;

};

static struct  file_operations fops=
{
    .owner=THIS_MODULE,
    .open=device_open,
    .read=device_read,
    .write=device_write,
    .unlocked_ioctl=device_ioctl,
    .release=device_release,
};

static struct Message_Slot *msg_slots=NULL;