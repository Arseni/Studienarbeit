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
					static char outTree[800];
					char * next, * this;

					next = muXMLCreateElement(outTree, xBtnUnit->Name);

					// if an error accured, add error attribute to unit or something
					this = next;
					muXMLCreateElement(next, ButtonStateCapability->Name);
					muXMLUpdateAttribute((struct muXMLTreeElement *)this, "cause", "press");
					next = muXMLAddElement((struct muXMLTreeElement *)outTree, (struct muXMLTreeElement *)this);

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

					next = muXMLUpdateData((struct muXMLTreeElement *)this, next);
					bUnitSend((struct muXMLTreeElement *)outTree, sendoutImmediateUid);//xBtnUnit, xQueueItem.xValue.xJob);
				}
				break;
			case JOB:
				strcpy(tmpStr, "Job: ");
				//strcat(tmpStr, xQueueItem.xValue.xJob.xCapability->Name);
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
static eUnitJobState vBtnUnitNewJob(struct muXMLTreeElement * job, int uid)
{
	int i;
	eUnitJobState ret = 0;
	if(strcmp(muXMLGetAttributeByName(job->SubElements, "sendonarrival"), "yes")==0)
	{
		ret = JOB_STORE | JOB_ACK;
		sendoutImmediateUid = uid;
		sendoutImmediate = true;
	}
	/*
	tUnitCapability * pxDstCapability;
	tBtnUnitQueueItem item;

	// TODO Validate job
	if(Job.xCapability == ButtonStateCapability)
	{
		if(strcmp(muXMLGetAttributeByName("sendonarrival"), "yes") == 0)
		{
			sendoutImmediate = true;
			sendoutImmediateUid = Job.uid;
			ret |= JOB_STORE;
		}
		for(i=0;i<Job.parametersCnt;i++)
		{
			if( (strcmp(Job.parameter[i].Name, "sendonarrival") == 0)
				&& (strcmp(Job.parameter[i].Value, "yes") == 0) )
			{

			}
		}
	}

	// format
	item.eType = JOB;
	item.xValue.xJob = Job;

	// enqueue
	if(xQueue != NULL)
		xQueueSend(xQueue, &item, portMAX_DELAY);
	*/
	return ret;
}
