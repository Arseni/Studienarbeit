/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"

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

static xQueueHandle xQueue;
static tUnit * xBtnUnit;

void vBtnUnitTask(void * pvParameters)
{

	tBtnUnitQueueItem xQueueItem;

	// init buttons driver
	// make sure the button task is running !!!
	bButtonRegisterCallback(vButtonPress);

	// init unit
	xBtnUnit = xUnitCreate("Buttons", vBtnUnitNewJob);
	bUnitAddCapability(xBtnUnit, (tUnitCapability){"ButtonState", NULL});

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
				// a button has been pressed... do something
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

	xQueueSend(xQueue, &item, portMAX_DELAY);
}

/**
 * This is a callback function to deal with an incomong job
 * It will run in a foreign task so make sure to sync the job
 * if you want to deal with it in your unit task
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
	// assert protocol frame
	// TODO : uhm check if ack is even valid !?
	if(Job.bAck)
	{
		bUnitSend(xBtnUnit, pxDstCapability, (tUnitValue){"ACK"});
	}

	// queue job
	item.eType = JOB;
	item.xValue.xJob = Job;
	xQueueSend(xQueue, &item, portMAX_DELAY);

}