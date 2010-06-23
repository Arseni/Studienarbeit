/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "croutine.h"

#include "OLEDDisplay/oledDisplay.h"

#include "buttonsUnit.h"
#include "buttons.h"
#include "unit.h"

static void vButtonPress(tButton button);
static void vBtnUnitNewJob(tUnitJob Job);

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

void vBtnUnitTask(void * pvParameters)
{

	tBtnUnitQueueItem xQueueItem;

	// init buttons driver
	// make sure the button task is running !!!
	bButtonRegisterCallback(vButtonPress);

	// init unit
	xBtnUnit = xUnitCreate("Buttons", vBtnUnitNewJob);
	ButtonStateCapability = bUnitAddCapability(xBtnUnit, (tUnitCapability){"ButtonState", NULL});
	SendImmediateCapability = bUnitAddCapability(xBtnUnit, (tUnitCapability){"SendImmediate", NULL});

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
					sprintf(tmpStr, "BtnPress:%d", xQueueItem.xValue.xButton);
					bUnitSend(xBtnUnit, ButtonStateCapability, tmpStr);
				}
				break;
			case JOB:
				strcpy(tmpStr, "Job: ");
				strcat(tmpStr, xQueueItem.xValue.xJob.xCapability.Type);
				vOledDbg(tmpStr);
				if(strcmp(xQueueItem.xValue.xJob.xCapability.Type, "SendImmediate") == 0)
				{
					sendoutImmediate = true;
				}
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
static void vBtnUnitNewJob(tUnitJob Job)
{
	int i;
	tUnitCapability * pxDstCapability;
	tBtnUnitQueueItem item;

	// Validate job
	for(i=0; i<UNIT_MAX_CAPABILITIES; i++)
	{
		if(!UNIT_CAPABILITIES_CMP(xBtnUnit->xCapabilities[i], Job.xCapability))
		{

			pxDstCapability = &(xBtnUnit->xCapabilities[i]);
			goto parameters_valid;
		}
	}
	return;

	parameters_valid:

	// format
	item.eType = JOB;
	item.xValue.xJob = Job;

	// enqueue
	if(xQueue != NULL)
		xQueueSend(xQueue, &item, portMAX_DELAY);

}
