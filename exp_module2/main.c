#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <linux/fcntl.h>
#include <stdio.h>
#include <string.h>
//#include <semaphore.h>

#if 1
int main()
{
	int fd;
	char wr_buf[] = "0123456789ABCDEF";
	char rd_buf[512];
	char ch = 'x';

	memset(rd_buf,0,sizeof(rd_buf));

	if((fd = open("/dev/scull1",O_RDWR)) == -1) {
		printf("open device failed,exit\n");
		return -1;
	}
	printf("open device ok,fd is %d\n",fd);

	printf("write 2 bytes\n");
	write(fd,&ch,1);
	write(fd,&ch,1);

	write(fd,wr_buf,sizeof(wr_buf));
	printf("data is %s\n",wr_buf);

	printf("write %d bytes to device...\n",(int)sizeof(wr_buf));
	write(fd,wr_buf,sizeof(wr_buf));
	printf("data is %s\n",wr_buf);

	/*
	 *	SEEK_SET = 0 seek relative to beginning of file
	 *	SEEK_CUR = 1 seek relative to current file posityion
	 *	SEEK_END = 2 seek relative to end of file
	 */
	printf("seek to beginning of file\n");
	lseek(fd,0,SEEK_SET);

	printf("read %d bytes from device...\n",(int)sizeof(rd_buf));
	read(fd,rd_buf,sizeof(rd_buf));
	printf("data is %s\n",rd_buf);

	printf("seek to offset 2 of file\n");
	lseek(fd,0,SEEK_SET);
	lseek(fd,2,SEEK_CUR);

	printf("read %d bytes from device...\n",(int)sizeof(rd_buf));
	read(fd,rd_buf,sizeof(rd_buf));
	printf("data is %s\n",rd_buf);

	close(fd);
	return 0;
}

#endif

#if 0
//#include <asm/semaphore.h>
#include </usr/src/linux-headers-3.13.0-24/include/linux/semaphore.h>

int main(void)
{
	struct semaphore sem;
	sem_t *sem;
	int r;

	sema_init(&sem,2);

	r = up(&sem);
	printf("r is %d\n",r);

	r = down_interruptible(&sem);
	printf("r is %d\n",r);
	r = down_interruptible(&sem);
	printf("r is %d\n",r);
	r = up(&sem);
	printf("r is %d\n",r);
	r = up(&sem);
	printf("r is %d\n",r);

	r = down_interruptible(&sem);
	printf("r is %d\n",r);
	r = down_interruptible(&sem);
	printf("r is %d\n",r);
	r = down_interruptible(&sem);
	printf("r is %d\n",r);

	return 0;
}
#endif

#if 0
#include <linux/spinlock.h>
int main(void)
{
	
}
#endif
