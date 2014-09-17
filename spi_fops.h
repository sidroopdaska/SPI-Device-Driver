#ifndef SPI_FOPS_H
#define SPI_FOPS_H

#include <linux/fs.h>

long spimod_ioctl(struct file* file, unsigned int ioctl_num, unsigned long ioctl_param);

ssize_t spimod_read(struct file* file, char __user* buf, size_t count, loff_t* offp);

ssize_t spimod_write(struct file* file, const char __user* buf, size_t count, loff_t* offp);

int spimod_open(struct inode* i, struct file* file);

int spimod_close(struct inode* i, struct file* file);

#endif
