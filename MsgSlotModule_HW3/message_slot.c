#include "message_slot.h"

/*the next function returns 1 if a minor number exists in msg_slots, else 0*/
static int is_minor_exists(int minor_num){
    if(msg_slots==NULL){
        return 0;
    }
    else{
        struct Message_Slot *temp=msg_slots;
        while(temp!=NULL){
            if (temp->minor_number==minor_num){return 1;}
            temp=temp->next_msg_slot;
        }
    }
    return 0;
}
/*the next function adds a new message slot with the provided minor number
returns 1 in success,otherwise 0*/
static int add_msg_slot(int minor_num){
    if(msg_slots==NULL){
        msg_slots=kmalloc(sizeof(struct Message_Slot),GFP_KERNEL);
        if(msg_slots==NULL){return 0;}
        msg_slots->minor_number=minor_num;
        msg_slots->channels=NULL;
        msg_slots->next_msg_slot=NULL;
    }
    else{
        struct Message_Slot *temp=msg_slots;
        while(temp->next_msg_slot!=NULL){temp=temp->next_msg_slot;}
        temp->next_msg_slot=kmalloc(sizeof(struct Message_Slot),GFP_KERNEL);
        temp=temp->next_msg_slot;
        if(temp==NULL){return 0;}
        temp->minor_number=minor_num;
        temp->channels=NULL;
        temp->next_msg_slot=NULL;
    }
    return 1;
}
/*the next function returns a pointer to the message slot with the provided minor number*/
static struct Message_Slot* get_msg_slot(int minor_num){
    struct Message_Slot *temp=msg_slots;
    while(temp!=NULL){
        if(temp->minor_number==minor_num){return temp;}
        temp=temp->next_msg_slot;
    }
    return NULL;
}
/*the next function adds a new channel to the current message slot(provided) with the provided id and returns its pointer*/
static struct Message_Channel* add_chann_to_slot(unsigned int id, struct Message_Slot *slot){
    struct Message_Channel *temp;
    if(slot->channels==NULL){
        slot->channels=kmalloc(sizeof(struct Message_Channel),GFP_KERNEL);
        slot->channels->channel_id=id;
        slot->channels->next_channel=NULL;
        slot->channels->msg=NULL;
        slot->channels->msg_size=0;
        return slot->channels;

    }
    temp=slot->channels;
    while(temp->next_channel!=NULL){
        temp=temp->next_channel;
    }
    temp->next_channel=kmalloc(sizeof(struct Message_Channel),GFP_KERNEL);
    temp=temp->next_channel;
    temp->channel_id=id;
    temp->next_channel=NULL;
    temp->msg=NULL;
    temp->msg_size=0;
    return temp;

}
/*the next function returns a pointer to the channel in the current message slot(provided with the provided id*/
static struct Message_Channel* get_chann_from_slot(unsigned int id, struct Message_Slot *slot){
    struct Message_Channel *temp=slot->channels;
    while(temp!=NULL){
        if(temp->channel_id==id){
            return temp;
        }
        temp=temp->next_channel;
    }
    return NULL;
}

static int __init message_slot_initialize(void){
    int init_status;
    init_status=register_chrdev(MAJOR_NUM,"message slot",&fops);
    if(init_status<0){
        printk(KERN_ERR "An error has occured registering,with status:%d\n",init_status);
        return init_status;
    }
    printk(KERN_INFO "Successfully registered.");
    return 0;
}

static int device_open(struct inode *inode,struct file *file){
    int minor=iminor(inode);
    if(is_minor_exists(minor)==0){
        if(add_msg_slot(minor)==1){
            printk(KERN_INFO "Message slot with minor number:%d successfully opened right now.",minor);
        }
        else{
            /*add_msg_slot fails iff failed to allocate memory*/
            return -ENOMEM;
        }
    }
    else{
        printk(KERN_INFO "Message slot with minor number:%d was already opened before.",minor);
    }
    return 0;
    /*note: i returned 0 assuming that this indicates success,
     and i haven't seen any guidelines regarding to errors, so i returned -EINVAL in that case*/

}

long device_ioctl(struct file *filp,unsigned int cmd, unsigned long arg){
    struct Message_Slot *msg_slot;
    struct Message_Channel *channel;
    switch (cmd)
    {
    case MSG_SLOT_CHANNEL:
        if(arg==0){return -EINVAL;}
        msg_slot=get_msg_slot(iminor(filp->f_path.dentry->d_inode));
        if(filp->f_mode & FMODE_READ){
            channel=get_chann_from_slot((unsigned int)arg, msg_slot);
            filp->private_data=channel;
        }
        if(filp->f_mode & FMODE_WRITE){
            channel=get_chann_from_slot((unsigned int)arg, msg_slot);
            if(channel==NULL){
                channel=add_chann_to_slot((unsigned int)arg, msg_slot);
            }
            filp->private_data=channel;
        }
        break;
    
    default:
        return -EINVAL;
    }
    return 0;
}

ssize_t device_read(struct file *filp,char __user *buf,size_t count,loff_t *f_pos){
    struct Message_Channel *channel=filp->private_data;
    if(channel ==NULL){
        return -EINVAL;
    }
    if(channel->msg==NULL){
        return -EWOULDBLOCK;
    }
    if(count < channel->msg_size){
        return -ENOSPC;
    }

    if(copy_to_user(buf,channel->msg,channel->msg_size)){
        return -EFAULT;
    }
    return channel->msg_size;

}

ssize_t device_write(struct file *filp,const char __user *buf,size_t count,loff_t *f_pos){
    struct Message_Channel *channel=filp->private_data;
    if(channel ==NULL){return -EINVAL;}
    if(count==0 || count>MAX_MSG_SIZE){return -EMSGSIZE;}
    kfree(channel->msg);
    channel->msg=kmalloc(count,GFP_KERNEL);
    if(channel->msg==NULL){return -ENOMEM;}
    channel->msg_size=count;

    if(copy_from_user(channel->msg,buf,count)){
        kfree(channel->msg);
        channel->msg=NULL;
        channel->msg_size=0;
        return -EFAULT;
    }
    return channel->msg_size;
}

static int device_release(struct inode *inode,struct file *file){
    printk(KERN_INFO "The device was successfully closed.");
    return 0;
}

static void __exit message_slot_exit(void){
    struct Message_Channel *temp_chann1,*temp_chann2;
    struct Message_Slot *temp_slot;
    while(msg_slots!=NULL){
        temp_chann1=msg_slots->channels;
        while(temp_chann1!=NULL){
            temp_chann2=temp_chann1->next_channel;
            kfree(temp_chann1);
            temp_chann1=temp_chann2;
        }
        temp_slot=msg_slots->next_msg_slot;
        kfree(msg_slots);
        msg_slots=temp_slot;
    }
    unregister_chrdev(MAJOR_NUM,"message_slot");
    printk(KERN_INFO "Successfully unregistered.");
}

module_init(message_slot_initialize);
module_exit(message_slot_exit);
