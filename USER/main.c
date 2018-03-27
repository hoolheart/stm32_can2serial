/**
  ******************************************************************************
  * @file    main.c
  * @author  Zhou Huide
  * @version V1.0
  * @date    24-August-2016
  * @brief   Main function for multi-CAN interface board
  ******************************************************************************
*/


#include "stm32f4xx.h"
#include "msg_interface.h"
#include "msg_handlers.h"
#include "console.h"
#include "debug_led.h"
#include "delay.h"
#include "stdio.h"
#include "fsmc.h"
#include "can_fsmc.h"
#include "timer.h"
#include "protocol.h"
#include "rs422.h"

void initilize()
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	delay_init(168);//delay engine
	initMsgInterface();//message interface
	initConsole(115200);//console
	initFSMC();//FSMC
	initProtocol();//protocol layer

	initDebugLED();//LED
	//flash LED
	openAllLED();
	delay_ms(1000);
	closeAllLED();
	delay_ms(4000);

	printf("Hello, this is CAN Hub.\r\n");

	initCan_fsmc();//CAN through FSMC
	//test SJA1000
	testCAN();
	
	initRS422();//RS422
	//test RS422
	testRS422();
	
	//close all LED
	delay_ms(2000);
	closeAllLED();

	initTimer();//timer
}

void processMsg()
{
	int msg,para;//prepare data
	if(getNextMsg(&msg,&para)==0) {
		switch(msg) {
		case MSG_CONSOLE_COMMAND:
			onConsoleCommand(para);
			break;
		case MSG_CANFSMC_BUSOFF:
			onCANBusOff_fsmc(para);
			break;
		case MSG_CANFSMC_AVAILABLE:
			onCANBuffer_fsmc();
			break;
		case MSG_CANFSMC_INTERRUPT:
			handleCANInterrupt();
			break;
		case MSG_WORKTICK:
			onWorkTick(para);
			break;
		case MSG_422_TASKFINISH:
			onRS422Finished();
			break;
		case MSG_422_TASKABORT:
			onRS422Aborted();
			break;
		default:
			break;
		}
	}
}

int main(void)
{
	initilize();

	while(1) {
		processMsg();
	}
}
