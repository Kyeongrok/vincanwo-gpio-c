/*
 *  GPIO interface for Baytrail IT87xx Super I/O chip
 *
 *  Author: ite 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License 2 as published
 *  by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/moduleparam.h>

#include <linux/gpio.h>

#include "sio_gpio.h"

#define BIT0                        0x01
#define BIT1                        0x02
#define BIT2                        0x04
#define BIT3                        0x08
#define BIT4                        0x10
#define BIT5                        0x20
#define BIT6                        0x40
#define BIT7                        0x80

#define IO_INDEX                   0x2E
#define P_INDEX                    IO_INDEX
#define P_DATA                     (P_INDEX+1)

static DEFINE_SPINLOCK(sio_lock);

#define GPIO_NAME		"xc_gpio"
#define SIO_REG_LDSEL               0x07
#define SIO_GPIO_LDN                0x07
#define GPIO_BA_HIGH_BYTE	          0x62
#define GPIO_BA_LOW_BYTE	          0x63
#define GPIO_IOSIZE		              0x08
#define SIO_UNLOCK_KEY1             0x87
#define SIO_UNLOCK_KEY2             0x01
#define SIO_UNLOCK_KEY3             0x55
#define SIO_UNLOCK_KEY4             0x55
#define SIO_DUAL_SET                0x22
#define SIO_DUAL_SET_VALUE          0x80
#define SIO_LOCK_KEY1               0x02
#define SIO_LOCK_KEY2               0x02
#define SIO_DID_REG                 0x20
#define SIO_VID_REG                 0x22
#define DSIO_UNLOCK_KEY1            0x22
#define DSIO_UNLOCK_KEY2            0x80
#define DSIO_UNLOCK_KEY3            0x07
#define DSIO_UNLOCK_KEY4            0x07

#define MAX_GPIO_NR                 0x08
#define SIO_GPIO_SEL_REG            0x25
#define SIO_GPIO_SIMPLE_REG         0xC0
#define SIO_GPIO_INOUT_REG          0xC8

#define MAX_GPIO_PIN                GPIO_PIN_8
#define MAX_PCH_GPIO_PIN            PCH_GP_LED4

#define SIO_DID                     0x8784
#define DEFAULT_DUALIO              0

/* Defaults for Module Parameter */
#define LDN_GPIO		0x07

/* Configuration Registers and Functions */
#define LDN_REG		0x07
#define CHIP_ID		0x20
#define CHIP_REV		0x22
#define ACT_REG		0x30
#define BASE_REG		0x60

static	unsigned int chip_type;

static  int dualio = DEFAULT_DUALIO;

MODULE_PARM_DESC(dualio, "MainBoard support dual super IO, default="
		__MODULE_STRING(DEFAULT_DUALIO));

/**************************************************************/

static inline int superio_inb(int base, int reg);
static inline int superio_inw(int base, int reg);
static inline void superio_outb(int base, int reg, u8 val);
static inline void superio_set_bit(int base, int reg, int bit);
static inline void superio_clear_bit(int base, int reg, int bit);
static inline void gpio_set_bit(int base, int bit);
static inline void gpio_clear_bit(int base, int bit);
static inline void pch_gpio_set_bit(int base, int bit);
static inline void pch_gpio_clear_bit(int base, int bit);
static inline int superio_enter(int base);
static inline void superio_select(int base, int ld);
static inline void superio_exit(int base);

static void gpio_conf(int base);
static void gpio_base_addr_conf(u16 base);

u16 base_addr;
u16 gpio_base_addr[8];
u8 gpio_pin[8] = {36,10,37,23,56,60,57,61};

//PCH_12 PCH_24 PCH_44  LED1 LED2 LED3 LED4
u16 LED_base_addr[8] = {0x1D60,0x1DC0,0x1E60,0x1E08,0x1E30,0x1E38,0x1E70,0};

static inline int superio_inb(int base, int reg)
{
	outb(reg, base);
	return inb(base + 1);
}

static int superio_inw(int base, int reg)
{
	int val;
	val  = superio_inb(base, reg) << 8;
	val |= superio_inb(base, reg + 1);
	return val;
}

