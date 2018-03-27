#ifndef __DELAY_H
#define __DELAY_H
#include "stm32f4xx_conf.h"

/** initialize delay engine */
void delay_init(u8 SYSCLK);
/** delay given us */
void delay_ms(u16 nms);
/** delay given ms */
void delay_us(u32 nus);
/** delay a moment */
void delay_moment(void);

#endif





























