# linux-project-2
# 1. Linux Kernel Modules
Write a kernel module to generate a random number.
Create a character device driver to pass (specifically, read and write) generated number between Linux user space program and a loadable kernal module (LKM), which is running in Linux kernal space.

## Getting started

### Prerequisites

- Ubuntu OS

- Make

### Installing

Run the following commands:

```
make
```
to compile using linux headers

```
sudo insmod random.ko
```
to install module into kernel

```
lsmod 
```
to check whether module is installed

```
sudo rmmod random
```
to remove module from kernel

# 2. System Call Hooking
- open system call: include process name and name of opened file in dmesg
- write system call: include process name and name of written file in dmesg
Change directory to hook and compile:

cd hook
make
Open a new terminal for observing kernel messages:
```
dmesg -C
dmesg -wH
```
Note: Login as root to perform following operations.

Load new module hook.ko:
```
insmod hook.ko
```
Unload module hook.ko
```
rmmod hook.ko
```