static inline void superio_outb(int base, int reg, u8 val)
{
	outb(reg, base);
	outb(val, base + 1);
}

static inline void superio_set_bit(int base, int reg, int bit)
{
	unsigned long val;
	superio_enter(base);
	val = superio_inb(base, reg);
	__set_bit(bit, &val);
	superio_outb(base, reg, val);
	superio_exit(base);
}

static inline void superio_clear_bit(int base, int reg, int bit)
{
	unsigned long val;
	superio_enter(base);
	val = superio_inb(base, reg);
	__clear_bit(bit, &val);
	superio_outb(base, reg, val);
	superio_exit(base);
}

static inline void gpio_set_bit(int base, int bit)
{
	int nbase = base;
	unsigned long val = inb(nbase);
	__set_bit(bit, &val);
	outb(val, nbase);
}

static inline void gpio_clear_bit(int base, int bit)
{
	int nbase = base;
	unsigned long val = inb(nbase);
	__clear_bit(bit, &val);
	outb(val, nbase);
}

static inline void pch_gpio_set_bit(int base, int bit)
{
	int nbase = base;
	unsigned long val;
	if (!request_muxed_region(base, 1, GPIO_NAME))
		return;
	val = inl(nbase);
	__set_bit(bit, &val);
	outl(val, nbase);
	release_region(base, 2);
}

static inline void pch_gpio_clear_bit(int base, int bit)
{
	int nbase = base;
	unsigned long val;
	
	if (!request_muxed_region(base, 1, GPIO_NAME))
		return;
	val = inl(nbase);
	__clear_bit(bit, &val);
	outl(val, nbase);
	release_region(base, 2);
}

static inline int superio_enter(int base)
{
	if (!request_muxed_region(base, 2, GPIO_NAME))
		return -EBUSY;
	outb(SIO_UNLOCK_KEY1, base);
	outb(SIO_UNLOCK_KEY2, base);
	outb(SIO_UNLOCK_KEY3, base);
	outb(SIO_UNLOCK_KEY4, base);
	if(dualio){
		outb(DSIO_UNLOCK_KEY1, base);
		outb(DSIO_UNLOCK_KEY2, base+1);
	}
	return 0;
}

static inline void superio_select(int base, int ld)
{
	outb(SIO_REG_LDSEL, base);
	outb(ld, base + 1);
}

static inline void superio_exit(int base)
{
	if(dualio){
			outb(DSIO_UNLOCK_KEY3, base);
			outb(DSIO_UNLOCK_KEY4, base+1);
	}
	else{
		outb(SIO_LOCK_KEY1, base);
		outb(SIO_LOCK_KEY2, base+1);
	}
	release_region(base, 2);
}

ssize_t gpio_write(struct file *file, const char __user *data,
		       size_t len, loff_t *ppos)
{
	return len;
}


ssize_t gpio_read(struct file *file, char __user * buf,
		      size_t len, loff_t * ppos)
{
    int rc = 0;
    u8 pin_index;
    unsigned long val;
    u8 iospace;

    if (len != sizeof(u8)) {
        return -EINVAL;
    }

    // GPIO 핀 인덱스를 사용자 공간에서 읽어오기
    if (copy_from_user(&pin_index, buf, sizeof(u8))) {
        return -EFAULT;
    }

    if (pin_index > MAX_GPIO_PIN) {
        return -EINVAL;
    }

    spin_lock(&sio_lock);
    
    // GPIO 상태 읽기
    val = inb(*(gpio_base_addr + pin_index));
    iospace = (val & (1 << ((*(gpio_pin + pin_index)) % 10))) ? 1 : 0;

    spin_unlock(&sio_lock);

    // 읽은 GPIO 상태를 사용자 공간으로 복사
    if (copy_to_user(buf, &iospace, sizeof(u8))) {
        return -EFAULT;
    }

    return sizeof(u8);

}


