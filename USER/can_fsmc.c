/*
 * can_fsmc.c
 *
 *  Created on: SEP 14, 2016
 *      Author: hzhou
 */

#include "can_fsmc.h"
#include "fsmc.h"
#include "stm32f4xx.h"
#include "stm32f4xx_fsmc.h"
#include "stm32f4xx_exti.h"
#include "delay.h"
#include "msg_interface.h"
#include "debug_led.h"
#include "stdio.h"

/** addresses of SJA1000 registers */
#define	SJA_MOD	  0
#define	SJA_CMR   1
#define	SJA_SR 	  2
#define	SJA_IR 	  3
#define	SJA_IER   4
#define	SJA_BTR0  6
#define	SJA_BTR1  7
#define	SJA_OCR   8
#define	SJA_TEST  9
#define	SJA_ALC   0x0b
#define	SJA_ECC   0x0c
#define	SJA_EMLR  0x0d
#define	SJA_RXERR 0x0e
#define	SJA_TXERR 0x0f
#define	SJA_ACR0  0x10
#define	SJA_ACR1  0x11
#define	SJA_ACR2  0x12
#define	SJA_ACR3  0x13
#define	SJA_AMR0  0x14
#define	SJA_AMR1  0x15
#define	SJA_AMR2  0x16
#define	SJA_AMR3  0x17
#define	SJA_AB    0x10
#define	SJA_DB    0x13
#define	SJA_RBSA  0x1e
#define	SJA_CDR   0x1f

#define MAX_RX_CAN_BLCK 100

/** information of all channels */
CAN_INFO canInfo_fsmc[CAN_FSMC_CNT];

/** receive buffers */
CAN_BLOCK rxBuff_fsmc[MAX_RX_CAN_BLCK];
u32 iReadRxBuff_fsmc,iWriteRxBuff_fsmc,cntRxBuff_fsmc;

void resetAllCAN_fsmc(void) {
	u32 cnt = (CAN_FSMC_CNT+7)/8;
	u32 i;
	for(i=0;i<cnt;i++) {
		writeFSMC(i,0x0);
	}
	delay_ms(10);
	for(i=0;i<cnt;i++) {
		writeFSMC(i,0xff);
	}
}

void resetCAN_fsmc(u32 index) {
	u32 addr = index/8;
	u8  val  = (((u8)0x01)<<(index%8));
	writeFSMC(addr,~val);
	delay_ms(10);
	writeFSMC(addr,0xFF);
}

u8 checkReg(u32 addr,u8 val,u8 cnt) {
	uint8_t tmp;
	u8 i;
	for(i=0;i<cnt;i++) {
		tmp = readFSMC(addr);
		if((tmp&val)==val) {
			break;
		}
	}
	if(i<cnt) {
		return 0;
	}
	else {
		return 1;
	}
}

/** initialize SJA1000 */
void initSJA1000(u32 index) {
	u32 base_addr = 0x2000+(index<<8);//get base address 0x2000 0x2100 ...
	uint8_t temp_data1;

	if(index>=CAN_FSMC_CNT) {
		return;
	}

	writeFSMC(base_addr+SJA_MOD,0x01); //enter reset mode
	temp_data1 = readFSMC(base_addr+SJA_MOD);
	temp_data1 = temp_data1 & 0x01;
	if(temp_data1  ==  0x01) {
		//set PeliCAN mode
		writeFSMC(base_addr+SJA_CDR,0XC0);

		//timing: 250K
		writeFSMC(base_addr+SJA_BTR0,0X83);
		writeFSMC(base_addr+SJA_BTR1,0XA3);

		//output mode: normal, pull-push
		writeFSMC(base_addr+SJA_OCR,0X1A);

		//no filter
		writeFSMC(base_addr+SJA_ACR0,0XFF);
		writeFSMC(base_addr+SJA_ACR1,0XFF);
		writeFSMC(base_addr+SJA_ACR2,0XFF);
		writeFSMC(base_addr+SJA_ACR3,0XFF);
		writeFSMC(base_addr+SJA_AMR0,0XFF);
		writeFSMC(base_addr+SJA_AMR1,0XFF);
		writeFSMC(base_addr+SJA_AMR2,0XFF);
		writeFSMC(base_addr+SJA_AMR3,0XFF);

		//set error threshold
		writeFSMC(base_addr+SJA_EMLR,128);

		//interrupt: overrun, error, and receive
		writeFSMC(base_addr+SJA_IER,0X0D);
	}
	writeFSMC(base_addr+SJA_MOD,0X08);//NORMAL MODE
	checkReg(base_addr+SJA_MOD,0x08,10);//check mode

	//initialize can info
	temp_data1 = readFSMC(base_addr+SJA_SR);//get status
	canInfo_fsmc[index].busStatus = (temp_data1&0x80)>0?0:1;
	canInfo_fsmc[index].errorFlag = (temp_data1&0x40)>0?1:0;
}

