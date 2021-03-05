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
