#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include <linux/kernel.h>           //used for do_exit()
#include <linux/threads.h>          //used for allow_signal
#include <linux/kthread.h>          //used for kthread_createa
#include <linux/sched/signal.h>
#include <linux/sched/types.h>

MODULE_LICENSE("GPL");
 
#define WORKER_THREAD_DELAY 4
#define DEFAULT_THREAD_DELAY 6
 
#define pwm1		18 // Kernel PWM channel 0
 
/* Global PWM parameters */
int duty      = 50;			// Dutycycle in %
int frequency = 25000;		// Frequency in Hz...
int enable    = 1;			// 0 = disable, 1 = enable 

volatile int sem = 0;

/* Global Threading variables*/
static struct task_struct *worker_task;
static int get_current_cpu;
 

// Variables that can be changed from userspace
// by writing in the corresponding files in
// sys/module/<modname>/parameters/<parameter name>
module_param(duty, int, 0644);
module_param(frequency, int, 0644);
module_param(enable, int, 0644);
 
/*--------------------------------------------------*/
/*          Configure the GPIO port(s)              */
/*
*@brief: Function that initialises a GPIO pin for use as PWM
*/
int __init pwm_gpio_init(void)
{
	//initialises defined gpio pin as output for use as soft-pwm
	printk(KERN_INFO "PWM: starting %s. \n", __FUNCTION__);
	gpio_request(pwm1, "pwm1");
	gpio_direction_output(pwm1, 0);
	
	return 0;
}

/*
*@brief: Function that frees the pin used for the pwm
*/
void __exit pwm_gpio_exit(void)
{
	// frees the gpio pin again
	printk(KERN_INFO "PWM: stopping %s.\n", __FUNCTION__);
	gpio_free(pwm1);
}
/*--------------------------------------------------*/
/*           Run PWM on the GPIO port               */
 
/*
*@brief: Function for the worker thread to implement the Soft-PWM
*/
 static int worker_task_handler_fn(void *arguments)
{
	 // This is the function being threaded by the module
	 // It handles the Soft-PWM by setting the GPIO pin
	 // to HIGH, sleeping for the (1/frequency/100)*duty
	 // then setting it to LOW and sleeping for
	 // (1/frequency/100)*(100-duty)
	int tusec_On;
	int tusec_Off;
	
	//sem=1;
	
	 //Allows this thread to be killed by the parent thread
	allow_signal(SIGKILL);
	
	//As long as the thread isnt killed from the outside do...
	while(!kthread_should_stop()){
		/* Calculate from frequency and dutycycle the delay-times */
		
		tusec_On  = (1000000*duty)/(frequency*100);			// Duration of on-cycle
		tusec_Off = (1000000*(100-duty))/(frequency*100);	// Duration of off-cycle
		printk("duty:%d, tusec_on:%d, tusec_off:%d, freq:%d", duty, tusec_On, tusec_Off, frequency);
		//se thigh and sleep
		gpio_set_value(pwm1, 1);
		usleep_range(tusec_On, tusec_On);
		//set low and sleep
		gpio_set_value(pwm1, 0);
		usleep_range(tusec_Off, tusec_Off);
		
		//End thread if theres an unhandled signal pending
		if (signal_pending(worker_task))
			            break;
	}
	
	// Function provided in linux/kernel to properly end a process in kernel mode
	do_exit(0);

	pr_alert("Worker task exiting\n");
	return 0;
}

/*
*brief: Function to start the Soft-PWM via a separate thread on the specified pin
*@return: 0, additional information in dmesg
*/
int pwn_run_init(void)
{
	//For proper results the Soft-PWM must be prioritize
	// otherwise in a high-workload-situation the htread
	// for the soft-PWM might be ignored and the fan stops
	// causing temperatures to rise and thermal throtteling
	// which in turn reducec performance, making everything worse
	struct sched_param task_sched_params =
	{
			.sched_priority = MAX_RT_PRIO
	};
	
	printk(KERN_ALERT "STARTING PWM: Frequency is %dMHz, and dutycycle is %d percent.\n", frequency, duty);
	
	//Start thread with some debug information about which CPU core handles the workload

	task_sched_params.sched_priority = 90;
	printk("Creating PWM thread\n");
	
	get_current_cpu = get_cpu();
	worker_task = kthread_create(worker_task_handler_fn,
			(void*)"arguments as char pointer","P");
	kthread_bind(worker_task,get_current_cpu);
	
	if(worker_task)
		printk("Worker task created successfully\n");
	else
		printk("Worker task error while creating\n");
	wake_up_process(worker_task);
	
	return 0;
}
 
/*
*brief: Function that terminates the PWM thread
*/
void pwm_run_exit(void)
{
	printk(KERN_ALERT "STOPPING PWM in %s.\n", __FUNCTION__);
	//End thread
	
	if(worker_task)
		kthread_stop(worker_task); 
	//while(sem);
			
}
 
/*--------------------------------------------------*/
/*         The Module starting function             */
 
int __init pwm_init(void)
{
	printk(KERN_INFO " Starte the %s function.\n", __FUNCTION__);
	/* Configure initial state of the PWM pins and start it*/
	pwm_gpio_init();
	pwn_run_init();
 
	return 0;
}
 
void __exit pwm_exit(void)  
{
	printk(KERN_INFO " Ending the function %s. \n", __FUNCTION__);
	pwm_run_exit();
	pwm_gpio_exit();
	usleep_range(60000, 60000);
	
}
 
module_init(pwm_init);
module_exit(pwm_exit);
