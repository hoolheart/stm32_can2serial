#ifndef __MSG_INTERFACE_H__
#define __MSG_INTERFACE_H__

/**
 * This file builds up an interface of message engine
 */

enum MSG_CODE {
	MSG_CONSOLE_COMMAND,

	MSG_CANFSMC_BUSOFF,
	MSG_CANFSMC_AVAILABLE,
	MSG_CANFSMC_INTERRUPT,

	MSG_WORKTICK,
	
	MSG_422_TASKFINISH,
	MSG_422_TASKABORT,

	MSG_END
};

/** initialize interface */
void initMsgInterface(void);
/** publish a message */
int publishMsg(int msg,int para);
/** get pending message number */
unsigned int getPendingMsgNum(void);
/** get next pending message */
int getNextMsg(int *msg,int *para);

#endif //__MSG_INTERFACE_H__
