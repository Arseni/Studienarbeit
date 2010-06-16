#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "croutine.h"

#include "unit.h"
#include <string.h>

#include "OLEDDisplay/oledDisplay.h"


struct tUnitHandler
{
	tUnit xUnit;
	tBoolean bInUse;
};

struct tUnitJobHandler
{
	tUnitJob xJob;
	xQueueHandle xResponseQueue;
	int SeqNum;
	int SecIntervall;
};



static struct tUnitHandler xUnits[UNIT_MAX_GLOBAL_UNITS];
static struct tUnitJobHandler xJobs[UNIT_MAX_GLOBAL_JOBS_PARALLEL];
static xQueueHandle xJobQueue = NULL, xRxQueue = NULL;
static xSemaphoreHandle xSmphrDataPresent = NULL, xSmphrSendComplete = NULL;
static unsigned char txBuffer[200];
static unsigned int txBufferLen = 0;
static tBoolean newTxData = false;
static tBoolean syncRxFlag = false;

static tBoolean InitXmit()
{
	static int uid=0;
	int i, rxLen;
	uip_ipaddr_t host;
	struct tData sender;


	uip_gethostaddr(host);

	strcpy(txBuffer, "<epm uid=\"0\" ack=\"yes\" home=\"192.168.10.227:50001\">");
	//sprintf(txBuffer, "<epm uid=\"%d\" ack=\"yes\" home=\"%d.%d.%d.%d:%d\">", uid, host[0]&0xFF, host[0]>>8, host[1]&0xFF, host[1]>>8, COMM_PORT);

	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		if(xUnits[i].bInUse)
		{
			strcat(txBuffer, "<unit name=\"");
			strcat(txBuffer, xUnits[i].xUnit.Name);
			strcat(txBuffer, "\"/>");
			//sprintf(txBuffer+strlen(txBuffer), "<unit name=\"%s\"/>", xUnits[i].xUnit.Name);
		}
	}
	strcat(txBuffer, "</epm>");

	bUnitSend(txBuffer, strlen(txBuffer));

	bUnitRead(txBuffer, &sender, 1000);
	// TODO auspacken und auf ack überprüfen
	if(strcmp(txBuffer, "ack") == 0)
	{
		vOledDbg("connected");
		//udpHandler_init(sender.sender[0]&0xFF, sender.sender[0]>>8, sender.sender[1]&0xFF, sender.sender[1]>>8, COMM_PORT, COMM_PORT);
		return true;
	}
	uid++;
	return false;
	//return true;
}

static void Initialize(void)
{
	int i;
	static tBoolean isInitialized = false;

	if(isInitialized)
		return;

	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		xUnits[i].bInUse = false;
	}

	isInitialized = true;
}

void vUnitHandlerTask(void * pvParameters)
{
	int i;
	tUnitJob xNewJob;
	xJobQueue = xQueueCreate(UNIT_JOB_QUEUE_LENGTH, sizeof(tUnitJob));
	xRxQueue = xQueueCreate(1, sizeof(struct tData));
	vSemaphoreCreateBinary(xSmphrSendComplete);
	xSemaphoreGive(xSmphrSendComplete);

	while(!InitXmit())
		vTaskDelay(INITIAL_BROADCAST_SEND_PERIOD / portTICK_RATE_MS);
	for(;;)
	{
		// Task only runs if a new job has been received
		if(xJobQueue != NULL && xQueueReceive(xJobQueue, &xNewJob, portMAX_DELAY))
		{
			// Search corresponding unit and call its job handler
			for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
			{
				// corresponding unit found
				if(xUnits[i].bInUse && strcmp(xUnits[i].xUnit.Name, xNewJob.unitName) == 0)
				{
					xUnits[i].xUnit.vNewJob(xNewJob);
					// TODO Handle handle handle
				}
			}
		}
	}
}

