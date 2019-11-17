#include <linux/mm.h>
#include <linux/mman.h>
#include <asm/unistd.h>
#include <asm/cacheflush.h>
#include <asm/pgtable_types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/highmem.h>
#include <linux/fs.h>
#include <linux/fdtable.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <linux/slab.h>

// Environment: Virtual Box with Ubuntu 18.04 (64 bit), kernel version 5.0.0-25-generic.
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nguyen Phuong Vy");
MODULE_DESCRIPTION("Hook open and write syscall");

// raw access to syscall table
unsigned long **syscall_table;

asmlinkage int(*old_open)(const char __user *, int, mode_t);
asmlinkage ssize_t(*old_write)(unsigned int, const void __user *, size_t);

asmlinkage int new_open(const char __user *pathname, int flags, mode_t mode)
{
	printk(KERN_INFO "HOOK open syscall\n");
	char *name = kmalloc(1024, GFP_KERNEL);
	copy_from_user(name, pathname, 1024);

	printk(KERN_INFO "Process %s is opening file: %s\n", current->comm, name);
	
	kfree(name);
	return (*old_open)(pathname, flags, mode);
}

asmlinkage ssize_t new_write(unsigned int fd, const void __user *buf, size_t count)
{
	/* Retrieve pathname from file descriptor using file descriptor table
	Reference: https://code.woboq.org/linux/linux/include/linux/fdtable.h.html#fdtable */

	char *buffer = kmalloc(1024, GFP_KERNEL);
	char *kfname = d_path(&fcheck_files(current->files, fd)->f_path, buffer, 1024);
	printk(KERN_INFO "Process %s writes %zu bytes to %s", current->comm, count, kfname);
	kfree(buffer);
	return(*old_write)(fd, buf, count);;

}

/* Make page writeable */
int make_rw(unsigned long address) 
{
	unsigned int level;
	pte_t *pte = lookup_address(address, &level);
	if (pte->pte &~_PAGE_RW)
	{
		pte->pte |= _PAGE_RW;
	}
	return 0;
}

/* Make page write protected */
int make_ro(unsigned long address)
 {
	unsigned int level;
	pte_t *pte = lookup_address(address, &level);
	pte->pte = pte->pte &~_PAGE_RW;
	return 0;
}

static void get_syscall_table(void)
{
	unsigned long int offset = PAGE_OFFSET;

	while (offset < ULLONG_MAX)
	{
		unsigned long **temp_syscall_table = (unsigned long **)offset;
		// found system syscall table
		if (temp_syscall_table[__NR_close] == (unsigned long *)ksys_close)
		{
			syscall_table = temp_syscall_table;
			return;
		}

		offset += sizeof(void *);
	}
	syscall_table = NULL;
}

static int init_hook(void)
{
	printk(KERN_INFO "Loaded hook successfully...\n")

	get_syscall_table();
	if (syscall_table == NULL) 
	{
		printk(KERN_ERR "HOOK not found syscall table\n");
		return -1;
	}

	// backup syscall
	/*old_open = (void *)syscall_table[__NR_open];
	old_write = (void *)syscall_table[__NR_write];*/
	old_open = syscall_table[__NR_open];
	old_write = syscall_table[__NR_write];

	/* Disable page protection */
	make_rw((unsigned long)syscall_table);

	// override syscall
	syscall_table[__NR_open] = (unsigned long *)new_open;
	syscall_table[__NR_write] = (unsigned long *)new_write;

	/* Renable page protection */
	make_ro((unsigned long)syscall_table);
	return 0;
}

static void exit_hook(void)
{
	printk(KERN_INFO "HOOK module exit\n");

	// tim duoc syscall table
	if (syscall_table != NULL)
	{
		/* Disable page protection */
		make_rw((unsigned long)syscall_table);

		// restore syscall
		syscall_table[__NR_open] = (unsigned long *)old_open;
		syscall_table[__NR_write] = (unsigned long *)old_write;

		/* Renable page protection */
		make_ro((unsigned long)syscall_table);
	}
}

module_init(init_hook);
module_exit(exit_hook);
