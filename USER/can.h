#ifndef DEF_CAN_BLOCK
#define DEF_CAN_BLOCK

#include "stm32f4xx_conf.h"

/** standard CAN block */
typedef struct str_can_block {
	u32 addr;
	u8  canId;
	u8  len:6;
	u8  remoteFlag:1;
	u8  extendFlag:1;
	u8  canData[8];
}CAN_BLOCK;

/** can info */
typedef struct str_can_info {
	u8 busStatus:1;
	u8 testFlag:1;
	u8 errorFlag:1;
	u8 resetFlag:1;
	u8 :4;
	u32 txCnt;
	u32 rxCnt;
	u8  txErrCnt;
	u8  rxErrCnt;
	u32 resetCnt;
}CAN_INFO;

#endif //DEF_CAN_BLOCK
