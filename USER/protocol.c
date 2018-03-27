#include "protocol.h"
#include "can_fsmc.h"
#include "fsmc.h"
#include "debug_led.h"
#include "stdio.h"
#include "delay.h"
#include "rs422.h"

#define DEVICE_CHL_NUM 20

#pragma pack(push)
#pragma pack(1)
typedef struct str_report_422 {
    u8  header;/**< 0x95 */
    u16 index;
    u8  data[4];
    u8  cs;
}REPORT_422;
#pragma pack(pop)

u8 device_num[DEVICE_CHL_NUM] = {
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,1,1,1,1
};
u8 report_len[DEVICE_CHL_NUM] = {
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,8,8,6,10
};
u32 start_index[DEVICE_CHL_NUM];

REPORT_422 reports[512];
u32 totalCnt;
u8 printFlag;

/** setup protocol */
void initProtocol(void) {
    u8 i,j;
    totalCnt = 0;
    for(i=0;i<DEVICE_CHL_NUM;i++) {
        start_index[i] = totalCnt;
        totalCnt += (report_len[i]+2)/4;
    }
    for(i=0;i<totalCnt;i++) {
        reports[i].header = 0x95;
        reports[i].index  = i;
        for(j=0;j<4;j++) {
            reports[i].data[j] = 0;
        }
    }
    printFlag = 0;
}

/** handle CAN block */
void handleCANBlock(CAN_BLOCK *block) {
    REPORT_422 *report;
    u8 device_id,frame_index;
    
    //control LED
    openLED(2);
    
    //check channel
    if(block->canId>=DEVICE_CHL_NUM) {
        return;
    }
    //check remote flag
    else if(block->remoteFlag) {
        return;
    }
    //check extend flag
    else if(block->extendFlag) {
        return;
    }

    //fetch basic info
    device_id = (u8)(block->addr&0xff);
    frame_index = device_id&0x0f;
    device_id = (device_id>>4)-1;
    report = reports+start_index[block->canId];
    //check device id
    if(device_id>=device_num[block->canId]) {
        return;
    }

    if(block->canId<18) {
        //case 1: one device has 2 voltages
        if((frame_index==0) && (block->len>=8)) {
            //first report
            report += device_id*2;
            report->data[1] = block->canData[0];
            report->data[0] = block->canData[1];
            report->data[3] = block->canData[2];
            report->data[2] = block->canData[3];
            //second report
            report++;
            report->data[1] = block->canData[4];
            report->data[0] = block->canData[5];
            report->data[3] = block->canData[6];
            report->data[2] = block->canData[7];
        }
    }
    else if(block->canId<19) {
        //case 2: 1 device has 3 voltages
        if((frame_index==0) && (block->len>=6)) {
            //first report
            report += device_id*2;
            report->data[1] = block->canData[0];
            report->data[0] = block->canData[1];
            report->data[3] = block->canData[2];
            report->data[2] = block->canData[3];
            //second report
            report++;
            report->data[1] = block->canData[4];
            report->data[0] = block->canData[5];
        }
    }
    else {
        //case 3: 1 device has 5 voltages
        if((frame_index==0) && (block->len>=8)) {
            //first report
            report += device_id*3;
            report->data[1] = block->canData[0];
            report->data[0] = block->canData[1];
            report->data[3] = block->canData[2];
            report->data[2] = block->canData[3];
            //second report
            report++;
            report->data[1] = block->canData[4];
            report->data[0] = block->canData[5];
            report->data[3] = block->canData[6];
            report->data[2] = block->canData[7];
        }
        else if((frame_index==1) && (block->len>=2)) {
            //third report
            report += device_id*3+2;
            report->data[1] = block->canData[0];
            report->data[0] = block->canData[1];
        }
    }
}

/** query CAN device */
void queryCANDevice(u8 index) {
    CAN_BLOCK block;
    u8 i,flag = 0;
    //prepare block
    block.addr = (index+1)<<4;
    block.len  = 0;
    for(i=0;i<8;i++) {
        block.canData[i] = 0;
    }
    block.remoteFlag = 1;
    block.extendFlag = 0;
    //check every channel
    for(i=0;i<DEVICE_CHL_NUM;i++) {
        if(index<device_num[i]) {
            block.canId = i;
            if((waitCANAvail_fsmc(i)==0) && (sendCanMsg_fsmc(&block)==0)) {
                flag = 1;
            }
        }
    }
    //control LED
    if(flag) {
        openLED(1);
    }
    else {
        closeLED(1);
    }
}

/** report gathered information */
void reportInfo(void) {
    u8 *report;
    u8 i,j;
    u8 cs;
    u16 val;
    float vol;
    //reset LED
    closeLED(1); closeLED(2);
    //prepare report
    for(i=0;i<totalCnt;i++) {
        report = (u8*)&reports[i];
        //calculate check sum
        cs = 0;
        for(j=0;j<7;j++) {
            cs += report[j];
        }
        reports[i].cs = cs+1;
    }
    //print report
    if(printFlag) {
        for(i=0;i<totalCnt;i++) {
            printf("Report of Index %u:\r\n",reports[i].index);
            if(printFlag==2) {
                report = (u8*)&reports[i];
                for(j=0;j<8;j++) {
                    printf("%02X ",report[j]);
                }
            }
            else {
                for(j=0;j<2;j++) {
                    val = reports[i].data[j*2+1];
                    val = (val<<8)+reports[i].data[j*2];
                    vol = val*0.1;//get voltage
                    printf("%5.1f\t",vol);
                }
            }
            printf("\r\n");
        }
        printFlag = 0;
    }
    //report
    if(waitAvailable_422()) {
        abortTask_422();
    }
    startTxTask_422((u8*)reports,totalCnt*8);
    //clear report for next period
    for(i=0;i<totalCnt;i++) {
        for(j=0;j<4;j++) {
            reports[i].data[j] = 0;
        }
    }
}

/** print next report */
void printReport(u8 mode) {
    if(mode==0) {
        printFlag = 1;
    }
    else {
        printFlag = 2;
    }
}
