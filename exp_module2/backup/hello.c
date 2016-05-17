#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/current.h>
#include <linux/sched.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

#if 1

#define MEMDEV_MAJOR	260
#define MEMDEV_MINOR	0
#define MEMDEV_NR_DEVS	2
#define	MEMDEV_SIZE	4096

#define MEM_DEV_DEBUG

#ifdef MEM_DEV_DEBUG
#define printf printk
#else
define printf do{}while(0)
#endif

struct stu_mem_dev
{
	char *data;
	unsigned long size;
};

static int mem_major = MEMDEV_MAJOR;
static int mem_minor = MEMDEV_MINOR;
static int mem_nr_devs = MEMDEV_NR_DEVS;
static char *mem_name = "memdev";

module_param(mem_major,int,S_IRUGO);
module_param(mem_minor,int,S_IRUGO);
module_param(mem_nr_devs,int,S_IRUGO);
module_param(mem_name,charp,S_IRUGO);

struct stu_mem_dev *mem_devp;
struct cdev mem_cdev;

int mem_open(struct inode *inode,struct file *filp)
{
	struct stu_mem_dev *dev;
	int num = MINOR(inode->i_rdev);
	printk(KERN_ALERT "num is %d\n",num);
	if(num >= MEMDEV_NR_DEVS)
		return -ENODEV;
	dev = &mem_devp[num];
	filp->private_data = dev;
	printk(KERN_ALERT "mem open ok\n");
	return 0;
}

int mem_release(struct inode *inode,struct file *filp)
{
	printk(KERN_ALERT "mem release ok\n");
	return 0;
}

static ssize_t mem_read(struct file *filp,char *buf,size_t size,loff_t *ppos)
{
	unsigned long pos = *ppos;
	unsigned long count = size;
	int ret = 0;
	struct stu_mem_dev *dev = filp->private_data;

	if(pos >= MEMDEV_SIZE)
		return 0;
	if(count > MEMDEV_SIZE-pos)
		count = MEMDEV_SIZE - pos;
	if(copy_to_user(buf,(void*)(dev->data + pos),count)) {
		ret = -EFAULT;
	} else {
		*ppos = *ppos + count;
		ret =(int) count;
		printk(KERN_ALERT "read %d bytes from %d\n",(int)count,(int)pos);
	}
	return ret;
}

static ssize_t mem_write(struct file *filp,const char *buf,size_t size, loff_t *ppos)
{
	unsigned long pos = *ppos;
	unsigned long count = size;
	int ret = 0;
	struct stu_mem_dev *dev = filp->private_data;

	if(pos > MEMDEV_SIZE)
		return 0;
	if(count > MEMDEV_SIZE - pos)
		count = MEMDEV_SIZE - pos;
	if(copy_from_user(((void *)(dev->data + pos)),buf,count)) {
		ret = -EFAULT;
	} else {
		*ppos = *ppos + count;
		ret = (int)count;
		printk(KERN_ALERT "write %d bytes to %d\n",(int)count,(int)pos);
	}
	return ret;
}

static loff_t mem_llseek(struct file *filp,loff_t offset,int whence)
{
	//printk(KERN_ALERT "mem llseek ok\n");
	loff_t newpos;

	switch(whence) {
	case 0:
		newpos = offset;
		break;
	case 1:
		newpos = filp->f_pos + offset;
		break;
	case 2:
		newpos = MEMDEV_SIZE - 1 + offset;
		break;
	default:
		return -EINVAL;
	}
	if((newpos < 0)||(newpos > MEMDEV_SIZE))
		return -EINVAL;

	filp->f_pos = newpos;
	return newpos;
}

struct file_operations mem_fops = 
{
	.owner = THIS_MODULE,
	.llseek = mem_llseek,
	.read = mem_read,
	.write = mem_write,
	.open = mem_open,
	.release = mem_release,
};

static int memdev_init(void)
{
	int r;
	int i;
	dev_t devno;

	printk(KERN_ALERT "mem_major:%d,mem_minor:%d,mem_nr_devs:%d,mem_name:%s\n",
				mem_major,mem_minor,mem_nr_devs,mem_name);

	devno = MKDEV(mem_major,mem_minor);
	if(mem_major) {
		r = register_chrdev_region(devno,mem_nr_devs,mem_name);
	} else {
		r = alloc_chrdev_region(&devno,mem_minor,mem_nr_devs,mem_name);
		mem_major = MAJOR(devno);
	}
	if(r < 0) {
		printk(KERN_ALERT "get device number failed,ret is %d\n",r);
		return r;
	}
	printk(KERN_ALERT "get device number ok,mem_major is %d\n",mem_major);

	cdev_init(&mem_cdev,&mem_fops);
	mem_cdev.owner = THIS_MODULE;
	mem_cdev.ops = &mem_fops;	

	r = cdev_add(&mem_cdev,MKDEV(mem_major,0),MEMDEV_NR_DEVS);
	if(r) {
		printk(KERN_ALERT "add cdev failed,ret is %d\n",r);
		goto fail_malloc;
	}

	mem_devp = kmalloc(MEMDEV_NR_DEVS*sizeof(struct stu_mem_dev),GFP_KERNEL);
	if(mem_devp == NULL) {
		r = -ENOMEM;
		goto fail_malloc;
	}
	memset(mem_devp,0,sizeof(struct stu_mem_dev));
	printk(KERN_ALERT "malloc mem_devp memory ok\n");

	for(i = 0;i < mem_nr_devs;i++) {
		printk(KERN_ALERT "malloc mem_devp[%d] data memory\n",i);
		mem_devp[i].size = MEMDEV_SIZE;
		mem_devp[i].data = kmalloc(MEMDEV_SIZE,GFP_KERNEL);
		memset(mem_devp[i].data,0,MEMDEV_SIZE);
	}
	return 0;
fail_malloc:
	unregister_chrdev_region(devno,1);
	return r;
}

static void memdev_exit(void)
{
	int i = 0;
	cdev_del(&mem_cdev);
	printk(KERN_ALERT "cdev delete ok\n");
	for(i = 0;i < mem_nr_devs;i++){
	//	kfree(mem_devp[i].data);
		printk(KERN_ALERT "free mem_devp[%d] data ok\n",i);
	}
	kfree(mem_devp);
	printk(KERN_ALERT "free mem_devp ok\n");
	unregister_chrdev_region(MKDEV(mem_major,mem_minor),mem_nr_devs);
	printk(KERN_ALERT "unregister device numebr ok\n");
	printk(KERN_ALERT "mem device exit ok\n");
}

module_init(memdev_init);
module_exit(memdev_exit);
#endif


#if 0
static char *name = "tariq zhang";
static int age = 28;

module_param(name,charp,S_IRUGO);
module_param(age,int,S_IRUGO);

static int __init hello_init(void)
{
	printk(KERN_ALERT "hello world,this is a new module\n");
	printk(KERN_ALERT "name is %s,age is %d\n",name,age);
	printk(KERN_ALERT "current process is %s,pid is %d\n",
		current->comm,current->pid);
	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_ALERT "exit hello module\n");
	printk(KERN_ALERT "current process is %s,pid is %d\n",
		current->comm,current->pid);
}

module_init(hello_init);
module_exit(hello_exit);

#endif

MODULE_AUTHOR("ZHANG DAQING");
MODULE_VERSION("1000");
MODULE_DESCRIPTION("HELLO WORLD MODULE");

