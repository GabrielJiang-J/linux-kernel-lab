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

### windows 10 + vitualbox双机 + gdb调试kernel
1. 创建上位机，选择COM1，启用串口，不要勾选“连接至现有通道或套接字”模式选择host pipe，路径按如下格式填写：\\\\.pipe\PIPENAME。
2. 创建下位机，操作如同1，只是要勾选“连接至现有通道或套接字”。
3. 启动上位机，执行命令`stty -F /dev/ttyS0 speed 115200; cat /dev/ttyS0`，等待接收信息。
4. 启动下位机，执行命令`stty -F /dev/ttyS0 speed 115200; echo hello /dev/ttyS0`，发送消息。
5. 上位机如果收到消息，说明双机串口连接成功，开始kgdb的调试配置。
6. 在下位机中，将`kgdboc=ttyS0,115200 kgdbwait`加入到grub.conf的linux16内核启动项中。
7. 在下位机中，执行命令`echo g>/proc/sysrq-trigger`。/proc/sysrq-trigger文件是在gdb没有下位机控制权的时候，下位机主动将控制权交给gdb。。
8. 重启下位机，此时下位机会停止等待从串口接收gdb发来的指令。
9. 在上位机中，执行命令`sudo gdb vmlinux`，因为要读写/dev/ttyS0文件，所以需要root权限。
10. 在上位机中，执行gdb命令`target remote /dev/ttyS0`，来连接下位机。
11. 此时，上位机就可以正常通过gdb指令调试下位机内核了。

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
