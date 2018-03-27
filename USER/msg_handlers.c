#include "msg_handlers.h"
#include "console.h"
#include "can_fsmc.h"
#include "delay.h"
#include "debug_led.h"
#include "protocol.h"
#include "rs422.h"

#include "string.h"
#include "stdio.h"

/** on command received from console */
void onConsoleCommand(int para) {
	u32 tmp32;
	CAN_INFO *info;
	RS422_INFO *info_422;
	if(getCommandNum()>0) {
		CONSOLE_CMD * cmd = getCommand();
		if(strcmp((char*)cmd->cmd,"test_can")==0) {
			testCAN();
		}
		else if(strcmp((char*)cmd->cmd,"show_status_can")==0) {
			for(tmp32=0;tmp32<CAN_FSMC_CNT;tmp32++) {
				info = getCanInfo_fsmc(tmp32);//get info
				printf("Info of CAN channel %u:\r\n",tmp32);
				printf("Bus status: %d\r\n",info->busStatus);
				printf("Self-test mode: %d\r\n",info->testFlag);
				printf("Error flag: %d\r\n",info->errorFlag);
				printf("Reset flag: %d\r\n",info->resetFlag);
				printf("Tx count: %u\r\n",info->txCnt);
				printf("Rx count: %u\r\n",info->rxCnt);
				printf("Tx error: %d\r\n",info->txErrCnt);
				printf("Rx error: %d\r\n",info->rxErrCnt);
				printf("Reset count: %u\r\n\r\n",info->resetCnt);
			}
		}
		else if(strcmp((char*)cmd->cmd,"test_rs422")==0) {
			testRS422();
		}
		else if(strcmp((char*)cmd->cmd,"show_status_rs422")==0) {
			info_422 = getInfo_422();//get info
			printf("RS422 status:\r\n");
			printf("Work status %u:\r\n",info_422->workFlag);
			printf("Bytes sent: %u\r\n",info_422->txCnt);
			printf("Tasks sent: %u\r\n",info_422->taskCnt);
			printf("Current task length: %u\r\n",info_422->curTaskLen);
			printf("Current bytes sent: %u\r\n",info_422->curCnt);
			printf("Current error count: %u\r\n",info_422->curErrCnt);
			printf("Tasks aborted: %u\r\n\r\n",info_422->abortCnt);
		}
		else if(strcmp((char*)cmd->cmd,"print_report")==0) {
			printReport(0);
		}
		else if(strcmp((char*)cmd->cmd,"print_report_hex")==0) {
			printReport(1);
		}
	}
}

/** on CAN bus off */
void onCANBusOff_fsmc(int para) {
	reInitCAN_fsmc((u32)para);//reset can channel
}

/** on CAN buffer available */
void onCANBuffer_fsmc(void) {
	CAN_BLOCK block;
	while(readPendingCan_fsmc(&block)==0) {//fetch CAN block
		//handle CAN block
		handleCANBlock(&block);
	}
}

/** on RS422 task finished */
void onRS422Finished(void) {
	RS422_INFO *info = getInfo_422();//get info
	if(info->curErrCnt>0) {
		printf("Report task has error, error count: %u\r\n",info->curErrCnt);
	}
}

/** on RS422 task aborted */
void onRS422Aborted(void) {
	printf("Previous report task has been canceled\r\n");
}

/** on work tick */
void onWorkTick(int para) {
	//work
	if((para>=0) && (para<8)) {
		//query
		queryCANDevice((u8)para);
	}
	else if(para==9) {
		//report
		reportInfo();
	}
	//heartbeat
	if(para<5) {
		openLED(0);
	}
	else {
		closeLED(0);
	}
}
