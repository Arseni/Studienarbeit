#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "croutine.h"

#include "unit.h"
#include "udpHandler.h"
#include <string.h>

#include "muXML/muXMLAccess.h"

#include "OLEDDisplay/oledDisplay.h"

#define NEW_UID() (unitJobUID++)
#define SUPER_UNIT ((tUnit*)(-1))

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
static tUnitCapability xGlobalUnitCapability;
static xQueueHandle xJobQueue = NULL;
static xSemaphoreHandle xJobHandlerMutex;
static unsigned char txBuffer[UNIT_TX_BUFFER_LEN];
static unsigned char xmlBuffer[UNIT_XML_TREE_BUFFER_LEN];
static tBoolean ack = false;
static unsigned int unitJobUID;

static void unitExtractJobHandle(struct tUnitJobHandler * handler, unsigned char * data, uip_udp_endpoint_t sender);
static struct tUnitJobHandler * unitGetFreeJobHandler()
{
	struct tUnitJobHandler * ret = NULL;
	int i;

	if(!xSemaphoreTake(xJobHandlerMutex, portMAX_DELAY))
	{
		return NULL;
	}

	for (i = 0; i < UNIT_MAX_GLOBAL_JOBS_PARALLEL; i++)
	{
		if (!xJobs[i].inUse)
		{
			ret = &(xJobs[i]);
			ret->inUse = true;
			break;
		}
	}
	xSemaphoreGive(xJobHandlerMutex);
	return ret;
}

static void InitXmitRxCallback(unsigned char * data, int len, uip_udp_endpoint_t sender)
{
	struct tUnitJobHandler * xjob = unitGetFreeJobHandler();
	unitExtractJobHandle(xjob, data, sender);
	if(strcmp(xjob->xJob.xCapability->Name, "ack"))
	{
		bUdpReceiveAsync(InitXmitRxCallback, 1);
	}
	else
	{
		ack = true;
		udpHandler_init(sender);
	}
}

/**
 * Was noch fehlt ist, dass globale Jobs übernommen werden und der timer gestartet wird etc etc etc...
 * noch viel zu tun
 */
static void DispatchXmitRxCallback(unsigned char * data, int len, uip_udp_endpoint_t sender)
{
	int i,j;
	struct tUnitJobHandler * xjob = NULL;

	xjob = unitGetFreeJobHandler();
	unitExtractJobHandle(xjob, data, sender);

	if (xjob->inUse)
	{
		xjob->unit->vNewJob(xjob->xJob);
	}
}

static tUnit * unitGetUnitByName(char * Name)
{
	int i;

	if(Name == NULL)
		return NULL;

	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		if(xUnits[i].bInUse && strcmp(xUnits[i].xUnit.Name, Name) == 0)
		{
			return &(xUnits[i].xUnit);
		}
	}
	return NULL;
}

static tUnitCapability * unitGetCapabilityByName(tUnit * unit, char * Name)
{
	int i;

	if(unit == NULL || Name == NULL)
		return NULL;

	if(unit == SUPER_UNIT)
	{
		strcpy(xGlobalUnitCapability.Name, Name);
		return &xGlobalUnitCapability;
	}

	for(i=0; i<UNIT_MAX_CAPABILITIES; i++)
	{
		if(strcmp(unit->xCapabilities[i].Name, Name) == 0)
			return &(unit->xCapabilities[i]);
	}
	return NULL;
}

/**
 * TODO: ack ds, dt etc + global dedicated
 */
static void unitExtractJobHandle(struct tUnitJobHandler * handler, unsigned char * data, uip_udp_endpoint_t sender)
{
	struct muXMLTree * tree = muXMLTreeDecode(data, xmlBuffer, sizeof(xmlBuffer), 0, NULL);
	struct muXMLTreeElement * element;
	char * tmpStr;

	if(tree == NULL)
	{
		handler->inUse = false;
		return;
	}

	// unit extraction
	element = muXMLGetElementByName(&(tree->Root), "unit");
	if(element == NULL)
	{
		handler->inUse = false;
		return;
	}
	tmpStr = muXMLGetAttributeByName(element, "name");
	if(tmpStr == NULL)
	{
		handler->unit = SUPER_UNIT;
	}
	else
	{
		handler->unit = unitGetUnitByName(tmpStr);
	}

	// capability extraction
	element = element->SubElements;
	if(element == NULL)
	{
		handler->inUse = false;
		return;
	}
	handler->xJob.xCapability = unitGetCapabilityByName(handler->unit, element->Element.Name);
	if(handler->xJob.xCapability == NULL)
	{
		handler->inUse = false;
		return;
	}
	if(element->Data.DataSize < UNIT_MIDDLE_STRING-1)
	{
		strncpy(handler->xJob.data, element->Data.Data, element->Data.DataSize);
	}
	else
	{
		handler->inUse = false;
		return;
	}
	handler->xJob.uid = NEW_UID();
	handler->inUse = true;
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
	while(true);/*
	{
		xQueueReceive(xJobQueue, &xNewJob, portMAX_DELAY);
		for(i=0; i<UNIT_MAX_GLOBAL_JOBS_PARALLEL; i++)
		{
		}
	}*/
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
			memcpy(pUnit->xCapabilities[i].Name, Capability.Name, sizeof(Capability.Name));
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
	strcat(txBuffer, capability->Name);
	strcat(txBuffer, ">");
	strcat(txBuffer, data);
	strcat(txBuffer, "</");
	strcat(txBuffer, capability->Name);
	strcat(txBuffer, ">");
	strcat(txBuffer, "</unit></epm>");

	bUdpSendAsync(txBuffer, strlen(txBuffer));
}
