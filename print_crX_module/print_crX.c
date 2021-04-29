#include <linux/module.h>
#include <linux/kernel.h>

static void print_crX(void) {

#ifdef __x86_64__
	u64 cr0, cr2, cr3;
	__asm__ __volatile__ (
		"movq %%cr0, %%rax \n\t"		
		"movq %%rax, %0 \n\t"
		"movq %%cr0, %%rax \n\t"
		"movq %%rax, %1 \n\t"
		"movq %%cr3, %%rax \n\t"
		"movq %%rax, %2 \n\t"
	: "=m" (cr0), "=m" (cr2), "=m" (cr3)
	:
	: "%rax"
	);
#elif defined(__i386__)
	u32 cr0, cr2, cr3;
	__asm__ volatile__ (
		"mov %%cr0, %%eax \n\t"
		"mov %%eax, %0 \n\t"
		"mov %%cr2, %%eax \n\t"
		"mov %%eax, %1 \n\t"
		"mov %%cr3, %%eax \n\t"
		"mov %%eax, %2 \n\t"
	: "=m" (cr0), "=m" (cr2), "=m" (cr3)
	:
	: "%eax"
	);
#endif

	printk(KERN_INFO "CR0 = %X\n", cr0);
	printk(KERN_INFO "CR2 = %X\n", cr2);
	printk(KERN_INFO "CR3 = %X\n", cr3);

	return;
}

static int __init test_init_module(void) {

	print_crX();

	return 0;
}
module_init(test_init_module);

static void __exit test_cleanup_module(void) {
	print_crX();
}
module_exit(test_cleanup_module);

MODULE_LICENSE("GPL");
