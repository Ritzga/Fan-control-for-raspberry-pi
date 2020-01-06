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
	{ 0x9f8384f0, "kthread_stop" },
	{ 0xdb7305a1, "__stack_chk_fail" },
	{ 0x294d1ccc, "wake_up_process" },
	{ 0xc8460f5d, "sched_setscheduler" },
	{ 0xe83b4eca, "kthread_bind" },
	{ 0x938e50ff, "kthread_create_on_node" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x952664c5, "do_exit" },
	{ 0xf9a482f9, "msleep" },
	{ 0x7c32d0f0, "printk" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x6df1aaf1, "kernel_sigaction" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

