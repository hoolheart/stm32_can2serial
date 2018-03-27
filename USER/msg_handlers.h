#ifndef __MSG_HANDLERS_H__
#define __MSG_HANDLERS_H__

/**
 * This file declares the functions to handle messages
 */

/** on command received from console */
void onConsoleCommand(int para);

/** on CAN bus off */
void onCANBusOff_fsmc(int para);

/** on CAN buffer available */
void onCANBuffer_fsmc(void);

/** on RS422 task finished */
void onRS422Finished(void);

/** on RS422 task aborted */
void onRS422Aborted(void);

/** on work tick */
void onWorkTick(int para);

#endif