/** initialize external interrupt */
void initExti_fsmc(void) {
	NVIC_InitTypeDef   NVIC_InitStructure;
	EXTI_InitTypeDef   EXTI_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure;

	//initialize GPIO
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//enable GPIOC
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PC2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//INPUT
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//pull-up
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);//enable SYSCFG
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource2);//PC2 --> exti2

	/* configure EXTI10 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line2;//LINE2
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//interrupt
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //falling to trigger
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;//enable LINE10
	EXTI_Init(&EXTI_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;//EXTI 2
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;//priority 0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;//sub priority 2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//enable
	NVIC_Init(&NVIC_InitStructure);
}

/** initialize CAN interface by FSMC */
void initCan_fsmc(void)
{
	u32 index;
	u8 * pInfo = (u8*)canInfo_fsmc;
	//initialize info
	for(index=0;index<sizeof(CAN_INFO)*CAN_FSMC_CNT;index++) {
		pInfo[index] = 0;
	}
	//reset
	resetAllCAN_fsmc();
	//configure SJA1000
	for(index=0;index<CAN_FSMC_CNT;index++) {
		initSJA1000(index);
	}
	//initialize rx buffer
	iReadRxBuff_fsmc = 0;
	iWriteRxBuff_fsmc = 0;
	cntRxBuff_fsmc = 0;
	//initialize interrupt
	initExti_fsmc();
}

/** wait for availability of CAN channel */
u8 waitCANAvail_fsmc(u32 index) {
	u8 i;
	u8 tmp;
	u32 base_addr = 0x2000+(index<<8);//get base address 0x2000 0x2100 ...
	if(index>=CAN_FSMC_CNT) {//check index
		return 1;
	}
	for(i=0;i<10;i++) {
		tmp = readFSMC(base_addr+SJA_SR);
		if(tmp&0x04) {
			break;
		}
		delay_us(100);
	}
	//return result
	if(i<10) {
		return 0;
	}
	else {
		return 1;
	}
}

/** send CAN message */
u8 sendCanMsg_fsmc(CAN_BLOCK *block)
{
	u8 tmp,i;
	u32 base_addr;
	u32 can_addr = block->addr;
	if(block->canId<CAN_FSMC_CNT) {
		base_addr = 0x2000+(u32)block->canId*0x100;//get base address
		tmp = readFSMC(base_addr+SJA_SR);//get status
		if(tmp&0x04) {//check status
			//length
			if(block->len>8) {
				tmp = 8;
			}
			else {
				tmp = block->len;
			}
			//remote flag
			can_addr <<= 1;
			if(block->remoteFlag) {
				tmp |= 0x40;
				can_addr |= 0x01;
			}
			//extend flag
			if(block->extendFlag) {
				tmp |= 0x80;
				can_addr = can_addr<<2;
			}
			else {
				can_addr = can_addr<<20;
			}
			writeFSMC(base_addr+SJA_AB,tmp);
			//address
			for(i=0;i<4;i++) {
				tmp = (u8)can_addr;
				if((i>1) || (block->extendFlag)) {
					writeFSMC(base_addr+SJA_AB+4-i,tmp);
				}
				can_addr >>= 8;
			}
			//data
			for(i=0;(i<block->len)&&(i<8);i++) {
				if(block->extendFlag) {
					writeFSMC(base_addr+SJA_DB+2+i,block->canData[i]);
				}
				else {
					writeFSMC(base_addr+SJA_DB+i,block->canData[i]);
				}
			}
			//send
			if(canInfo_fsmc[block->canId].testFlag) {
				//test
				writeFSMC(base_addr+SJA_CMR,0x10);
			}
			else {
				//normal
				writeFSMC(base_addr+SJA_CMR,0x01);
				canInfo_fsmc[block->canId].txCnt++;
			}
			return 0;
		}
	}
	return 1;
}

