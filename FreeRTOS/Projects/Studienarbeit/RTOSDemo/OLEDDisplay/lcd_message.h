#ifndef LCD_MESSAGE_H
#define LCD_MESSAGE_H

#define LCD_MESSAGE_MAX_LENGTH 25

#include "portmacro.h"
#include "FreeRTOS.h"
#include "queue.h"

typedef struct
{
	char pcMessage[LCD_MESSAGE_MAX_LENGTH];
} xOLEDMessage;

extern xQueueHandle xOLEDQueue;

static inline LCDDBG(char * s, int i)
{
	xOLEDMessage msg;
	sprintf(msg.pcMessage, "%s : %d", s, i);
	xQueueSend(xOLEDQueue, &msg, portMAX_DELAY);
}

#endif /* LCD_MESSAGE_H */
