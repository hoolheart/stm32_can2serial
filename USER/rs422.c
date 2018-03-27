#include "rs422.h"
#include "fsmc.h"
#include "stm32f4xx.h"
#include "stm32f4xx_exti.h"
#include "msg_interface.h"
#include "delay.h"
#include "stdio.h"
#include "debug_led.h"

#define RS422_RATE1_ADDR 0x100
#define RS422_RATE2_ADDR 0x200

#define RS422_TX1_ADDR 0x102
#define RS422_TX2_ADDR 0x202

#define RS422_RX1_ADDR 0x103
#define RS422_RX2_ADDR 0x203

#define RS422_RX1_FLAG_ADDR 0x104
#define RS422_RX2_FLAG_ADDR 0x204

#define RS422_BAUDRATE (10e3)

#define BUFFER_RS422_SIZE 4096

RS422_INFO rs422Info;
u8 buffer422[BUFFER_RS422_SIZE];

/** initialize RS422 */
void initRS422(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef   NVIC_InitStructure;
	EXTI_InitTypeDef   EXTI_InitStructure;
	u16 rate = 10e6/RS422_BAUDRATE;
	u8 tmp;
	
	//initialize info struct
	rs422Info.workFlag = 0;
	rs422Info.txCnt = 0;
	rs422Info.taskCnt = 0;
	rs422Info.curTaskLen = 0;
	rs422Info.curCnt = 0;
	rs422Info.curErrCnt = 0;
	rs422Info.abortCnt = 0;

	//initialize GPIO
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//enable GPIOC
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; //GPIOC3
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//INPUT
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//pull-up
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);//enable SYSCFG
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource3);//PC3 --> exti3

	/* configure EXTI3 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line3;//LINE3
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//interrupt
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //falling to trigger
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;//enable LINE3
	EXTI_Init(&EXTI_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;//EXTI 2
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;//priority 0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;//sub priority 2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//enable
	NVIC_Init(&NVIC_InitStructure);

	//set rate
	tmp = (u8)rate;
	writeFSMC(RS422_RATE1_ADDR,tmp);
	writeFSMC(RS422_RATE2_ADDR,tmp);
	tmp = (u8)(rate>>8);
	writeFSMC(RS422_RATE1_ADDR+1,tmp);
	writeFSMC(RS422_RATE2_ADDR+1,tmp);

  //close LED
	closeLED(8);
}

//EXTI3 handler
void EXTI3_IRQHandler(void) {
	u8 tmp=0,valid,success=1;
	if(EXTI_GetITStatus(EXTI_Line3)==SET) {
		if(rs422Info.workFlag && (rs422Info.curCnt<rs422Info.curTaskLen)) {
			//check status
			valid = readFSMC(RS422_RX1_FLAG_ADDR);
			while(valid) {
				tmp = readFSMC(RS422_RX1_ADDR);
				valid = readFSMC(RS422_RX1_FLAG_ADDR);
			}
			if(tmp!=buffer422[rs422Info.curCnt]) {
				success = 0;
			}
			valid = readFSMC(RS422_RX2_FLAG_ADDR);
			while(valid) {
				tmp = readFSMC(RS422_RX2_ADDR);
				valid = readFSMC(RS422_RX2_FLAG_ADDR);
			}
			if(tmp!=buffer422[rs422Info.curCnt]) {
				success = 0;
			}
			if(success==0) {
				rs422Info.curErrCnt++;
			}
			//send next
			rs422Info.curCnt++;
			if(rs422Info.curCnt<rs422Info.curTaskLen) {
				writeFSMC(RS422_TX1_ADDR,buffer422[rs422Info.curCnt]);
				writeFSMC(RS422_TX2_ADDR,buffer422[rs422Info.curCnt]);
				rs422Info.txCnt++;
			}
			else {
				rs422Info.workFlag = 0;
				closeLED(8);
				publishMsg(MSG_422_TASKFINISH,0);
			}
		}
		else if(rs422Info.workFlag) {
			rs422Info.workFlag = 0;
			closeLED(8);
			publishMsg(MSG_422_TASKFINISH,0);
		}
	}
	EXTI_ClearITPendingBit(EXTI_Line3);
}

/** wait available of RS422 */
u8 waitAvailable_422(void) {
	u8 i;
	for(i=0;i<100;i++) {
		if(rs422Info.workFlag) {
			delay_ms(1);
		}
		else {
			break;
		}
	}
	return rs422Info.workFlag;
}

