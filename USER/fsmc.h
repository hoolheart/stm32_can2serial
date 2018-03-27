/*
 * can_fsmc.h
 *
 *  Created on: SEP 14, 2016
 *      Author: hzhou
 */

#ifndef FSMC_H_
#define FSMC_H_
#include "stm32f4xx_conf.h"

/** initialize CAN interface by FSMC */
void initFSMC(void);
/** read from FSMC */
u8 readFSMC(u32 addr);
/** write a byte to FSMC */
void writeFSMC(u32 addr,u8 val);

#endif /* FSMC_H_ */
