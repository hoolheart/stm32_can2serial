#include "stdio.h"
#include "stm32f4xx_conf.h"
#include "console.h"
#include "msg_interface.h"

//////////////////////////////////////////////////////////////////
//use following code to support printf without need of MicroLIB	  
#if 1
//redefine fputc to enable printf
int fputc(int ch, FILE *f)
{ 	
	while((USART3->SR&0X40)==0);//wait for idle state
	USART3->DR = (u8) ch;      
	return ch;
}
#endif

//received command buffer
#define CMD_MAX_CNT 8
CONSOLE_CMD cmds[CMD_MAX_CNT];
unsigned short iCmdRead=0, iCmdSave=0, cmdCnt=0;

//receive buffer
unsigned char USART3_RX_BUF[COMMAND_MAX_SIZE];
//receive state
//bit15 -	flag of end
//bit14 -	flag of 0x0d
//bit13~0 -	received count
unsigned short USART3_RX_STA = 0;

/** initialize console */
void initConsole(int baud)
{
	// prepare configuration
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE); //enable GPIOC clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//enable USART3 clock
 
	//set GPIO mapping
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_USART3); //GPIOC10->USART3
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_USART3); //GPIOC11->USART3
	
	//USART3 IO setting
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11; //GPIOC10 & GPIOC11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//alternative mode
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//50MHz speed
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //PP output
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //pull up
	GPIO_Init(GPIOC,&GPIO_InitStructure); //Initialize PB10 and PB11

   //USART3 initialization
	USART_InitStructure.USART_BaudRate = baud;//baudrate
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//8bit len
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//1bit stop
	USART_InitStructure.USART_Parity = USART_Parity_No;//no check
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//no flow control
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//rx & tx
	USART_Init(USART3, &USART_InitStructure); //initialization
	
	USART_Cmd(USART3, ENABLE);  //enable
	
	USART_ClearFlag(USART3, USART_FLAG_TC);
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//enable interrupt

	//Usart3 NVIC config
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;//define interrupt channel
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//priority: 3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ channel enabled
	NVIC_Init(&NVIC_InitStructure);	//initialize NVIC
	
	//initialize command list
	iCmdRead = 0; iCmdSave = 0;
	cmdCnt = 0;
}

/** print message */
void outputConsole(char msg[],int len)
{
	int i=0;
	for(;i<len;i++) {
		while((USART3->SR&0X40)==0);//wait for idle
		USART3->DR = (u8)msg[i];
	}
}

/** whether console received a command */
unsigned short getCommandNum()
{
	return cmdCnt;
}

/** get command */
CONSOLE_CMD * getCommand()
{
	CONSOLE_CMD * pCmd = NULL;
	if(cmdCnt>0) {
		pCmd = &cmds[iCmdRead];//get command
		iCmdRead = (iCmdRead+1)%CMD_MAX_CNT;//increase index
		cmdCnt--;//decrease count
	}
	return pCmd;
}

/** interrupt handler of USART3 */
void USART3_IRQHandler(void)
{
	unsigned char Res;
	int i;
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {//check state
		Res =USART_ReceiveData(USART3);//(USART1->DR);	//get data
		if((USART3_RX_STA&0x8000)==0) {//not end
			if(USART3_RX_STA&0x4000) {//ox0d received
				if(Res!=0x0a) {
					USART3_RX_STA=0;//error, start over
				}
				else {
					USART3_RX_STA|=0x8000;	//finished
					//record into command buffer
					if(cmdCnt<CMD_MAX_CNT) {
						cmds[iCmdSave].len = (USART3_RX_STA&0x3fff);//length
						for(i=0;i<(USART3_RX_STA&0x3fff);i++) {
							cmds[iCmdSave].cmd[i] = USART3_RX_BUF[i];//copy data
						}
						iCmdSave = (iCmdSave+1)%CMD_MAX_CNT;//increase index
						cmdCnt++;//increase count
						publishMsg(MSG_CONSOLE_COMMAND,0);//publish message
						USART3_RX_STA = 0;//start over
					}
				}
			}
			else {//content
				if(Res==0x0d) {
					USART3_RX_STA|=0x4000;//content finished
				}
				else if((USART3_RX_STA&0X3FFF)<COMMAND_MAX_SIZE) {
					USART3_RX_BUF[USART3_RX_STA&0X3FFF]=Res;//record data
					USART3_RX_STA++;	  
				}		 
			}
		}   		 
  }
	//USART_ClearITPendingBit(USART3,USART_IT_RXNE);
}
