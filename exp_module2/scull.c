#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>

#include <linux/wait.h>

#define CONFIG_SCULL_DEBUG

enum SCULL_DEVICE_TYPE
{
	SCULL_N	= (0),
	SCULL_R = (1),
	SCULL_W = (2),
	SCULL_WR = (3),
};

struct scull_data
{
	char *label;
	char *data;
	unsigned long size;
};
struct scull_device
{
	spinlock_t		lock;
	struct cdev		dev;
	struct scull_data 	*scu;
	struct file_operations 	fops;
	int type;

//	int flag;
//	wait_queue_head_t wqueue;
};

#define SCULL_MAJOR	260
#define SCULL_DEV_NR	2
#define SCULL_DEV_DATA_MAX_SIZE	2048

static int scull_open(struct inode *node,struct file *filp)
{
	struct scull_device *pdev;
	unsigned long flags;

	pdev = container_of(node->i_cdev,struct scull_device,dev);
	printk(KERN_ALERT "scull open, pdev = %x\n",(unsigned int)(pdev));

//	spin_lock_irqsave(&(pdev->lock),flags);
	filp->private_data = pdev;
//	spin_unlock_irqrestore(&(pdev->lock),flags);
	return 0;
}

static int scull_release(struct inode *node, struct file *filp)
{
	filp->private_data = 0;
	printk(KERN_ALERT "scull release\n");
	return 0;
}

static ssize_t scull_read(struct file *filp, char __user *buf,
				size_t len,loff_t *offs)
{
	struct scull_device *pdev = filp->private_data;
	struct scull_data *ptr_scu;
	unsigned long flags,pos;
	unsigned long count;

//	spin_lock_irqsave(&(pdev->lock),flags);

	ptr_scu = pdev->scu;
	pos = *offs;
	count = len;

	if(pos >= ptr_scu->size)
		return 0;//count is 0
	if(count > ptr_scu->size - pos)
		count = ptr_scu->size - pos;

	if(copy_to_user(buf, ptr_scu->data + pos, count))
		return -EFAULT;
	else
		*offs = pos + count;

//	spin_unlock_irqrestore(&(pdev->lock),flags);
	printk(KERN_ALERT "%s read %d bytes to %x from device,offs = %d\n",
			ptr_scu->label,
			(int)count,
			(unsigned int)(buf),
			(int)(*offs));
	return count;
}

static ssize_t scull_write(struct file *filp, const char __user *data,
				size_t len,loff_t *offs)
{
	struct scull_data *ptr_scu;
	struct scull_device *pdev;
	unsigned long flags,pos,count;

//	spin_lock_irqsave(&(pdev->lock),flags);

	pdev = filp->private_data;
	ptr_scu = pdev->scu;
	pos = *offs;
	count = len;

	if(pos >= ptr_scu->size)
		return 0;
	if(count > ptr_scu->size - pos)
		count = ptr_scu->size - pos;

	if(copy_from_user(ptr_scu->data + pos, data, count))
		return -EFAULT;
	else
		*offs = pos + count;

//	spin_unlock_irqrestore(&(pdev->lock),flags);
	printk(KERN_ALERT "%s write %d bytes to device from %x,offs = %d\n",
			ptr_scu->label,
			(int)count,
			(unsigned int)(data),
			(int)(*offs));
	return count;
}
#if 0
static ssize_t sleepy_read(struct file *filp, char __user *buf,
				size_t count,loff_t *pos)
{
	struct scull_device *ptr_scull = filp->private_data;

	printk(KERN_ALERT "sleepy_read, wait event,suspend task\n");
	wait_event_interruptible(ptr_scull->wqueue,ptr_scull->flag != 0);
	ptr_scull->flag = 0;
	printk(KERN_ALERT "sleepy_read, evnet come,resume task\n");
	return 0;
}

static ssize_t sleepy_write(struct file *filp, char __user *data,
				size_t count,loff_t *pos)
{
	struct scull_devie *ptr_scull = filp->private_data;

	printk(KERN_ALERT "sleepy write, wake up task\n");
	scull_device->flag = 1;
	wake_up_interruptible(&ptr_scull->wqueue);
	return count;
}
#endif
enum SCULL_DEV_LLSEEK_CMD
{
	LLS_SET_POS = 0,
	LLS_CUR_POS,
	LLS_END_POS,
};