static long gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	u8 pin_index = 0;
	unsigned long val = 0;
	u8 iospace = 0;
	
	if (_IOC_TYPE(cmd) != TAIO_IOCTL_BASE) return -ENOTTY;
	if (_IOC_NR(cmd) > TAIO_MAX_NR) return -ENOTTY;
		
	switch(cmd){
		case SET_GPIO_LOWLEVEL:
			spin_lock(&sio_lock);
			if (get_user(pin_index, (u8 __user *)arg)){
				spin_unlock(&sio_lock);
				return -EFAULT;
			}
			if(pin_index > MAX_GPIO_PIN){
				spin_unlock(&sio_lock);
				return -EINVAL;
			}
			gpio_clear_bit(*(gpio_base_addr+pin_index),(*(gpio_pin+pin_index))%10);
			spin_unlock(&sio_lock);
			return rc;
		case SET_GPIO_HIGHLEVEL:
			spin_lock(&sio_lock);
			if (get_user(pin_index, (u8 __user *)arg)){
				spin_unlock(&sio_lock);
				return -EFAULT;
			}
			if(pin_index > MAX_GPIO_PIN){
				spin_unlock(&sio_lock);
				return -EINVAL;
			}
			gpio_set_bit(*(gpio_base_addr+pin_index),(*(gpio_pin+pin_index))%10);
			spin_unlock(&sio_lock);
			return rc;
		case GET_GPIO_LEVEL:
			spin_lock(&sio_lock);
			if (get_user(pin_index, (u8 __user *)arg)){
				spin_unlock(&sio_lock);
				return -EFAULT;
			}

			if(pin_index > MAX_GPIO_PIN){
				spin_unlock(&sio_lock);
				return -EINVAL;
			}
			val = inb(*(gpio_base_addr+pin_index));
			iospace = (val& (1 << ((*(gpio_pin+pin_index))%10)));
			if(iospace)
				iospace = 1;
			else
				iospace = 0;

			if (put_user(iospace, (u8 __user *)arg)){
				spin_unlock(&sio_lock);
				return -EFAULT;
			}
			spin_unlock(&sio_lock);
			return rc;
		case SET_LED_HIGHLEVEL:
			spin_lock(&sio_lock);
			if (get_user(pin_index, (u8 __user *)arg)){
				spin_unlock(&sio_lock);
				return -EFAULT;
			}
			if(pin_index > MAX_PCH_GPIO_PIN){
				spin_unlock(&sio_lock);
				return -EINVAL;
			}
			pch_gpio_set_bit(*(LED_base_addr+pin_index),31);
			spin_unlock(&sio_lock);
			return rc;
		case SET_LED_LOWLEVEL:
			spin_lock(&sio_lock);
			if (get_user(pin_index, (u8 __user *)arg)){
				spin_unlock(&sio_lock);
				return -EFAULT;
			}
			if(pin_index > MAX_PCH_GPIO_PIN){
				spin_unlock(&sio_lock);
				return -EINVAL;
			}
			pch_gpio_clear_bit(*(LED_base_addr+pin_index),31);
			spin_unlock(&sio_lock);
			return rc;
	default:
		return -ENOTTY;
	}
}

static int conf_gpio_sel(int base,u8 pin)
{
	u8 bit,reg,ngroup;
	u8 nbase = base;
	ngroup = pin /10;
	reg = SIO_GPIO_SEL_REG+ngroup-1;
	bit =  pin % 10;
	if(ngroup < 6){
			superio_set_bit(nbase, reg, bit);
	}
	return 0;
}

static int conf_gpio_simple(int base,u8 pin)
{
	u8 bit,reg,ngroup;
	u8 nbase = base;
	ngroup = pin /10;
	reg = SIO_GPIO_SIMPLE_REG+ngroup-1;
	bit =  pin % 10;
	if(ngroup < 6){
			superio_set_bit(nbase, reg, bit);
	}
	//printk(KERN_INFO"%s base is 0x%x, pin is 0x%x\n",__FUNCTION__,base, pin);
	return 0;
}

static int conf_gpio_inout(int base,u8 pin,u8 index)
{
	//u8 gpio_pin[8] = {64,66,63,67,23,16,37,14};
	u8 bit,reg,ngroup;
	u8 nbase = base;
	ngroup = pin /10;
	reg = SIO_GPIO_INOUT_REG+ngroup-1;
	bit =  pin % 10;
	if(index && ((index%2) == 1)){
		superio_clear_bit(nbase, reg, bit);
	}
	else{
		superio_set_bit(nbase, reg, bit);
	}
	return 0;
}

