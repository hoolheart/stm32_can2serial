/*
 * debug_led.h
 *
 *  Created on: 2016-9-9
 *      Author: hzhou
 */

#ifndef DEBUG_LED_H_
#define DEBUG_LED_H_

/** initialize debug LED interface */
void initDebugLED(void);
/** open LED */
void openLED(unsigned char index);
/** close LED */
void closeLED(unsigned char index);
/** open all LEDs */
void openAllLED(void);
/** close all LEDs */
void closeAllLED(void);

#endif /* DEBUG_LED_H_ */
