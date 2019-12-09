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
#define DEVICE_NAME "testi2c"	/* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80		/* Max length of the message from the device */

static struct hrtimer htimer;
static ktime_t kt_periode;
static void timer_cleanup(void);
static enum hrtimer_restart timer_function(struct hrtimer * timer);
static int driver_open(struct inode*geraete_datei, struct file *instanz );
static int driver_close(struct inode*geraete_datei, struct file *instanz );
static ssize_t driver_read(struct file *filp, char *buffer,  size_t length, loff_t * offset);
static ssize_t driver_write(struct file *Instanz,const char *User, size_t Count, loff_t *offs);
static void SetTemperature(void);

static char msg[BUF_LEN];	/* The msg the device will give when asked */
static char *msg_Ptr;
static int Device_Open = 0;
static struct i2c_client* client;
static struct i2c_adapter* adptr;



static int temperature = 1234;

static struct file_operations fops = {
   read: driver_read,
   write: driver_write,
   open: driver_open,
   release: driver_close
};


static struct i2c_board_info bme280_i2c_board_info = {
	I2C_BOARD_INFO("bme280", 0x76)
};

//=======================================================================
//i2c Functionality

static s32 bme280_read_block_data_once(const struct i2c_client *client, u8 command, u8 length, u8 *values)
{
	if(!client)
	{
	    pr_alert("[bme280_read_block_data_once]: Client was null!");
	    return -1;
	}
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
//=======================================================================
//File Interface for userspace


static int driver_open(struct inode*geraete_datei, struct file *instanz )
{
	static int counter = 0;

	if (Device_Open)
		return -EBUSY;

	Device_Open++;
	
	int temperature_high = temperature/100;
	int temperature_low = temperature%100;
	sprintf(msg, "%d.%d\n", temperature_high, temperature_low);
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
    SetTemperature();
    /*
    int ret = snprintf(msg, sizeof msg, "%f", temperature);

    if (ret < 0) {
        snprintf(msg, sizeof msg, "Coundn't convert float to char[].");
    } 
    if (ret >= sizeof msg) {
        snprintf(msg, sizeof msg, "Float value truncated");
    }*/
    
    hrtimer_forward_now(timer, kt_periode);

    return HRTIMER_RESTART;
}

static void SetTemperature(){
    u8 temps[3];
    //bme280_read_block_data(client, 0xFA, 3, temps);
    /*
    u8 bT1[2], bT2[2], bT3[2];
    bme280_read_block_data(client, 0x88, 2, bT1);
    bme280_read_block_data(client, 0x8A, 2, bT2);
    bme280_read_block_data(client, 0x8C, 2, bT3);
    
    long temp_raw=(temps[0]<<16)+(temps[1]<<8)+temps[2];
    long T1 = (bT1[0]<<8)+bT1[1];
    long T2 = (bT2[0]<<8)+bT2[1];
    long T3= (bT3[0]<<8)+bT3[1];
    
    int adc_T = temp_raw >> 4;
    
    int32_t var1  = ((((adc_T>>3) - ((int32_t)T1 <<1))) *
		((int32_t)T2)) >> 11;
    int32_t var2  = (((((adc_T>>4) - ((int32_t)T1)) *
       ((adc_T>>4) - ((int32_t)T1))) >> 12) *
	((int32_t)T3)) >> 14;
	
    int T  = ((var1 + var2) * 5 + 128) >> 8;
    temperature = T/100;*/
    temperature++;
}

//=======================================================================
//i2c BME280 functions
static int bme280_detect(struct i2c_client *client, int kind,
			  struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE
				     | I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
		return -ENODEV;

	/* Now, we would do the remaining detection. But the PCF8591 is plainly
	   impossible to detect! Stupid chip. */

	strlcpy(info->type, "bme280", I2C_NAME_SIZE);

	return 0;
}

static int pcf8591_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct pcf8591_data *data;
	int err;

	if (!(data = kzalloc(sizeof(struct pcf8591_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);

	/* Initialize the PCF8591 chip */
	pcf8591_init_client(client);

	/* Register sysfs hooks */
	err = sysfs_create_group(&client->dev.kobj, &pcf8591_attr_group);
	if (err)
		goto exit_kfree;

	/* Register input2 if not in "two differential inputs" mode */
	if (input_mode != 3) {
		if ((err = device_create_file(&client->dev,
					      &dev_attr_in2_input)))
			goto exit_sysfs_remove;
	}

	/* Register input3 only in "four single ended inputs" mode */
	if (input_mode == 0) {
		if ((err = device_create_file(&client->dev,
					      &dev_attr_in3_input)))
			goto exit_sysfs_remove;
	}

	return 0;

exit_sysfs_remove:
	sysfs_remove_group(&client->dev.kobj, &pcf8591_attr_group_opt);
	sysfs_remove_group(&client->dev.kobj, &pcf8591_attr_group);
exit_kfree:
	kfree(data);
exit:
	return err;
}

static int pcf8591_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &pcf8591_attr_group_opt);
	sysfs_remove_group(&client->dev.kobj, &pcf8591_attr_group);
	kfree(i2c_get_clientdata(client));
	return 0;
}
//=======================================================================
static const struct i2c_device_id bme280_id[] = {
	{ "bme280", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, bme280_id);

static struct i2c_driver bme280_driver = {
	.driver = {
		.name	= "bme280",
	},
	.probe		= bme280_probe,
	.remove		= bme280_remove,
	.id_table	= bme280_id,

	.class		= I2C_CLASS_HWMON,	/* Nearest choice */
	.detect		= bme280_detect,
	//.address_data	= &addr_data,
};

static int timer_init(void)
{    
    /*
    adptr = i2c_get_adapter(1);
    client = i2c_new_device(adptr, &bme280_i2c_board_info);
	*/
	
    //Set Timer
    if(register_chrdev(201, DEVICE_NAME, &fops) == 0)
    {
        kt_periode = ktime_set(0, 500000000); //seconds,nanoseconds
        hrtimer_init (& htimer, CLOCK_REALTIME, HRTIMER_MODE_REL);
        htimer.function = timer_function;
        hrtimer_start(& htimer, kt_periode, HRTIMER_MODE_REL);
    
	return i2c_add_driver(&bme280_driver);
    }
    else
    {
        return -EIO;
    }    
    
    
    
}

static void timer_cleanup(void)
{
    
    i2c_del_driver(&bme280_driver);
    hrtimer_cancel(& htimer);
    
    unregister_chrdev(201, DEVICE_NAME);
}



module_init(timer_init);
module_exit(timer_cleanup);

/* Sources
 * https://wintergreenworks.wordpress.com/2019/02/20/using-hr-timer-in-linux-kernel/
 * https://linux.die.net/lkmpg/x569.html
 * https://github.com/raspberrypi/linux/blob/rpi-4.4.y/drivers/rtc/rtc-ds1307.c
 * https://github.com/spotify/linux/blob/master/drivers/hwmon/pcf8591.c
 */