static void gpio_conf(int base)
{
	u8 chkloop = 0;
	u8 *ppin = gpio_pin;
	for(; chkloop < MAX_GPIO_NR; chkloop++){
		conf_gpio_sel(base,*(ppin+chkloop));
		conf_gpio_simple(base,*(ppin+chkloop));
		conf_gpio_inout(base,*(ppin+chkloop),chkloop);
	}
}

static void gpio_base_addr_conf(u16 base)
{
	u8 ngroup,chkloop = 0;
	u16 *ppinbaseaddr = gpio_base_addr;
	u8 *ppin = gpio_pin;
	u16 baseaddr = base;
	for(; chkloop < MAX_GPIO_NR; chkloop++){
		ngroup = *(ppin+chkloop) /10;
		*(ppinbaseaddr+chkloop) = baseaddr+ngroup-1;
	}
}

#if 0
static u8 config_gpio1_module_N85()
{
	
	return 0;
}
#endif

static void get_conf_reg_status(void)
{
#if 0
	u8 nsize,portval,chkloop = 0;
	superio_enter(IO_INDEX);

	superio_select(IO_INDEX,SIO_GPIO_LDN);
	//config gpio
	nsize = sizeof(global_reg);
	for(;chkloop < nsize; chkloop++){
		portval = (u8)superio_inb(IO_INDEX,*(global_reg+chkloop));
		printk(KERN_INFO"debug: %s() global_reg reg->val : 0x%x -> 0x%x\n",__FUNCTION__,*(global_reg+chkloop),portval);
	}

	superio_exit(IO_INDEX);

	for(chkloop = 0;chkloop < MAX_GPIO_NR; chkloop++){
		printk(KERN_INFO"debug: %s() gpio_base_addr value is : 0x%x\n",__FUNCTION__,*(gpio_base_addr+chkloop));
	}
#endif
}

static int gpio_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static const struct file_operations gpio_fileops = {
	.owner					= THIS_MODULE,
	.open						= gpio_open,
	.unlocked_ioctl = gpio_ioctl,
	.write					= gpio_write,
	.read						= gpio_read,
	.llseek 				= no_llseek,
};

static struct miscdevice gpio_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = GPIO_NAME,
	.fops = &gpio_fileops,
};

static int __init gpio_init(void)
{
	int rc;
	rc = superio_enter(IO_INDEX);
	if(rc){
		printk(KERN_INFO"%s enter pnp mode fail",__FUNCTION__);
		superio_exit(IO_INDEX);		
		return rc;
	}
	chip_type = superio_inw(IO_INDEX,SIO_DID_REG);
	if (chip_type != SIO_DID) {
		superio_exit(IO_INDEX);
		//release_region(IO_INDEX, 2);
		return -ENODEV;
	}
	superio_select(IO_INDEX,SIO_GPIO_LDN);
	base_addr = superio_inw(IO_INDEX,GPIO_BA_HIGH_BYTE);
	superio_exit(IO_INDEX);
	
	gpio_conf(IO_INDEX);
	if (!request_region(base_addr, GPIO_IOSIZE, GPIO_NAME)){
		return -EBUSY;
	}
  printk(KERN_INFO"%s request gpio address region 0x%x successful\n",__FUNCTION__,base_addr);
	rc = misc_register(&gpio_miscdev);
	if (rc) {
		pr_err("cannot register miscdev on minor=%d (err=%d)\n",
		       MISC_DYNAMIC_MINOR, rc);
		release_region(base_addr, GPIO_IOSIZE);
	}
	gpio_base_addr_conf(base_addr);
	get_conf_reg_status();
	return 0;
}

static void __exit gpio_exit(void)
{
	misc_deregister(&gpio_miscdev);
	release_region(base_addr, GPIO_IOSIZE);
}
module_init(gpio_init);
module_exit(gpio_exit);

MODULE_AUTHOR("ite");
MODULE_DESCRIPTION("GPIO interface for Super IO chip IT87XX V1.00");
MODULE_LICENSE("GPL");
