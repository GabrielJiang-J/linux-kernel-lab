今天尝试安装一个非本机编译的内核rpm包，遇到如下的坑，记录如下：

### 环境
**上位机**: 
虚拟机：6.1.18 r142142
内核版本：3.10.0-1160.15.2.el7.x86_64 CentOS7
GCC版本：gcc version 4.8.5 20150623 (Red Hat 4.8.5-44) (GCC)
待安装内核版本：mainline 3.10

**下位机**:
虚拟机：6.1.18 r142142
内核版本：3.10.0-1160.15.2.el7.x86_64 CentOS7

### 如何制作内核RPM包
在`make menuconfig`之后，使用`make rpm-pkg`，会在rpmbuild/RPMS/x86_64目录下生成headers包和二进制包，
在rpmbuild/SOURCES目录下生成src包。
下位不需要做开发工作，所以不需要src包和headers包，所以只需要二进制包即可。

### 如何安装内核RPM包
在下位机RPM所在目录执行命令：`rpm -ivh XXX.rpm`即可安装新内核。
如果遇到冲突，可以执行命令：`rpm -ivh XXX.rpm --force --nodeps`。

RPM包安装完成以后，可能会需要手动更新grub记录才能启用新的内核。
执行命令：`grub2-mkconfig -o /boot/grub2/grub.cfg`，手动更新grub启动记录。
执行命令：`awk -F \' '$1=="menuentry " {print i++ " : " $2}' /etc/grub2.cfg`，查看当前所有启动项。
执行命令：`grub2-set-default 'xxx'`，将xxx设置为默认启动项。
执行命令：`grub2-editenv list`，查看当前默认启动项。

### 问题
#### 1. 在下位机安装完新内核后，一直卡在“Booting the kernel” (pending!!!)
1.1 刚开始怀疑没有initramfs，导致无法启动。尝试过使用dracut -f 生成initramfs，结果并没有什么卵用。
initramfs在启动过程中的作用还有待加深理解；
1.2 查看源码，定位到应该是卡在decompress_kernel成功解压内核压缩镜像之后，跳转到start_kernel之前的某个位置。
但是，我在start_kernel函数刚开始的位置用pr_notice函数打印信息，却并没有输出出来。

64位中的代码，`arch/x86/boot/compressed/head_64.S`
```asm
	pushq	%rsi			/* Save the real mode argument */
	movq	%rsi, %rdi		/* real mode address */
	leaq	boot_heap(%rip), %rsi	/* malloc area for uncompression */
	leaq	input_data(%rip), %rdx  /* input_data */
	movl	$z_input_len, %ecx	/* input_len */
	movq	%rbp, %r8		/* output target address */
	call	decompress_kernel
	popq	%rsi

	jmp	*%rbp

```

32位中的代码，`arch/x86/boot/compressed/head_32.S`
```c
	leal	z_extract_offset_negative(%ebx), %ebp
				/* push arguments for decompress_kernel: */
	pushl	%ebp		/* output address */
	pushl	$z_input_len	/* input_len */
	leal	input_data(%ebx), %eax
	pushl	%eax		/* input_data */
	leal	boot_heap(%ebx), %eax
	pushl	%eax		/* heap area */
	pushl	%esi		/* real mode pointer */
	call	decompress_kernel
	addl	$20, %esp

#if CONFIG_RELOCATABLE
/*
 * Find the address of the relocations.
 */
	leal	z_output_len(%ebp), %edi

/*
 * Calculate the delta between where vmlinux was compiled to run
 * and where it was actually loaded.
 */
	movl	%ebp, %ebx
	subl	$LOAD_PHYSICAL_ADDR, %ebx
	jz	2f	/* Nothing to be done if loaded at compiled addr. */
/*
 * Process relocations.
 */

1:	subl	$4, %edi
	movl	(%edi), %ecx
	testl	%ecx, %ecx
	jz	2f
	addl	%ebx, -__PAGE_OFFSET(%ebx, %ecx)
	jmp	1b
2:
#endif

/*
 * Jump to the decompressed kernel.
 */
	xorl	%ebx, %ebx
	jmp	*%ebp

```