static loff_t scull_llseek(struct file *filp, loff_t offs, int cmd)
{
	loff_t newpos;

	switch(cmd) {
	case LLS_SET_POS:
		newpos = offs;
		break;
	case LLS_CUR_POS:
		newpos = filp->f_pos + offs;
		break;
	case LLS_END_POS:
		newpos = SCULL_DEV_DATA_MAX_SIZE - 1 + offs;
		break;
	default:
		return -EINVAL;
	}
	if((newpos < 0) || (newpos > SCULL_DEV_DATA_MAX_SIZE))
		return -EINVAL;

	filp->f_pos = newpos;

	printk(KERN_ALERT "scull llseek f_pos = %d\n",(int)newpos);
	return newpos;
}

static char *scull_name_tab[SCULL_DEV_NR] = {"scull0", "scull1"};
struct scull_device scull_dev[SCULL_DEV_NR] = {
	{
		.fops = {
			.owner  = THIS_MODULE,
			.open 	= scull_open,
			.read	= scull_read,
			.write	= scull_write,
			.llseek	= scull_llseek,
			.release = scull_release,
		},
		.type = SCULL_WR,
	},
	{
		.fops = {
			.owner  = THIS_MODULE,
			.open 	= scull_open,
			.read	= scull_read,
			.write	= scull_write,
			.llseek	= scull_llseek,
			.release = scull_release,
		},
		.type = SCULL_WR,
	},
};

static int _scull_dev_setup_cdev(struct scull_device *scull, int index)
{
	int err, devno = MKDEV(SCULL_MAJOR,index);

	cdev_init(&scull->dev,&scull->fops);
	scull->dev.owner = THIS_MODULE;
	scull->dev.ops = &scull->fops;
	err = cdev_add(&scull->dev,devno,1);
	if(err) {
		goto SETUP_FAIL;
	}

//	init_waitqueue_head(&scull->wqueue);
//	scull->flag = 0;

	printk(KERN_ALERT "scull %d setup cdev done\n",index);
	return 0;
SETUP_FAIL:
	printk(KERN_ALERT "scull %d setup cdev failed,err is %d\n"
		,index,err);
	cdev_del(&scull->dev);
	return -1;
}

static void _scull_dev_free_cdev(struct scull_device *scull,int index)
{
	cdev_del(&scull->dev);
	printk(KERN_ALERT "del scull %d cdev done\n",index);
}

static int _scull_dev_setup_data(struct scull_device *scull,int index)
{
	struct scull_data *mem_scull;
	mem_scull = kmalloc(sizeof(struct scull_data),GFP_KERNEL);
	if(mem_scull == NULL) {
		return -1;
	};
	mem_scull->size = SCULL_DEV_DATA_MAX_SIZE;
	mem_scull->label = scull_name_tab[index];
	mem_scull->data = kmalloc(mem_scull->size,GFP_KERNEL);
	if(mem_scull->data == NULL) {
		return -1;
	};
	memset(mem_scull->data,0,mem_scull->size);
	scull->scu = mem_scull;

	printk(KERN_ALERT "%s setup data = %x, size = %d\n",
		scull->scu->label,
		(unsigned int)(mem_scull->data),
		(int)(mem_scull->size));
	return 0;
}

static int _scull_dev_free_data(struct scull_device *scull, int index)
{
	struct scull_data *mem_scull = scull->scu;
	kfree(mem_scull->data);
	kfree(mem_scull);
	printk(KERN_ALERT "free scull %d data done\n",index);
	return 0;
}

static int __init scull_dev_init(void)
{
	int i = 0;
	for(i = 0;i < SCULL_DEV_NR;i++) {
		spin_lock_init(&(scull_dev[i].lock));
		if(_scull_dev_setup_data(&scull_dev[i],i))
			goto INIT_FAIL_2;
		if(_scull_dev_setup_cdev(&scull_dev[i],i))
			goto INIT_FAIL_1;
	}
	printk(KERN_ALERT "init scull dev done\n");
	return 0;
INIT_FAIL_1:
	_scull_dev_free_data(&scull_dev[i],i);
	return -2;
INIT_FAIL_2:
	return -1;
}

static void __exit scull_dev_exit(void)
{
	int i;
	for(i = 0;i < SCULL_DEV_NR;i++) {
		_scull_dev_free_cdev(&scull_dev[i],i);
		_scull_dev_free_data(&scull_dev[i],i);
	}
	printk(KERN_ALERT "exit scull dev done\n");
}

module_init(scull_dev_init);
module_exit(scull_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TariqZhang");
MODULE_VERSION("V1.0");
MODULE_DESCRIPTION("SCULL device module");
