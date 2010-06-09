
#include "freertos.h"
#include "task.h"

/* Library includes. */
#include "hw_types.h"
#include "gpio.h"
#include "hw_memmap.h"
#include "portmacro.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <string.h>

/* drivers include */
#include "gpio.h"
#include "sysctl.h"

/* API include */
#include "buttons.h"
#include "OLEDDisplay/oledDisplay.h"

/* Presets */
#define BUTTON_MAX_CALLBACKS	10
#define BUTTON_ARROW_PINS		(GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3)
#define BUTTON_SEL_PIN			GPIO_PIN_1
#define BUTTON_ARROW_PORT_BASE	GPIO_PORTE_BASE
#define BUTTON_SEL_PORT_BASE	GPIO_PORTF_BASE

extern xQueueHandle xOLEDQueue;

/* ---------------------- private library --------------------- */
static tBoolean isInitialized = false;
static tButtonCallback callbackCollection[BUTTON_MAX_CALLBACKS];
static xQueueHandle xButtonQueue;

static void buttonArrowsPressed(unsigned char ucPins)
{
	// We have not woken a task at the start of the ISR.
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	tButton btnPressed = ucPins & BUTTON_ARROW_PINS;

	if(btnPressed)
		xQueueSendFromISR(xButtonQueue, &btnPressed, &xHigherPriorityTaskWoken);

	// Now the buffer is empty we can switch context if necessary.
	if( xHigherPriorityTaskWoken )
	{
		// Actual macro used here is port specific.
		taskYIELD();
	}
}
static void buttonSelPressed(unsigned char ucPins)
{
	// We have not woken a task at the start of the ISR.
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	tButton btnPressed = ucPins & BUTTON_SEL_PIN;
	if(btnPressed)
	{
		btnPressed = BUTTON_SEL;
		xQueueSendFromISR(xButtonQueue, &btnPressed, &xHigherPriorityTaskWoken);
	}

	// Now the buffer is empty we can switch context if necessary.
	if( xHigherPriorityTaskWoken )
	{
		// Actual macro used here is port specific.
		taskYIELD();
	}
}


static void Init(void)
{
	if(isInitialized)
		return;

	int i;
	for(i=0; i<BUTTON_MAX_CALLBACKS; i++)
		callbackCollection[i] = NULL;

	// enable PORTE clock
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	// set Button pins as input
	GPIODirModeSet(BUTTON_ARROW_PORT_BASE, BUTTON_ARROW_PINS, GPIO_DIR_MODE_IN);
	GPIODirModeSet(BUTTON_SEL_PORT_BASE, BUTTON_SEL_PIN, GPIO_DIR_MODE_IN);

	// Set the buttons to weak pullup
	GPIOPadConfigSet( BUTTON_ARROW_PORT_BASE, BUTTON_ARROW_PINS, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU );
	GPIOPadConfigSet( BUTTON_SEL_PORT_BASE, BUTTON_SEL_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU );

	// Enable and register interrupt
	GPIOIntTypeSet(BUTTON_ARROW_PORT_BASE, BUTTON_ARROW_PINS, GPIO_FALLING_EDGE);
	GPIOPinIntEnable(BUTTON_ARROW_PORT_BASE, BUTTON_ARROW_PINS);
	GPIOPortIntRegister(BUTTON_ARROW_PORT_BASE, BUTTON_ARROW_PINS, buttonArrowsPressed);

	GPIOIntTypeSet(BUTTON_SEL_PORT_BASE, BUTTON_SEL_PIN, GPIO_FALLING_EDGE);
	GPIOPinIntEnable(BUTTON_SEL_PORT_BASE, BUTTON_SEL_PIN);
	GPIOPortIntRegister(BUTTON_SEL_PORT_BASE, BUTTON_SEL_PIN, buttonSelPressed);

	isInitialized = true;
}


/* ---------------------- public  library --------------------- */

void vButtonTask(void * pvParameters)
{
	tButton btnPressed;
	xOLEDMessage msg;
	int i;

	Init();
	xButtonQueue = xQueueCreate( BUTTON_QUEUE_SIZE, sizeof( tButton ) );
	for(;;)
	{
		xQueueReceive( xButtonQueue, &btnPressed, portMAX_DELAY );
		for(i=0; i<BUTTON_MAX_CALLBACKS; i++)
		{
			if(callbackCollection[i] != NULL)
				callbackCollection[i](btnPressed);
		}
	}
}

tBoolean bButtonRegisterCallback(tButtonCallback btnCb)
{
	int i=0;

	Init();

	for(i=0; i<BUTTON_MAX_CALLBACKS; i++)
	{
		if(callbackCollection[i] == NULL)
		{
			callbackCollection[i] = btnCb;
			return true;
		}
	}
	return false;
}

tButton xButtonIsPressed(void)
{
	Init();

	tButton ret = 0;
	portBASE_TYPE pinState;
	pinState = GPIOPinRead(BUTTON_ARROW_PORT_BASE, BUTTON_ARROW_PINS);
	ret = (tButton)(~pinState & BUTTON_ARROW_PINS);

	if(!GPIOPinRead(BUTTON_SEL_PORT_BASE, BUTTON_SEL_PIN))
		ret |= BUTTON_SEL;

	return ret;
}

