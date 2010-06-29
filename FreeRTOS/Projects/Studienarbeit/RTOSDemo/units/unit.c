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

#define STATUS_USE_SEQ_NO		(1<<0)
#define STATUS_USE_UID			(1<<1)
#define STATUS_ACK_REQUESTED	(1<<2)
#define STATUS_SEND_ON_ARRIVE	(1<<3)

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

	int uid;
	int seqNo;
	tBoolean relTime;
	tBoolean ack;
	unsigned int ds;
	unsigned int dt;
	char statusFlags;

	tBoolean store;
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
	eUnitJobState state = 0;
	struct tUnitJobHandler * xjob = NULL;

	xjob = unitGetFreeJobHandler();
	if(xjob == NULL)
		return;

	unitExtractJobHandle(xjob, data, sender);

	if (!xjob->inUse)
	{
		// TODO fehler
	}

	state = xjob->unit->vNewJob(xjob->xJob);

	if(state & JOB_STORE)
		xjob->store = true;

	if(xjob->statusFlags & STATUS_ACK_REQUESTED && state & JOB_ACK)
	{
		tUnitJob job;
		job.xCapability = unitGetCapabilityByName(xjob->unit, "ack");
		job.uid = xjob->xJob.uid;
		*(job.data) = 0;
		bUnitSend(xjob->unit, job);
	}
}

tUnit * unitGetUnitByName(char * Name)
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

tUnitCapability * unitGetCapabilityByName(tUnit * unit, char * Name)
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
	int i;
	struct muXMLTree * tree = muXMLTreeDecode(data, xmlBuffer, sizeof(xmlBuffer), 0, NULL);
	struct muXMLTreeElement * element;
	char * tmpStr;

	if(tree == NULL)
	{
		handler->inUse = false;
		return;
	}

	// epm attributes to flags
	handler->statusFlags = 0;
	for(i=0; i<tree->Root.Element.nAttributes; i++)
	{
		if(strcmp(tree->Root.Element.Attribute[i].Name, "ack") == 0 && strcmp(tree->Root.Element.Attribute[i].Value, "yes") == 0)
			handler->statusFlags |= STATUS_ACK_REQUESTED;
		if(strcmp(tree->Root.Element.Attribute[i].Name, "uid") == 0)
		{
			handler->uid = atoi(tree->Root.Element.Attribute[i].Value);
			handler->statusFlags |= STATUS_USE_UID;
		}
		if(strcmp(tree->Root.Element.Attribute[i].Name, "withseqno") == 0 && strcmp(tree->Root.Element.Attribute[i].Value, "yes") == 0)
		{
			handler->statusFlags |= STATUS_USE_SEQ_NO;
			handler->seqNo = 1;
		}
		if(strcmp(tree->Root.Element.Attribute[i].Name, "sendonarrival") == 0 && strcmp(tree->Root.Element.Attribute[i].Value, "yes") == 0)
		{
			handler->statusFlags |= STATUS_SEND_ON_ARRIVE;
		}
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
	handler->xJob.parameter = element->Element.Attribute;
	handler->xJob.parametersCnt = element->Element.nAttributes;
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

static char * intToStr(char * buffer, int number)
{
	sprintf(buffer, "%d", number);
	return buffer;
}

tBoolean bUnitSend(tUnit * unit, tUnitJob xjob)
{
	int i;
	struct tUnitJobHandler * handler = NULL;
	void * pp, * p;
	char tmpStr[UNIT_MIDDLE_STRING];

	for(i=0; i<UNIT_MAX_GLOBAL_JOBS_PARALLEL; i++)
	{
		if(xjob.uid == xJobs[i].xJob.uid)
		{
			handler = &xJobs[i];
			break;
		}
	}

	pp = muXMLCreateTree(xmlBuffer, sizeof(xmlBuffer), "epm");
	if((handler->statusFlags & STATUS_USE_SEQ_NO) && (handler->xJob.xCapability == xjob.xCapability))
		muXMLUpdateAttribute((struct muXMLTree*)xmlBuffer, &(((struct muXMLTree*)xmlBuffer)->Root), "seqno", intToStr(tmpStr, handler->seqNo++));
	if(handler->statusFlags & STATUS_USE_UID)
		muXMLUpdateAttribute((struct muXMLTree*)xmlBuffer, &(((struct muXMLTree*)xmlBuffer)->Root), "uid", intToStr(tmpStr, handler->uid));

	muXMLCreateElement(pp, "unit");
	muXMLUpdateAttribute((struct muXMLTree*)xmlBuffer, pp, "name", unit->Name);
	p = pp;
	pp = muXMLAddElement((struct muXMLTree*)xmlBuffer, &(((struct muXMLTree*)xmlBuffer)->Root), pp);

	muXMLCreateElement(pp, xjob.xCapability->Name);
	muXMLUpdateData((struct muXMLTree*)xmlBuffer, pp, xjob.data);
	muXMLAddElement((struct muXMLTree*)xmlBuffer, p, pp);

	muXMLTreeEncode(txBuffer, sizeof(txBuffer), (struct muXMLTree*)xmlBuffer);

	bUdpSendAsync(txBuffer, strlen(txBuffer));

	if(!handler->store)
		handler->inUse = false;
}
