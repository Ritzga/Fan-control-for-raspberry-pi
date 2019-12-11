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
 
module_param(duty, int, 0644);
module_param(frequency, int, 0644);
module_param(enable, int, 0644);
 
/*--------------------------------------------------*/
/*          Configure the GPIO port(s)              */
 
int __init pwm_gpio_init(void)
{
	printk(KERN_INFO "PWM: starting %s. \n", __FUNCTION__);
	gpio_request(pwm1, "pwm1");
	gpio_direction_output(pwm1, 0);
	
	return 0;
}
 
void __exit pwm_gpio_exit(void)
{
	printk(KERN_INFO "PWM: stopping %s.\n", __FUNCTION__);
	gpio_free(pwm1);
}
/*--------------------------------------------------*/
/*           Run PWM on the GPIO port               */
 
 static int worker_task_handler_fn(void *arguments)
{
	int tusec_On;
	int tusec_Off;
	
	//sem=1;
	
	allow_signal(SIGKILL);
	
	while(!kthread_should_stop()){
		/* Calculate from frequency and dutycycle the delay-times */
		
		tusec_On  = (1000000*duty)/(frequency*100);			// Duration of on-cycle
		tusec_Off = (1000000*(100-duty))/(frequency*100);	// Duration of off-cycle
		printk("duty:%d, tusec_on:%d, tusec_off:%d, freq:%d", duty, tusec_On, tusec_Off, frequency);
		gpio_set_value(pwm1, 1);
		usleep_range(tusec_On, tusec_On);
		gpio_set_value(pwm1, 0);
		usleep_range(tusec_Off, tusec_Off);

		if (signal_pending(worker_task))
			            break;
	}
	//sem=0;
	do_exit(0);

	pr_alert("Worker task exiting\n");
	return 0;
}

int pwn_run_init(void)
{
	struct sched_param task_sched_params =
	{
			.sched_priority = MAX_RT_PRIO
	};
	
	printk(KERN_ALERT "STARTING PWM: Frequency is %dMHz, and dutycycle is %d percent.\n", frequency, duty);
	
	//Start thread

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
	/* Configure initial state of the PWM pins */
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
