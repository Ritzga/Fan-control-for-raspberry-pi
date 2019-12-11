#include <linux/init.h>
#include <linux/module.h>
#include <linux/pwm.h>

MODULE_LICENSE("GPL");

#define PIN 18
static struct pwm_device* pwm_dev;

void pwm_init(void)
{

}

void pwm_exit(void)
{

}
static int __init pix_init(void){
  printk(KERN_INFO "PIX: staring...");
  pwm_init();
  printk(KERN_INFO "PIX: staring done.");
  return 0;
}

static void __exit pix_exit(void){
  printk(KERN_INFO "PIX: stopping...");
  pwm_exit();
  printk(KERN_INFO "PIX: stopping done.");
}

module_init(pix_init);
module_exit(pix_exit);
