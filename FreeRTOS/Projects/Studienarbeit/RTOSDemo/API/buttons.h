
#ifndef BUTTONS_H
#define BUTTONS_H

#include "hw_types.h"

#define BUTTON_QUEUE_SIZE	10

typedef enum
{
	BUTTON_NONE=0,
	BUTTON_UP=1,
	BUTTON_DOWN=2,
	BUTTON_LEFT=4,
	BUTTON_RIGHT=8,
	BUTTON_SEL=16
}tButton;

typedef void (* tButtonCallback) (tButton btn);

tBoolean bButtonRegisterCallback(tButtonCallback btnCb);

tButton xButtonIsPressed(void);
void vButtonTask(void * pvParameters);

#endif
