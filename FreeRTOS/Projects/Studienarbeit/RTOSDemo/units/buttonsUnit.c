/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "croutine.h"

#include "OLEDDisplay/oledDisplay.h"

#include "buttonsUnit.h"
#include "buttons.h"
#include "unit.h"

static void vButtonPress(tButton button);
static tBoolean vBtnUnitNewJob(tUnitJob Job);

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
	tUnitJob xJob;
}xValue;
}tBtnUnitQueueItem;

static xQueueHandle xQueue = NULL;
static tUnit * xBtnUnit;
static tUnitCapability * ButtonStateCapability, * SendImmediateCapability;
static char tmpStr[200]; // TODO delete me
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
	bUnitAddCapability(xBtnUnit, (tUnitCapability){"ack", NULL});
	ButtonStateCapability = bUnitAddCapability(xBtnUnit, (tUnitCapability){"ButtonState", NULL});

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
					sprintf(xQueueItem.xValue.xJob.data, "BtnPress:%d", xQueueItem.xValue.xButton);
					xQueueItem.xValue.xJob.xCapability = ButtonStateCapability;
					xQueueItem.xValue.xJob.uid = sendoutImmediateUid;
					bUnitSend(xBtnUnit, xQueueItem.xValue.xJob);
				}
				break;
			case JOB:
				strcpy(tmpStr, "Job: ");
				strcat(tmpStr, xQueueItem.xValue.xJob.xCapability->Name);
				vOledDbg(tmpStr);

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
static eUnitJobState vBtnUnitNewJob(tUnitJob Job)
{
	int i;
	eUnitJobState ret = 0;
	tUnitCapability * pxDstCapability;
	tBtnUnitQueueItem item;

	// TODO Validate job
	if(Job.xCapability == ButtonStateCapability)
	{
		for(i=0;i<Job.parametersCnt;i++)
		{
			if( (strcmp(Job.parameter[i].Name, "sendonarrival") == 0)
				&& (strcmp(Job.parameter[i].Value, "yes") == 0) )
			{
				sendoutImmediate = true;
				sendoutImmediateUid = Job.uid;
				ret |= JOB_STORE;
			}
		}
	}

	// format
	item.eType = JOB;
	item.xValue.xJob = Job;

	// enqueue
	if(xQueue != NULL)
		xQueueSend(xQueue, &item, portMAX_DELAY);

	return ret | JOB_ACK;
}
