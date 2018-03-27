#include "msg_interface.h"

/****************** prepare data *****************/

/** message structure */
typedef struct strMsgBody {
	int msg;
	int para;
}MSG_BODY;

#define MSG_MAX_LEN (512/8)

MSG_BODY msgList[MSG_MAX_LEN];
unsigned int iMsgPull = 0, iMsgPush = 0;
unsigned int nMsg = 0;

/****************** functions *****************/

/** initialize interface */
void initMsgInterface()
{
	iMsgPull = 0;
	iMsgPush = 0;
	nMsg = 0;
}

/** publish a message */
int publishMsg(int msg,int para)
{
	if(nMsg<MSG_MAX_LEN) {//check number
		msgList[iMsgPush].msg = msg;//record message
		msgList[iMsgPush].para = para;
		iMsgPush = (iMsgPush+1)%MSG_MAX_LEN;//increase index
		nMsg++;//increase number
		return 0;
	}
	else {
		return 1;
	}
}

/** get pending message number */
unsigned int getPendingMsgNum()
{
	return nMsg;
}

/** get next pending message */
int getNextMsg(int *msg,int *para)
{
	if(nMsg>0) {
		*msg = msgList[iMsgPull].msg;//output message
		*para = msgList[iMsgPull].para;
		iMsgPull = (iMsgPull+1)%MSG_MAX_LEN;//increase index
		nMsg--;
		return 0;
	}
	else {
		return 1;
	}
}
