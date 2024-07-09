#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <termios.h>
#include <sys/io.h>

//==========================================================WatchDogTimer=========================================================//
#define SIO_CONFIG_INDEX 0x2E
#define SIO_CONFIG_DATA 0x2F
void WatchDogTimer(uint8_t TimerValue, uint8_t Unit) // 1 < TimerValue < 255 , Unit = Second or Minute
{
	// Enter Configuration Mode.
	outb_p(0x87, SIO_CONFIG_INDEX );
	outb_p(0x87, SIO_CONFIG_INDEX );

	//=====================LDN 07====================================//
	outb_p(0x07, SIO_CONFIG_INDEX );
	outb_p(0x07, SIO_CONFIG_DATA );

	//=====================WDT====================================//   
	outb_p(0xF6, SIO_CONFIG_INDEX );
	outb_p(TimerValue, SIO_CONFIG_DATA );	//Enable WDT
	
	if(Unit == 0)
	{
	    outb_p(0xF5, SIO_CONFIG_INDEX );
	    outb_p(0x72, SIO_CONFIG_DATA );	    // Second
	}
	else
	{
		outb_p(0xF5, SIO_CONFIG_INDEX );
	    outb_p(0x7A, SIO_CONFIG_DATA );	    // Minute	
	}

	outb_p(0xFA, SIO_CONFIG_INDEX);
	outb_p(((inb_p(SIO_CONFIG_DATA) & 0xFE) | 0x01), SIO_CONFIG_DATA );	    // WDT EN	
}

void DisableWdt()
{
	// Enter Configuration Mode.
	outb_p(0x87, SIO_CONFIG_INDEX );
	outb_p(0x87, SIO_CONFIG_INDEX );

	//=====================LDN 07====================================//
	outb_p(0x07, SIO_CONFIG_INDEX );
	outb_p(0x07, SIO_CONFIG_DATA );

	//=====================WDT====================================//   
	outb_p(0xF5, SIO_CONFIG_INDEX );
	outb_p(0x00, SIO_CONFIG_DATA );	//

	outb_p(0xF6, SIO_CONFIG_INDEX);
	outb_p((inb_p(SIO_CONFIG_DATA) & 0xDF), SIO_CONFIG_DATA);	//
        
	outb_p(0xFA, SIO_CONFIG_INDEX);
	outb_p((inb_p(SIO_CONFIG_DATA) & 0xFE), SIO_CONFIG_DATA);	//
}
//================================================================================================================================//

//GPIO4 Input------------------0xA02.bit4
int IsGpio4Set()
{
	if(inb_p(0xA02) & 0x10) 
		return 1;
	else 
		return 0;
}

//GPIO5 Input------------------0xA02.bit5
int IsGpio5Set()
{
	if(inb_p(0xA02) & 0x20) 
		return 1;
	else 
		return 0;
}

//GPIO6 Input------------------0xA02.bit6
int IsGpio6Set()
{
	if(inb_p(0xA02) & 0x40) 
		return 1;
	else 
		return 0;
}

//GPIO7 Input------------------0xA02.bit7
int IsGpio7Set()
{
	if(inb_p(0xA02) & 0x80) 
		return 1;
	else 
		return 0;
}

//GPIO0 Output------------------0xA02.bit0
void Gpio0Set(int outputvalue)
{
	if(outputvalue == 0)
		outb_p(((inb_p(0xA02) & 0xFE) | 0x00),0xA02 );//GPO0输出低电平
	else
		outb_p(((inb_p(0xA02) & 0xFE) | 0x01),0xA02 );//GPO0输出高电平

}

//GPIO1 Output------------------0xA02.bit1
void Gpio1Set(int outputvalue)
{
	if(outputvalue == 0)
		outb_p(((inb_p(0xA02) & 0xFD) | 0x00),0xA02 );//GPO1输出低电平
	else
		outb_p(((inb_p(0xA02) & 0xFD) | 0x02),0xA02 );//GPO1输出高电平
}


//GPIO2 Output------------------0xA02.bit2
void Gpio2Set(int outputvalue)
{
	if(outputvalue == 0)
		outb_p(((inb_p(0xA02) & 0xFB) | 0x00),0xA02 );//GPO2输出低电平
	else
		outb_p(((inb_p(0xA02) & 0xFB) | 0x04),0xA02 );//GPO2输出高电平
}

//GPIO3 Output------------------0xA02.bit3
void Gpio3Set(int outputvalue)
{
	if(outputvalue == 0)
		outb_p(((inb_p(0xA02) & 0xF7) | 0x00),0xA02 );//GPO3输出低电平
	else
		outb_p(((inb_p(0xA02) & 0xF7) | 0x08),0xA02 );//GPO3输出高电平
}


void main()
{
	uint8_t iodata8;
	int ret = iopl(3);
	printf("iopl(3):%d\n", ret);
	if (ret < 0)
	{
		perror("iopl ser error");
		printf("iopl ser error!\n");
		return;
	}
	
	//WatchDogTimer(50,0); //喂狗50秒
	//WatchDogTimer(40,1); //喂狗40分
	
	int kk = IsGpio7Set();

	printf("kk:%d\n", kk);
	if(IsGpio4Set())
	{
		//GPIO4输入高
	}
	else
	{
		//GPIO4输入低
	}
	
	Gpio0Set(1);//GPIO0输出高
	Gpio0Set(0);//GPIO0输出低

	while(1)
	{
		printf("here");
		usleep(100000);
	}
}
