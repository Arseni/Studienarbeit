#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "croutine.h"

#include "unit.h"
#include "udpHandler.h"
#include <string.h>

#include "muXML/muXMLAccess.h"

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

	tUnit * unit;
	tUnitCapability * capability;
	uip_udp_endpoint_t endpoint;

	tBoolean seqNo;
	tBoolean relTime;
	tBoolean ack;
	unsigned int ds;
	unsigned int dt;

	tBoolean inUse;
};

static struct tUnitHandler xUnits[UNIT_MAX_GLOBAL_UNITS];
static struct tUnitJobHandler xJobs[UNIT_MAX_GLOBAL_JOBS_PARALLEL];
static xQueueHandle xJobQueue = NULL;
static xSemaphoreHandle xJobHandlerMutex;
static unsigned char txBuffer[UNIT_TX_BUFFER_LEN];
static unsigned char xmlBuffer[UNIT_XML_TREE_BUFFER_LEN];
static tBoolean ack = false;

static void InitXmitRxCallback(unsigned char * data, int len, uip_udp_endpoint_t sender)
{
	if(strcmp(data, "ack"))
	{
		bUdpReceiveAsync(InitXmitRxCallback, 1);
	}
	else
	{
		ack = true;
		udpHandler_init(sender);
	}
}
static void DispatchXmitRxCallback(unsigned char * data, int len, uip_udp_endpoint_t sender)
{
	int i,j;
	struct tUnitJobHandler * xjob = NULL;

	if(!xSemaphoreTake(xJobHandlerMutex, 200))
	{
		// failure !!
	}

	for (i = 0; i < UNIT_MAX_GLOBAL_JOBS_PARALLEL; i++)
	{
		if (!xJobs[i].inUse)
		{
			xjob = &(xJobs[i]);
			break;
		}
	}
	xSemaphoreGive(xJobHandlerMutex);

	if (xjob == NULL)
	{
		// over capacity !!
	}
	bUnitExtractJobHandle(xjob, data, sender);

	if (xjob->inUse)
	{
	}




/*	//enqueue job to corresponding unit
	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		if(xUnits[i].bInUse && strcmp(xUnits[i].xUnit.Name, xjob.unitName) == 0)
		{
			for(j=0; j<UNIT_MAX_CAPABILITIES; j++)
			{
				if(strcmp(xUnits[i].xUnit.xCapabilities[j].Type, xjob.xCapability.Type) == 0)
				{
					// newJob(Capability, Data) müsst reichen
					xUnits[i].xUnit.vNewJob(xjob);
				}
			}
		}
	}*/
}
static int unitBuildInitialEpm(unsigned char * buffer, int maxSize)
{
	int i;
	void * pp = muXMLCreateTree(xmlBuffer, sizeof(xmlBuffer), "epm");
	muXMLUpdateAttribute((struct muXMLTree*)xmlBuffer, &(((struct muXMLTree*)xmlBuffer)->Root), "uid", "0");
	muXMLUpdateAttribute((struct muXMLTree*)xmlBuffer, &(((struct muXMLTree*)xmlBuffer)->Root), "ack", "yes");
	muXMLUpdateAttribute((struct muXMLTree*)xmlBuffer, &(((struct muXMLTree*)xmlBuffer)->Root), "home", "192.168.0.13:50001");
	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		if(xUnits[i].bInUse)
		{
			muXMLCreateElement(pp, "unit");
			muXMLUpdateAttribute((struct muXMLTree*)xmlBuffer, pp, "name", xUnits[i].xUnit.Name);
			pp = muXMLAddElement((struct muXMLTree*)xmlBuffer, &(((struct muXMLTree*)xmlBuffer)->Root), pp);
		}
	}


	muXMLTreeEncode(buffer, maxSize, (struct muXMLTree*)xmlBuffer);

	return strlen(buffer);
}

void vUnitHandlerTask(void * pvParameters)
{
	int i, dataLen;
	tUnitJob xNewJob;

	xJobQueue = xQueueCreate(UNIT_JOB_QUEUE_LENGTH, sizeof(tUnitJob));
	xJobHandlerMutex = xSemaphoreCreateMutex();
	// Init State
	dataLen = unitBuildInitialEpm(txBuffer, sizeof(txBuffer));
	bUdpReceiveAsync(InitXmitRxCallback, 1);
	while(!ack)
	{
		bUdpSendAsync(txBuffer, dataLen);
		vTaskDelay(1000/portTICK_RATE_MS);
	}

	// Unit dispatch state
	bUdpReceiveAsync(DispatchXmitRxCallback, -1);
	while(true)
	{
		xQueueReceive(xJobQueue, &xNewJob, portMAX_DELAY);
		for(i=0; i<UNIT_MAX_GLOBAL_JOBS_PARALLEL; i++)
		{
		}
	}
}

tUnit * xUnitCreate(char * Name, tcbUnitNewJob JobReceived)
{
	int i;
	tUnit * ret = NULL;

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

		//finally give the unit the InUse status
		xUnits[i].bInUse = true;
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
tUnitCapability * bUnitAddCapability(tUnit * pUnit, tUnitCapability Capability)
{
	int i;

	// Validate parameters
	// -> validate Unit pointer to be one of the global unit pool
	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		if(xUnits[i].bInUse && &(xUnits[i].xUnit) == pUnit)
			goto parameters_valid;
	}
	return NULL;

	parameters_valid:
	for(i=0; i<UNIT_MAX_CAPABILITIES; i++)
	{
		if(!UNIT_CAPABILITY_VALID(pUnit->xCapabilities[i]))
		{
			pUnit->xCapabilities[i].pxDependancy = Capability.pxDependancy;
			memcpy(pUnit->xCapabilities[i].Type, Capability.Type, sizeof(Capability.Type));
			return &(pUnit->xCapabilities[i]);
		}
	}
	return NULL;
}

tBoolean bUnitSend(tUnit * unit, tUnitCapability * capability, char * data)
{
	// TODO xmlBuildSendStringOderSo
	strcpy(txBuffer, "<epm><unit name=\"");
	strcat(txBuffer, unit->Name);
	strcat(txBuffer, "\"><");
	strcat(txBuffer, capability->Type);
	strcat(txBuffer, ">");
	strcat(txBuffer, data);
	strcat(txBuffer, "</");
	strcat(txBuffer, capability->Type);
	strcat(txBuffer, ">");
	strcat(txBuffer, "</unit></epm>");

	bUdpSendAsync(txBuffer, strlen(txBuffer));
}
