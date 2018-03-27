#include "delay.h"
 
static u8  fac_us=0;//us factor
static u16 fac_ms=0;//ms factor

/** initialize delay engine */
void delay_init(u8 SYSCLK)
{
 	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
	fac_us=SYSCLK/8;		//calculate us factor
	fac_ms=(u16)fac_us*1000;//calculate ms factor
}

/** delay given us */
void delay_us(u32 nus)
{		
	u32 temp;	    	 
	SysTick->LOAD=nus*fac_us; //load system ticks
	SysTick->VAL=0x00;        //reset system tick
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          //start
	do {
		temp=SysTick->CTRL;
	}
	while((temp&0x01)&&!(temp&(1<<16)));//wait to finish
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //stop
	SysTick->VAL =0X00;       //clear value
}

void delay_xms(u16 nms)
{
	u32 temp;		   
	SysTick->LOAD=(u32)nms*fac_ms;//load system ticks
	SysTick->VAL =0x00;           //clear value
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          //start tick
	do {
		temp=SysTick->CTRL;
	}
	while((temp&0x01)&&!(temp&(1<<16)));//wait to finish
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //stop
	SysTick->VAL =0X00;       //reset value
}

/** delay given ms */
void delay_ms(u16 nms)
{	 	 
	u8 repeat=nms/540;

	u16 remain=nms%540;
	while(repeat) {
		delay_xms(540);
		repeat--;
	}
	if(remain) {
		delay_xms(remain);
	}
}

/** delay a moment */
void delay_moment(void) {
	u32 temp;		   
	SysTick->LOAD=1;//load system ticks
	SysTick->VAL =0x00;           //clear value
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          //start tick
	do {
		temp=SysTick->CTRL;
	}
	while((temp&0x01)&&!(temp&(1<<16)));//wait to finish
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //stop
	SysTick->VAL =0X00;       //reset value
}