tUnit * xUnitCreate(char * Name, tcbUnitNewJob JobReceived)
{
	int i;
	tUnit * ret = NULL;

	Initialize();

	// Validate parameters
	if(strlen(Name) < sizeof(ret->Name)) // Name fits in array?
		goto parameters_valid;
	return NULL;

	parameters_valid:
	// Get a free slot
	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		if(xUnits[i].bInUse == false)
		{
			xUnits[i].bInUse = true;
			ret = &(xUnits[i].xUnit);
			break;
		}
	}

	// found free slot
	if(ret != NULL)
	{
		// initial setup
		memset(ret, 0, sizeof(tUnit));
		strcpy(ret->Name, Name);
		ret->vNewJob = JobReceived;
	}
	return ret;
}

tBoolean xUnitUnlink(tUnit * pUnit)
{
	int i;

	// Validate parameters
	// -> validate Unit pointer to be one of the global unit pool
	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		if(xUnits[i].bInUse && &(xUnits[i].xUnit) == pUnit)
			goto parameters_valid;
	}
	return false;

	parameters_valid:
	memset(pUnit, 0, sizeof(tUnit));
	return true;
}


/**
 *
 */
tBoolean bUnitAddCapability(tUnit * pUnit, tUnitCapability Capability)
{
	int i;

	// Validate parameters
	// -> validate Unit pointer to be one of the global unit pool
	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		if(xUnits[i].bInUse && &(xUnits[i].xUnit) == pUnit)
			goto parameters_valid;
	}
	return false;

	parameters_valid:
	for(i=0; i<UNIT_MAX_CAPABILITIES; i++)
	{
		if(!UNIT_CAPABILITY_VALID(pUnit->xCapabilities[i]))
		{
			memcpy(pUnit->xCapabilities[i].Type, Capability.Type, sizeof(Capability.Type));
			pUnit->xCapabilities[i].pxDependancy = Capability.pxDependancy;
			return true;
		}
	}
	return false;
}

/**
 * schickt Daten an buffer, der weiter an buffer und so weiter... am ende über UDP am Empfänger juhu
 */
tBoolean bUnitSend(const unsigned char * data, int dataLen)//const tUnit * pUnit, const tUnitCapability * Capability, const tUnitValue value)
{
	if(xSmphrSendComplete != NULL)
		xSemaphoreTake(xSmphrSendComplete, portMAX_DELAY);
	else
		return false;
	if(data != txBuffer)
		memcpy(txBuffer, data, dataLen);
	txBufferLen = dataLen;
	newTxData = true;
}

tBoolean bUnitRead(unsigned char * buffer, struct tData * sender, portTickType timeout)
{
	syncRxFlag = true;

	if(xRxQueue != NULL && xQueueReceive(xRxQueue, sender, timeout/configTICK_RATE_HZ))
	{
		memcpy(buffer, sender->data, sender->dataLen);
		return true;
	}
	else
	{
		sender->dataLen = 0;
		sender->data = NULL;
		return false;
	}

}


/**
 * Es muss noch eine Funktion gebaut werden, die fungiert für neue jobs
 * Die kann dann co-routines erstellen und löschen für periodische Jobs
 * das wars erstmal...
 */
void vUnitNewUdpData(unsigned char * pData, unsigned int uDataLen, uip_ipaddr_t * sender)
{
	int i;
	tUnitJob xJob;

	if(syncRxFlag)
	{
		struct tData enqueueData;
		enqueueData.data = pData;
		enqueueData.dataLen = uDataLen;
		uip_ipaddr_copy(&(enqueueData.sender), sender);
		if(xRxQueue != NULL)
			xQueueSend(xRxQueue, &enqueueData, 0);
		syncRxFlag = false;
	}

	if(memcmp(pData, "TEL:", 4) == 0)
	{
		pData += 4;
		strcpy(xJob.unitName, pData);
		strcpy(xJob.xCapability.Type, pData+strlen(pData)+1);

		// TODO: xJob = blblaextract();

		if(xJobQueue != NULL)
			xQueueSend(xJobQueue, &xJob, UNIT_MAX_WAITTIME_ON_FULL_QUEUE/portTICK_RATE_MS );
	}
}

void vUnitCheckUdpEntries(void)
{
	if(newTxData)
	{
		memcpy(uip_appdata, txBuffer, txBufferLen);
		uip_udp_send(txBufferLen);
		newTxData = false;
	}
	if(xSmphrSendComplete != NULL)
		xSemaphoreGive(xSmphrSendComplete);
}