/** start a transmit task */
u8 startTxTask_422(u8 *data,u32 len) {
	u32 i;
	//check status and task
	if(rs422Info.workFlag || (len>BUFFER_RS422_SIZE) || (len==0)) {
		return 1;
	}
	//copy data
	for(i=0;i<len;i++) {
		buffer422[i] = data[i];
	}
	//initialize task info
	rs422Info.curTaskLen = len;
	rs422Info.curCnt = 0;
	rs422Info.curErrCnt = 0;
	rs422Info.workFlag = 1;
	openLED(8);
	rs422Info.taskCnt++;
	//start
	writeFSMC(RS422_TX1_ADDR,buffer422[rs422Info.curCnt]);
	writeFSMC(RS422_TX2_ADDR,buffer422[rs422Info.curCnt]);
	rs422Info.txCnt++;
	return 0;
}

/** abort current task */
void abortTask_422(void) {
	if(rs422Info.workFlag) {
		rs422Info.workFlag = 0;
		closeLED(8);
		publishMsg(MSG_422_TASKABORT,0);
	}
}

/** get information of RS422 */
RS422_INFO * getInfo_422(void) {
	return &rs422Info;
}

/** test RS422 */
void testRS422(void) {
	EXTI_InitTypeDef   EXTI_InitStructure;
	u16 i,cnt;
	u8 valid,tmp;
	
	//wait to finish
	if(waitAvailable_422()) {
		abortTask_422();
	}

	//stop interrupt
	EXTI_InitStructure.EXTI_Line = EXTI_Line3;//LINE3
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//interrupt
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //falling to trigger
	EXTI_InitStructure.EXTI_LineCmd = DISABLE;//disable LINE3
	EXTI_Init(&EXTI_InitStructure);
	
	printf("Testing RS422...\r\n");
	//test channel 1
	cnt = 0;
	for(i=0;i<256;i++) {
		writeFSMC(RS422_TX1_ADDR,(u8)i);
		delay_us(100);
		valid = readFSMC(RS422_RX1_FLAG_ADDR);
		while(valid==1) {
			tmp = readFSMC(RS422_RX1_ADDR);
			valid = readFSMC(RS422_RX1_FLAG_ADDR);
		}
		if(tmp!=((u8)i)) {
			cnt++;
		}
	}
	if(cnt==0) {
		printf("RS422 Channel 1: Success\r\n");
	}
	else {
		printf("RS422 Channel 1: Error Count %u\r\n",cnt);
	}
	//test channel 2
	cnt = 0;
	for(i=0;i<256;i++) {
		writeFSMC(RS422_TX2_ADDR,(u8)i);
		delay_us(100);
		valid = readFSMC(RS422_RX2_FLAG_ADDR);
		while(valid==1) {
			tmp = readFSMC(RS422_RX2_ADDR);
			valid = readFSMC(RS422_RX2_FLAG_ADDR);
		}
		if(tmp!=((u8)i)) {
			cnt++;
		}
	}
	if(cnt==0) {
		printf("RS422 Channel 2: Success\r\n");
	}
	else {
		printf("RS422 Channel 2: Error Count %u\r\n",cnt);
	}
	
	//restart interrupt
	EXTI_InitStructure.EXTI_Line = EXTI_Line3;//LINE3
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//interrupt
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //falling to trigger
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;//enable LINE3
	EXTI_Init(&EXTI_InitStructure);
}
