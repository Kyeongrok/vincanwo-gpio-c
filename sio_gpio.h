#ifndef _T_OEM_CTRL_H
#define _T_OEM_CTRL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define GPIO_PIN_1                  0x00
#define GPIO_PIN_2                  0x01
#define GPIO_PIN_3                  0x02
#define GPIO_PIN_4                  0x03
#define GPIO_PIN_5                  0x04
#define GPIO_PIN_6                  0x05
#define GPIO_PIN_7                  0x06
#define GPIO_PIN_8                  0x07

#define PCH_GPIO12                  0x00
#define PCH_GPIO24                  0x01
#define PCH_GPIO44                  0x02
#define PCH_GP_LED1                 0x03
#define PCH_GP_LED2                 0x04
#define PCH_GP_LED3                 0x05
#define PCH_GP_LED4                 0x06

#define TAIO_MAX_NR                     4

#define	TAIO_IOCTL_BASE	'X'

#define SET_GPIO_LOWLEVEL               _IOWR(TAIO_IOCTL_BASE,0,int)
#define SET_GPIO_HIGHLEVEL              _IOWR(TAIO_IOCTL_BASE,1,int)
#define GET_GPIO_LEVEL                  _IOR(TAIO_IOCTL_BASE,2,int)
#define SET_LED_LOWLEVEL               _IOWR(TAIO_IOCTL_BASE,3,int)
#define SET_LED_HIGHLEVEL              _IOWR(TAIO_IOCTL_BASE,4,int)

#endif  //_XBROTHER_T_OEM_CTRL_H