/** read a block */
u8 readCanMsg_fsmc(u32 index,CAN_BLOCK *block) {
	u32 base_addr = 0x2000+(index<<8);//get base address 0x2000 0x2100 ...
	u8 tmp = readFSMC(base_addr+SJA_SR);
	if((index<CAN_FSMC_CNT) && (tmp&0x01)) {
		block->canId = index;
		tmp = readFSMC(base_addr+SJA_AB);
		//length
		block->len = tmp&0x0f;
		//extend flag
		if(tmp&0x80) {
			block->extendFlag = 1;
		}
		else {
			block->extendFlag = 0;
		}
		//remote flag
		if(tmp&0x40) {
			block->remoteFlag = 1;
		}
		else {
			block->remoteFlag = 0;
		}
		//address
		for(tmp=0;tmp<4;tmp++) {
			block->addr <<= 8;
			block->addr += readFSMC(base_addr+SJA_AB+tmp+1);
		}
		if(block->extendFlag) {
			block->addr >>= 3;
		}
		else {
			block->addr >>= 21;
		}
		//data
		for(tmp=0;(tmp<block->len)&&(tmp<8);tmp++) {
			if(block->extendFlag) {
				block->canData[tmp] = readFSMC(base_addr+SJA_DB+2+tmp);
			}
			else {
				block->canData[tmp] = readFSMC(base_addr+SJA_DB+tmp);
			}
		}
		//release receive buffer
		writeFSMC(base_addr+SJA_CMR,0x04);
		canInfo_fsmc[index].rxCnt++;
		return 0;
	}
	else {
		return 1;
	}
}

/** get pending number of received CAN messages */
u32 getPendingCanNumber_fsmc()
{
	return cntRxBuff_fsmc;
}

/** read next received CAN message */
u8 readPendingCan_fsmc(CAN_BLOCK *block)
{
	if(cntRxBuff_fsmc>0) {
		*block = rxBuff_fsmc[iReadRxBuff_fsmc];//copy block
		iReadRxBuff_fsmc = (iReadRxBuff_fsmc+1)%MAX_RX_CAN_BLCK;//increase index
		cntRxBuff_fsmc--;
		return 0;
	}
	return 1;
}

/** clear receive buffer */
void clearPendingCAN_fsmc() {
	iReadRxBuff_fsmc = 0;
	iWriteRxBuff_fsmc = 0;
	cntRxBuff_fsmc = 0;
}

/** get status of CAN channel */
CAN_INFO* getCanInfo_fsmc(u32 index) {
	u32 base_addr = 0x2000+(index<<8);//get base address 0x2000 0x2100 ...
	u8 tmp;
	if(index<CAN_FSMC_CNT) {
		//fetch real-time info
		tmp = readFSMC(base_addr+SJA_SR);
		canInfo_fsmc[index].busStatus = (tmp&0x80)>0?0:1;
		canInfo_fsmc[index].errorFlag = (tmp&0x40)>0?1:0;
		canInfo_fsmc[index].txErrCnt = readFSMC(base_addr+SJA_TXERR);
		canInfo_fsmc[index].rxErrCnt = readFSMC(base_addr+SJA_RXERR);
		return &canInfo_fsmc[index];
	}
	return 0;
}

