#ifndef __RS422_H__
#define __RS422_H__
#include "stm32f4xx_conf.h"

typedef struct str_rs422_info {
	u8 workFlag;
	u32 txCnt;
	u32 taskCnt;
	u32 curTaskLen;
	u32 curCnt;
	u32 curErrCnt;
	u32 abortCnt;
}RS422_INFO;

/** initialize RS422 */
void initRS422(void);

/** wait available of RS422 */
u8 waitAvailable_422(void);

/** start a transmit task */
u8 startTxTask_422(u8 *data,u32 len);

/** abort current task */
void abortTask_422(void);

/** get information of RS422 */
RS422_INFO * getInfo_422(void);

/** test RS422 */
void testRS422(void);

#endif //__RS422_H__
