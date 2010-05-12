#ifndef LCD_MESSAGE_H
#define LCD_MESSAGE_H

#define LCD_MESSAGE_MAX_LENGTH 25

/* Constants used when writing strings to the display. */
#define mainCHARACTER_HEIGHT				( 9 )
#define mainMAX_ROWS_128					( mainCHARACTER_HEIGHT * 14 )
#define mainMAX_ROWS_96						( mainCHARACTER_HEIGHT * 10 )
#define mainMAX_ROWS_64						( mainCHARACTER_HEIGHT * 7 )
#define mainFULL_SCALE						( 15 )
#define ulSSI_FREQUENCY						( 3500000UL )

#include "portmacro.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <string.h>

typedef struct
{
	char pcMessage[LCD_MESSAGE_MAX_LENGTH];
} xOLEDMessage;

extern xQueueHandle xOLEDQueue;

void vOLEDTask( void *pvParameters );

static inline void vOledDbg1(char * s, int i)
{
	if(xOLEDQueue == NULL)
			return;
	xOLEDMessage msg;
	sprintf(msg.pcMessage, "%s : %d", s, i);
	xQueueSend(xOLEDQueue, &msg, portMAX_DELAY);
}
static inline void vOledDbg(char * s)
{
	if(xOLEDQueue == NULL)
		return;
	xOLEDMessage msg;
	strcpy(msg.pcMessage, s);
	xQueueSend(xOLEDQueue, &msg, portMAX_DELAY);
}

#endif /* LCD_MESSAGE_H */