/** enable/disable self-testing */
void enableSelfTest_fsmc(u32 index, u8 enabled) {
	u32 base_addr;
	u8 tmp;
	if(index<CAN_FSMC_CNT) {
		base_addr = 0x2000+(index<<8);//get base address 0x2000 0x2100 ...
		writeFSMC(base_addr+SJA_MOD,0x01); //enter reset mode
		tmp = readFSMC(base_addr+SJA_MOD)&0x01;
		if(tmp  ==  0x01) {
			if(enabled) {
				//enter self-test mode
				writeFSMC(base_addr+SJA_IER,0);//no interrupt in test mode
				writeFSMC(base_addr+SJA_MOD,0x0C);
				checkReg(base_addr+SJA_MOD,0x0C,10);//check mode
				canInfo_fsmc[index].testFlag = 1;
			}
			else {
				//normal mode
				writeFSMC(base_addr+SJA_IER,0X0D);
				checkReg(base_addr+SJA_IER,0x0D,10);//check mode
				writeFSMC(base_addr+SJA_MOD,0x08);
				checkReg(base_addr+SJA_MOD,0x08,10);//check mode
				canInfo_fsmc[index].testFlag = 0;
			}
		}
	}
}

/** test CAN */
u8 testCAN_fsmc(u32 index) {
	u8 i,tmp;
	u32 base_addr = 0x2000+(index<<8);//get base address
	CAN_BLOCK block;
	if((index<CAN_FSMC_CNT) && canInfo_fsmc[index].testFlag) {
		//prepare testing block
		block.canId = index;
		block.addr = 0x0A;
		block.len  = 8;
		block.remoteFlag = 0;
		block.extendFlag = 0;
		for(i=0;i<8;i++) {
			block.canData[i] = i;
		}
		//clear rx buffer
		while(1) {
			tmp = readFSMC(base_addr+SJA_SR);
			if(tmp&0x01) {
				writeFSMC(base_addr+SJA_CMR,0x04);
			}
			else {
				break;
			}
		}
		//send
		if((waitCANAvail_fsmc(index)==0) && (sendCanMsg_fsmc(&block)==0)) {
			for(i=0;i<20;i++) {
				tmp = readFSMC(base_addr+SJA_SR);
				if(tmp&0x01) {
					break;
				}
				else {
					delay_us(1000);
				}
			}
			if(i<20) {
				//receive
				if(readCanMsg_fsmc(index,&block)==0) {
					for(i=0;i<8;i++) {
						if(block.canData[i]!=i) {
							break;
						}
					}
					if(i>=8) {
						return 0;
					}
				}
			}
		}
	}
	return 1;
}

/** re-initialize channel */
void reInitCAN_fsmc(u32 index) {
	u32 base_addr = 0x2000+(index<<8);//get base address 0x2000 0x2100 ...
	uint8_t temp_data1;
	if(index<CAN_FSMC_CNT) {
		//reset
		resetCAN_fsmc(index);
		//initialize
		writeFSMC(base_addr+SJA_MOD,0x01); //enter reset mode
		temp_data1 = readFSMC(base_addr+SJA_MOD);
		temp_data1 = temp_data1 & 0x01;
		if(temp_data1  ==  0x01) {
			//set PeliCAN mode
			writeFSMC(base_addr+SJA_CDR,0XC0);
			//timing: 250K
			writeFSMC(base_addr+SJA_BTR0,0X83);
			writeFSMC(base_addr+SJA_BTR1,0XA3);
			//output mode: normal, pull-push
			writeFSMC(base_addr+SJA_OCR,0X1A);
			//no filter
			writeFSMC(base_addr+SJA_AMR0,0XFF);
			writeFSMC(base_addr+SJA_AMR1,0XFF);
			writeFSMC(base_addr+SJA_AMR2,0XFF);
			writeFSMC(base_addr+SJA_AMR3,0XFF);
			//set error threshold
			writeFSMC(base_addr+SJA_EMLR,128);
			//interrupt: overrun, error, and receive
			writeFSMC(base_addr+SJA_IER,0X0D);
		}
		writeFSMC(base_addr+SJA_MOD,0X08);//NORMAL MODE
		checkReg(base_addr+SJA_MOD,0x08,10);//check mode
		//update info
		canInfo_fsmc[index].testFlag = 0;
		canInfo_fsmc[index].resetCnt++;
	}
}

