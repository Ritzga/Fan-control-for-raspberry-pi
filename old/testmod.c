#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Fox");
MODULE_DESCRIPTION("MyModule");
MODULE_VERSION("0.1a");



static struct hrtimer htimer;
static ktime_t kt_periode;
static void timer_cleanup(void);
static enum hrtimer_restart timer_function(struct hrtimer * timer);
static int driver_open(struct inode*geraete_datei, struct file *instanz );
static int driver_close(struct inode*geraete_datei, struct file *instanz );
static ssize_t driver_read(struct file *instanz,char *User, size_t Count, loff_t *offset );
static ssize_t driver_write(struct file *Instanz,const char *User, size_t Count, loff_t *offs);

static struct file_operations fops = {
   read: driver_read,
   write: driver_write,
   open: driver_open,
   release: driver_close
};


static int driver_open(struct inode*geraete_datei, struct file *instanz )
{
 printk(KERN_ALERT "SD Open\n");

 //param = ' ';
 return 0;
}

static int driver_close(struct inode*geraete_datei, struct file *instanz )
{
 printk(KERN_ALERT "SD Close\n");
 return 0;
}

char Buffer[10] = "!";
static ssize_t driver_read(struct file *instanz,char *User, size_t Count, loff_t *offset )
{
 uint32_t ret;
 printk(KERN_ALERT "SD Read\n");
 ret = copy_to_user(User, Buffer, 1);
 
 if (ret != 0) printk(KERN_ALERT "SD Fail toRead\n");


 return 1;
}

static ssize_t driver_write(struct file *Instanz,const char *User, size_t Count, loff_t *offs)
{
 uint32_t ret;
 // Copy data from userspace
 
 
 ret = copy_from_user(Buffer, User, Count); 
 printk(KERN_ALERT "SD Write\n");

 return Count;
}

static int timer_init(void)
{    
    if(register_chrdev(200, "testmod", &fops) == 0)
    {
        kt_periode = ktime_set(0, 500000000); //seconds,nanoseconds
        hrtimer_init (& htimer, CLOCK_REALTIME, HRTIMER_MODE_REL);
        htimer.function = timer_function;
        hrtimer_start(& htimer, kt_periode, HRTIMER_MODE_REL);
    
        return 0;
    }
    else
    {
        return -EIO;
    }    
    
}

static void timer_cleanup(void)
{
    hrtimer_cancel(& htimer);
}

static enum hrtimer_restart timer_function(struct hrtimer * timer)
{
    // @Do your work here. 
    
    hrtimer_forward_now(timer, kt_periode);

    return HRTIMER_RESTART;
}

module_init(timer_init);
module_exit(timer_cleanup);
