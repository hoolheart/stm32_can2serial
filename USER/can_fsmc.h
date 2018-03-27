/*
 * can_fsmc.h
 *
 *  Created on: SEP 14, 2016
 *      Author: hzhou
 */

#ifndef CAN_FSMC_H_
#define CAN_FSMC_H_

#include "stm32f4xx_conf.h"
#include "can.h"

/** count of CAN channels through FSMC */
#define CAN_FSMC_CNT 22

/** initialize CAN interface by FSMC */
void initCan_fsmc(void);
/** wait for availability of CAN channel */
u8 waitCANAvail_fsmc(u32 index);
/** send CAN message */
u8 sendCanMsg_fsmc(CAN_BLOCK *block);
/** get pending number of received CAN messages */
u32 getPendingCanNumber_fsmc(void);
/** read next received CAN message */
u8 readPendingCan_fsmc(CAN_BLOCK *block);
/** clear receive buffer */
void clearPendingCAN_fsmc(void);
/** get status of CAN channel */
CAN_INFO* getCanInfo_fsmc(u32 index);
/** enable/disable self-testing */
void enableSelfTest_fsmc(u32 index, u8 enabled);
/** test CAN */
u8 testCAN_fsmc(u32 index);
/** re-initialize channel */
void reInitCAN_fsmc(u32 index);
/** handle interrupt */
void handleCANInterrupt(void);
/** test CAN */
void testCAN(void);

#endif /* CAN_FSMC_H_ */
