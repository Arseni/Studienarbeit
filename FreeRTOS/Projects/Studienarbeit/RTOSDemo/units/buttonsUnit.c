/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "croutine.h"

#include "OLEDDisplay/oledDisplay.h"

#include "muXML/muXMLAccess.h"

#include "buttonsUnit.h"
#include "buttons.h"
#include "unit.h"

static void vButtonPress(tButton button);
static tBoolean vBtnUnitNewJob(struct muXMLTreeElement * Job, int uid);

typedef struct
{
enum eType
{
	NONE	= (0),
	BUTTON 	= (1<<0),
	JOB 	= (1<<1)
}eType;
union xValue
{
	tButton xButton;
}xValue;
}tBtnUnitQueueItem;

static xQueueHandle xQueue = NULL;
static tUnit * xBtnUnit;
//static tUnitCapability * ButtonStateCapability, * SendImmediateCapability;
static tBoolean sendoutImmediate = false;
int sendoutImmediateUid = 0;

void vBtnUnitTask(void * pvParameters)
{
	int i;
	tBtnUnitQueueItem xQueueItem;

	// init buttons driver
	// make sure the button task is running !!!
	bButtonRegisterCallback(vButtonPress);

	// init unit
	xBtnUnit = xUnitCreate("Buttons", vBtnUnitNewJob);

	// init internal
	xQueue = xQueueCreate(BTN_UNIT_MAX_QUEUE_LEN, sizeof(tBtnUnitQueueItem));

	//periodic
	for(;;)
	{
		if(xQueueReceive(xQueue, &xQueueItem, portMAX_DELAY))
		{
			switch(xQueueItem.eType)
			{
			case BUTTON:
				vOledDbg1("Button", xQueueItem.xValue.xButton);
				// a button has been pressed... do something
				if(sendoutImmediate)
				{
					static struct muXMLTreeElement element;
					static char dataBuff[6];
					char * next;

					// if an error accured, add error attribute to unit or something
					next = muXMLCreateElement(&element, "ButtonState");// ButtonStateCapability->Name);
					muXMLUpdateAttribute(&element, "cause", "press");

					*next = 0;
					if(xQueueItem.xValue.xButton & BUTTON_LEFT)
						strcat(next, "left");
					if(xQueueItem.xValue.xButton & BUTTON_RIGHT)
						strcat(next, "right");
					if(xQueueItem.xValue.xButton & BUTTON_UP)
						strcat(next, "up");
					if(xQueueItem.xValue.xButton & BUTTON_DOWN)
						strcat(next, "down");
					if(xQueueItem.xValue.xButton & BUTTON_SEL)
						strcat(next, "sel");

					next = muXMLUpdateData(&element, next);
					bUnitSend(&element, sendoutImmediateUid);//xBtnUnit, xQueueItem.xValue.xJob);
				}
				break;
			case JOB:
				// a job arrived, handle it
				break;
			}
		}
	}
}

static void vButtonPress(tButton button)
{
	tBtnUnitQueueItem item;
	item.eType = BUTTON;
	item.xValue.xButton = button;

	if(xQueue != NULL)
		xQueueSend(xQueue, &item, portMAX_DELAY);
}

/**
 * Use this function to validate, format and enqueue the Job only!
 */
static eUnitJobState vBtnUnitNewJob(struct muXMLTreeElement * job, int uid)
{
	int i;
	eUnitJobState ret = 0;
	if(strcmp(muXMLGetAttributeByName(job, "onchange"), "yes")==0)
	{
		ret = JOB_STORE | JOB_ACK;
		sendoutImmediateUid = uid;
		sendoutImmediate = true;
	}
	else
	{
		vOledDbg("job");
		ret = JOB_ACK;
	}
	return ret;
}
