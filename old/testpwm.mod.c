#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xf230cadf, "module_layout" },
	{ 0xfd958c00, "param_ops_int" },
	{ 0x9f8384f0, "kthread_stop" },
	{ 0x294d1ccc, "wake_up_process" },
	{ 0xe83b4eca, "kthread_bind" },
	{ 0x938e50ff, "kthread_create_on_node" },
	{ 0xfe990052, "gpio_free" },
	{ 0xe2df5c3d, "gpiod_direction_output_raw" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x12a38747, "usleep_range" },
	{ 0x6a8eabbf, "gpiod_set_raw_value" },
	{ 0xe851e37e, "gpio_to_desc" },
	{ 0x7c32d0f0, "printk" },
	{ 0x952664c5, "do_exit" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x6df1aaf1, "kernel_sigaction" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x2196324, "__aeabi_idiv" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "BF156DBB07B6B42A7F2A3F8");
