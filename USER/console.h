#ifndef __CONSOLE_H__
#define __CONSOLE_H__

/**
 * This file builds up a console through uart 3
 */

#define COMMAND_MAX_SIZE 20

/** data structure of command */
typedef struct _CONSOLE_CMD {
	unsigned short len;
	unsigned char  cmd[COMMAND_MAX_SIZE];
}CONSOLE_CMD;

/** initialize console */
void initConsole(int baud);
/** print message */
void outputConsole(char msg[],int len);
/** whether console received a command */
unsigned short getCommandNum(void);
/** get command */
CONSOLE_CMD * getCommand(void);

#endif //__CONSOLE_H__