void handleCANInterrupt(void) {
	u32 index;
	u8 ir,tmp,sr;
	u32 base_addr;
	CAN_BLOCK block;
	u8 flag = (cntRxBuff_fsmc>10)?1:0;
	for(index=0;index<CAN_FSMC_CNT;index++) {
		base_addr = 0x2000+(index<<8);//get base address
		ir = readFSMC(base_addr+SJA_IR);//get interrupts
		sr = readFSMC(base_addr+SJA_SR);//get status
		if(ir&0x08) {
			//overrun
			writeFSMC(base_addr+SJA_CMR,0x08);//clear overrun
		}
		if(ir&0x01) {
			//receive
			while(readCanMsg_fsmc(index,&block)==0) {
				rxBuff_fsmc[iWriteRxBuff_fsmc] = block;
				//change index
				iWriteRxBuff_fsmc = (iWriteRxBuff_fsmc+1)%MAX_RX_CAN_BLCK;
				cntRxBuff_fsmc++;
			}
		}
		if((ir&0x04) || (sr&0xc0)) {
			//error
			tmp = readFSMC(base_addr+SJA_SR);
			if(tmp&0x80) {
				//bus off
				publishMsg(MSG_CANFSMC_BUSOFF,(int)index);
			}
			else if(tmp&0x40) {
				//error
				canInfo_fsmc[index].txErrCnt = readFSMC(base_addr+SJA_TXERR);
				canInfo_fsmc[index].rxErrCnt = readFSMC(base_addr+SJA_RXERR);
				writeFSMC(base_addr+SJA_MOD,0x01); //enter reset mode
				tmp = readFSMC(base_addr+SJA_MOD) & 0x01;
				if(tmp  ==  0x01) {
					//clear error count
					writeFSMC(base_addr+SJA_TXERR,0);
					writeFSMC(base_addr+SJA_RXERR,0);
				}
				writeFSMC(base_addr+SJA_MOD,0x08);//NORMAL MODE
				checkReg(base_addr+SJA_MOD,0x08,10);//check mode
				//update info
				canInfo_fsmc[index].testFlag = 0;
			}
		}
	}
	
	//check rx buff
	if((flag==0) && (cntRxBuff_fsmc>10)) {
		publishMsg(MSG_CANFSMC_AVAILABLE,0);
	}
}

//EXTI2 handler
void EXTI2_IRQHandler(void) {
	if(EXTI_GetITStatus(EXTI_Line2)==SET) {
		//handleCANInterrupt();
		publishMsg(MSG_CANFSMC_INTERRUPT,0);
	}
	EXTI_ClearITPendingBit(EXTI_Line2);
}

void testCAN(void) {
	u32 tmp32;
	u8 tmp8;
	for(tmp32=0;tmp32<CAN_FSMC_CNT;tmp32++) {
		printf("Test Channel %u...\r\n",tmp32);
		enableSelfTest_fsmc(tmp32,1);
		tmp8 = testCAN_fsmc(tmp32);
		enableSelfTest_fsmc(tmp32,0);
		if(tmp8==0) {
			openLED((u8)tmp32);
			printf("Test Succeed!\r\n");
		}
		else {
			closeLED((u8)tmp32);
			printf("Test Failed\r\n");
		}
	}
}
