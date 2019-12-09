/*
sudo rm -r -f testi2c
sudo mknod /dev/testi2c c 201 0
make
sudo rm /lib/modules/4.19.75-v7+/testmodi2c.ko
sudo ln ~/sysprog/testmodi2c.ko /lib/modules/4.19.75-v7+
sudo depmod -a
sudo modprobe testmodi2c
*/


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
//Header für i2c
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
//Header für timer
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
//Header für device interface
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nobody");
MODULE_DESCRIPTION("MyModule");
MODULE_VERSION("0.1a");


#define BLOCK_DATA_MAX_TRIES 10
#define DEVICE_NAME "testi2c"
#define SUCCESS 0
#define DEVICE_NAME "chardev"	/* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80		/* Max length of the message from the device */

static struct hrtimer htimer;
static ktime_t kt_periode;
static void timer_cleanup(void);
static enum hrtimer_restart timer_function(struct hrtimer * timer);
static int driver_open(struct inode*geraete_datei, struct file *instanz );
static int driver_close(struct inode*geraete_datei, struct file *instanz );
static ssize_t driver_read(struct file *filp, char *buffer,  size_t length, loff_t * offset);
static ssize_t driver_write(struct file *Instanz,const char *User, size_t Count, loff_t *offs);

static char msg[BUF_LEN];	/* The msg the device will give when asked */
static char *msg_Ptr;
static int Device_Open = 0;

static struct file_operations fops = {
   read: driver_read,
   write: driver_write,
   open: driver_open,
   release: driver_close
};


//=======================================================================
//i2c Functionality

static s32 bme280_read_block_data_once(const struct i2c_client *client, u8 command, u8 length, u8 *values)
{
	s32 i, data;
	
	for (i=0; i < length; i++)
	{
		data = i2c_smbus_read_byte_data(client, command+1);
		
		if(data<0)
			return data;
		values[i]=data;
	}
}
static s32 bme280_read_block_data(const struct i2c_client *client, u8 command, u8 length, u8 *values)
{
	u8 oldvalues[255];
	s32 ret;
	int tries = 0;
	
	dev_dbg(&client->dev, "bme280_read_block_data(length=%d)\n", length);
	ret = bme280_read_block_data_once(client, command, length, values);
	if(ret<0) return ret;
	
	do{	
		if (++tries > BLOCK_DATA_MAX_TRIES){
				dev_err(&client->dev, "bme280_read_block_data failed\n");
				return -EIO;
		}
		memcpy(oldvalues, values, length);
		ret=bme280_read_block_data_once(client, command, length, values);
		
		if(ret<0) return ret;
	}while(memcmp(oldvalues, values, length));
}
static s32 bme280_write_block_data(const struct i2c_client *client, u8 command, u8 length, const u8 *values)
{
		u8 currentvalues[255];
		int tries=0;
		
		dev_dbg(&client->dev, "bme280_write_block_data(length=%d)\n", length);
		
		do{
				s32 i, ret;
				
				if(++tries>BLOCK_DATA_MAX_TRIES){
					dev_err(&client->dev, "bme_write_block_data failed\n");
					return -EIO;
				}
				for(i = 0; i < length; i++){
					ret=i2c_smbus_write_byte_data(client, command+i, values[i]);
					
					if (ret<0) return ret;
				}
				
				ret=bme280_read_block_data_once(client, command, length, currentvalues);
				
				if (ret<0) return ret;
		}while (memcmp(currentvalues, values, length));
		
		
		return length;
}
static struct i2c_board_info stm32_i2c_test_board_info = {
	I2C_BOARD_INFO("bme280", 0x76)
};

//=======================================================================
//File Interface for userspace


static int driver_open(struct inode*geraete_datei, struct file *instanz )
{
	static int counter = 0;

	if (Device_Open)
		return -EBUSY;

	Device_Open++;
	
	
	sprintf(msg, "I already told you %d times Hello world!\n", counter++);
	msg_Ptr = msg;
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

static int driver_close(struct inode*geraete_datei, struct file *instanz )
{
	Device_Open--;		
	
	module_put(THIS_MODULE);

	return 0;
}


static ssize_t driver_read(struct file *filp, char *buffer,  size_t length, loff_t * offset)
{
	int bytes_read = 0;

	 //return 0 signifying end of file 
	if (*msg_Ptr == 0)
		return 0;
		
	while (length && *msg_Ptr) {
		put_user(*(msg_Ptr++), buffer++);

		length--;
		bytes_read++;
	}

	/* 
	 * Most read functions return the number of bytes put into the buffer
	 */
	return bytes_read;
}

static ssize_t driver_write(struct file *Instanz,const char *User, size_t Count, loff_t *offs)
{
	pr_alert("Tried to write in a READ-ONLY file.");

	return 0;
}

//=======================================================================
//Timer Functionality

static enum hrtimer_restart timer_function(struct hrtimer * timer)
{
    // @Do your work here. 
    
    hrtimer_forward_now(timer, kt_periode);

    return HRTIMER_RESTART;
}

//=======================================================================
static int timer_init(void)
{    
    if(register_chrdev(201, DEVICE_NAME, &fops) == 0)
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
    
	unregister_chrdev(201, DEVICE_NAME);
}



module_init(timer_init);
module_exit(timer_cleanup);

/* Sources
 * https://wintergreenworks.wordpress.com/2019/02/20/using-hr-timer-in-linux-kernel/
 * https://linux.die.net/lkmpg/x569.html
 * https://github.com/raspberrypi/linux/blob/rpi-4.4.y/drivers/rtc/rtc-ds1307.c
 */
