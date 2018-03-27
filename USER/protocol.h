#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "can.h"

/** setup protocol */
void initProtocol(void);

/** handle CAN block */
void handleCANBlock(CAN_BLOCK *block);

/** query CAN device */
void queryCANDevice(u8 index);

/** report gathered information */
void reportInfo(void);

/** print next report */
void printReport(u8 mode);

#endif //__PROTOCOL_H__
