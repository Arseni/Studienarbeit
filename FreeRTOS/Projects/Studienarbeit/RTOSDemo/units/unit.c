#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include "unit.h"
#define DBG(X);//#include "unitDebug.h"
#include "udpHandler.h"
#include "dataAccess.h"
#include <string.h>

#include "muXML/muXMLAccess.h"

#include "OLEDDisplay/oledDisplay.h"

#define NEW_UID() (++unitJobUID)
#define SUPER_UNIT ((tUnit*)(-1))
#define HANDLE_ATTRIBUTE(A) if(strcmp(tree->Root.Element.Attribute[i].Name, A) == 0)
#define HANDLE_VALUE(A) if(strcmp(tree->Root.Element.Attribute[i].Value, A) == 0)
#define ATTR_VALUE()	(tree->Root.Element.Attribute[i].Value)
#define ATTR_NAME()		(tree->Root.Element.Attribute[i].Name)

#define SINK() muXMLUpdateAttribute(tree->Root.SubElements->SubElements, ATTR_NAME(), ATTR_VALUE())
#define SINK_AS(X,Y) muXMLUpdateAttribute(tree->Root.SubElements->SubElements, X, Y)

#define STATUS_USE_SEQ_NO		(1<<0)
#define STATUS_USE_UID			(1<<1)
#define STATUS_ACK_REQUESTED	(1<<2)
#define STATUS_PERIODIC			(1<<3)
#define STATUS_DELETE			(1<<4)

struct tUnitHandler
{
	tUnit xUnit;
	tBoolean bInUse;
};

static const char capabilityReadyStateDescription[] = "Ready";

static struct tUnitHandler xUnits[UNIT_MAX_GLOBAL_UNITS];
static struct tUnitJobHandler xJobs[UNIT_MAX_GLOBAL_JOBS_PARALLEL];
//static tUnitCapability xGlobalUnitCapability;
static xSemaphoreHandle xJobHandlerMutex;
static unsigned char txBuffer[UNIT_TX_BUFFER_LEN];
static unsigned char xmlBuffer[UNIT_XML_TREE_BUFFER_LEN];
static tBoolean ack = false;
static unsigned int unitJobUID;

tBoolean bUnitSendTo(struct muXMLTreeElement * xjob, int uid, uip_udp_endpoint_t * dest);
static void unitExtractJobHandle(struct tUnitJobHandler * handler, unsigned char * data, uip_udp_endpoint_t sender);
static void RunJobHandler(struct tUnitJobHandler * xjob, uip_udp_endpoint_t sender);

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
			ret->startTime = xTaskGetTickCount();
			break;
		}
	}
	xSemaphoreGive(xJobHandlerMutex);
	//memset(ret, 0, sizeof(struct tUnitJobHandler));
	return ret;
}

static void InitXmitRxCallback(unsigned char * data, int len, uip_udp_endpoint_t sender)
{
	struct muXMLTree * tree = muXMLTreeDecode(data, xmlBuffer, sizeof(xmlBuffer), 0, NULL);

	if(strcmp(tree->Root.SubElements->SubElements->Element.Name, "ack"))
	/*
	struct tUnitJobHandler * xjob = unitGetFreeJobHandler();
	unitExtractJobHandle(xjob, data, sender);
	if(strcmp(xjob->job->Element.Name, "ack"))*/
	{
		bUdpReceiveAsync(InitXmitRxCallback, 1);
	}
	else
	{
		ack = true;
		udpHandler_init(sender);
		//xjob->inUse = false;
	}
}

tBoolean deleteJobByUID(int uid)
{
	int i;
	for (i = 0; i < UNIT_MAX_GLOBAL_JOBS_PARALLEL; i++)
	{
		if (xJobs[i].inUse && xJobs[i].uid == uid)
		{
			xJobs[i].inUse = false;
			return true;
		}
	}
	return false;
}

int hasJobWithUid(int uid)
{
	int i, ret = 0;
	for (i = 0; i < UNIT_MAX_GLOBAL_JOBS_PARALLEL; i++)
	{
		if (xJobs[i].inUse && xJobs[i].uid == uid)
		{
			ret++;
		}
	}
	return ret;
}

static void sendNAK(unsigned char * data, uip_udp_endpoint_t sender)
{
	struct tUnitJobHandler job[1];
	struct muXMLTreeElement element;

	unitExtractJobHandle(job, data, sender);

	muXMLCreateElement(&element, "ack");
	muXMLUpdateAttribute(&element, "cmd", job->job->Element.Name);
	if(job->statusFlags & STATUS_USE_UID)
	{
		char uidString[6];
		sprintf(uidString, "%d", job->uid);
		muXMLUpdateAttribute(&element, "uid", uidString);
	}

	muXMLUpdateAttribute(&element, "error", "full");

	bUnitSendTo(&element, job->internal_uid, &sender);
}

static void DispatchXmitRx(unsigned char * data, int len,
		uip_udp_endpoint_t sender)
{
	int i, j;
	struct tUnitJobHandler * xjob = NULL;

	xjob = unitGetFreeJobHandler();
	if (xjob == NULL)
	{
		sendNAK(data, sender);
		DBG("Kein freier Slot");
		return;
	}

	memcpy(&(xjob->endpoint), &(sender), sizeof(uip_udp_endpoint_t));

	unitExtractJobHandle(xjob, data, sender);

	if (!xjob->inUse)
	{
		return;
	}


	// handle stored jobs
	RunJobHandler(xjob, sender);
}

static void RunJobHandler(struct tUnitJobHandler * xjob, uip_udp_endpoint_t sender)
{
	eUnitJobState state = 0;

	if(!xjob->inUse)
		return;

	state = xjob->unit->vNewJob(xjob->job, xjob->internal_uid);

	if (state & JOB_ACK)
	{
		if (state & JOB_STORE)
			xjob->store = true;

		if (xjob->statusFlags & STATUS_PERIODIC)
		{
			xjob->store = true;
			vUnitTimerStart(xjob->dt, RunJobHandler, xjob, sender);
		}
		if(xjob->statusFlags & STATUS_DELETE)
			if(hasJobWithUid(xjob->uid) < 2)
				state &= ~JOB_ACK;
	}

	// ack ?
	if (xjob->statusFlags & STATUS_ACK_REQUESTED)
	{
		struct muXMLTreeElement element;
		muXMLCreateElement(&element, "ack");
		muXMLUpdateAttribute(&element, "cmd", xjob->job->Element.Name);
		/*if(xjob->statusFlags & STATUS_USE_UID)
		{
			char uidString[6];
			sprintf(uidString, "%d", xjob->uid);
			muXMLUpdateAttribute(&element, "uid", uidString);
		}*/
		if(!(state & JOB_ACK))
			muXMLUpdateAttribute(&element, "error", "internal");
		bUnitSendTo(&element, xjob->internal_uid, &sender);
	}

	if(state & JOB_COMPLETE)
		xjob->inUse = false;

	if(xjob->statusFlags & STATUS_DELETE)
	{
		deleteJobByUID(xjob->uid);
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

tBoolean setupRequired(struct muXMLTreeElement * element)
{
	int i, ret=0;
	for(i=0; i<element->Element.nAttributes; i++)
		if(strcmp(element->Element.Attribute[i].Name, "name"))
			ret++;
	return ret;
}

/**
 */
static void unitExtractJobHandle(struct tUnitJobHandler * handler, unsigned char * data, uip_udp_endpoint_t sender)
{
	int i,j;
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
	handler->timeout = -1; // default timeout
	for(i=0; i<tree->Root.Element.nAttributes; i++)
	{
		if(strcmp(tree->Root.Element.Attribute[i].Name, "ack") == 0 && strcmp(tree->Root.Element.Attribute[i].Value, "yes") == 0)
			handler->statusFlags |= STATUS_ACK_REQUESTED;
		if(strcmp(tree->Root.Element.Attribute[i].Name, "uid") == 0)
		{
			handler->uid = atoi(tree->Root.Element.Attribute[i].Value);
			handler->statusFlags |= STATUS_USE_UID;
			//deleteJobByUID(handler->uid);
		}
		if(strcmp(tree->Root.Element.Attribute[i].Name, "withseqno") == 0 && strcmp(tree->Root.Element.Attribute[i].Value, "yes") == 0)
		{
			handler->statusFlags |= STATUS_USE_SEQ_NO;
			handler->seqNo = 1;
		}
		if(strcmp(tree->Root.Element.Attribute[i].Name, "dt") == 0)
		{
			handler->dt = atoi(tree->Root.Element.Attribute[i].Value);
			if(handler->dt)
				handler->statusFlags |= STATUS_PERIODIC;
		}
		HANDLE_ATTRIBUTE("reply")
		{
			HANDLE_VALUE("never")
				handler->statusFlags |= STATUS_DELETE;

			// SINK
			muXMLUpdateAttribute(tree->Root.SubElements->SubElements, ATTR_NAME(), ATTR_VALUE());
		}
		HANDLE_ATTRIBUTE("dest")
		{
			uip_parseIpAddr(handler->endpoint.rAddr, &(handler->endpoint.rPort), ATTR_VALUE());
		}
		HANDLE_ATTRIBUTE("timeout")
		{
			handler->timeout = atoi(ATTR_VALUE());
		}
	}

	// unit extraction
	element = muXMLGetElementByName(&(tree->Root), "unit");
	if(element == NULL)
	{
		handler->inUse = false;
		DBG("EPM: <unit> fehlt");
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
		if(handler->unit == NULL)
		{
			handler->inUse = false;
			DBG("Unit not implemented");
			sendNAK(data, sender);
			return;
		}
	}

	// capability extraction
	handler->job = element->SubElements;
	if(handler->job == NULL)
	{
		handler->inUse = false;
		DBG("Kein Auftrag enthalten");
		return;
	}

	if(handler->unit == SUPER_UNIT)
	{
		for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
		{
			if(xUnits[i].bInUse)
			{
				struct tUnitJobHandler * addJob = unitGetFreeJobHandler();
				if(addJob == NULL)
				{
					sendNAK(data, sender);
					handler->inUse = false;
					return;
				}
				memcpy(addJob, handler, sizeof(struct tUnitJobHandler));
				addJob->unit = &(xUnits[i].xUnit);
				addJob->internal_uid = NEW_UID();

				if(setupRequired(element))
				{
					struct tUnitJobHandler * addJob = unitGetFreeJobHandler();
					static struct muXMLTreeElement setup[1];
					if(addJob == NULL)
					{
						sendNAK(data, sender);
						handler->inUse = false;
						return;
					}
					memcpy(addJob, handler, sizeof(struct tUnitJobHandler));
					addJob->job = setup;
					strcpy(setup->SubElements->Element.Name, "setup");
					for(i=0; i<element->Element.nAttributes; i++)
					{
						if(strcmp(element->Element.Attribute[i].Name, "name"))
							muXMLUpdateAttribute(setup, element->Element.Attribute[i].Name, element->Element.Attribute[i].Value);
					}
					addJob->internal_uid = NEW_UID();
					RunJobHandler(addJob, sender);
				}

				RunJobHandler(addJob, sender);
			}
		}
		handler->inUse = false;
		return;
	}

	if(setupRequired(element))
	{
		struct tUnitJobHandler * addJob = unitGetFreeJobHandler();
		static struct muXMLTreeElement setup[1];
		if(addJob == NULL)
		{
			sendNAK(data, sender);
			handler->inUse = false;
			return;
		}
		memcpy(addJob, handler, sizeof(struct tUnitJobHandler));
		addJob->job = setup;
		strcpy(setup->SubElements->Element.Name, "setup");
		for(i=0; i<element->Element.nAttributes; i++)
		{
			if(strcmp(element->Element.Attribute[i].Name, "name"))
				muXMLUpdateAttribute(setup, element->Element.Attribute[i].Name, element->Element.Attribute[i].Value);
		}
		addJob->internal_uid = NEW_UID();
		RunJobHandler(addJob, sender);
	}

	handler->internal_uid = NEW_UID();
	handler->inUse = true;
}

static int unitBuildInitialEpm(unsigned char * buffer, int maxSize)
{
	int i;
	void * pp = muXMLCreateTree(xmlBuffer, sizeof(xmlBuffer), "epm");
	muXMLUpdateAttribute(&(((struct muXMLTree*)xmlBuffer)->Root), "uid", "0");
	muXMLUpdateAttribute(&(((struct muXMLTree*)xmlBuffer)->Root), "ack", "yes");
	muXMLUpdateAttribute(&(((struct muXMLTree*)xmlBuffer)->Root), "home", "192.168.0.13:50000");// TODO ip adresse!!
	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		if(xUnits[i].bInUse)
		{
			muXMLCreateElement(pp, "unit");
			muXMLUpdateAttribute(pp, "name", xUnits[i].xUnit.Name);
			pp = muXMLAddElement(&(((struct muXMLTree*)xmlBuffer)->Root), pp);
		}
	}


	muXMLTreeEncode(buffer, maxSize, (struct muXMLTree*)xmlBuffer);

	return strlen(buffer);
}

void vUnitHandlerTask(void * pvParameters)
{
	int dataLen, i;

	xJobHandlerMutex = xSemaphoreCreateMutex();
	// Init State
	dataLen = unitBuildInitialEpm(txBuffer, sizeof(txBuffer));
	bUdpReceiveAsync(InitXmitRxCallback, 1);
	while(!ack)
	{
		//bUdpSendAsync(txBuffer, dataLen);
		bUdpSendTo(txBuffer, dataLen, (uip_udp_endpoint_t){0xFFFF, 0xFFFF, 50001, 50000});
		vTaskDelay(1000/portTICK_RATE_MS);
	}
	// Unit dispatch state
	bUdpReceiveAsync(DispatchXmitRx, -1);

	while(true)
	{
		// check the unit job internals
		vTaskDelay(UNIT_JOB_VALIDATION_INTERVALL);

		// get exclusive access to jobhandlers
		xSemaphoreTake(xJobHandlerMutex, portMAX_DELAY);

		for (i = 0; i < UNIT_MAX_GLOBAL_JOBS_PARALLEL; i++)
		{
			int timeout;
			if(xJobs[i].timeout > 0)
				timeout = xJobs[i].timeout / portTICK_RATE_MS;
			else
				timeout = UNIT_JOB_TIMEOUT / portTICK_RATE_MS;


			if(xJobs[i].inUse // if active, ...
			&& !xJobs[i].store // ... not selfcontrolled, ...
			&& xTaskGetTickCount() - xJobs[i].startTime > timeout)// ... and overdue
			{
				struct muXMLTreeElement err;

				xSemaphoreGive(xJobHandlerMutex);

				muXMLCreateElement(&err, xJobs[i].job->Element.Name);
				muXMLUpdateAttribute(&err, "error", "timeout");
				bUnitSend(&err, xJobs[i].internal_uid);

				xSemaphoreTake(xJobHandlerMutex, portMAX_DELAY);
			}
		}
		//release jobhandlers
		xSemaphoreGive(xJobHandlerMutex);
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
tUnitCapability * xUnitAddCapability(tUnit * pUnit, char * Name)
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
			strcpy(pUnit->xCapabilities[i].Name, Name);
			pUnit->xCapabilities[i].State = 0;
			pUnit->xCapabilities[i].StateDescription = capabilityReadyStateDescription;
			return &(pUnit->xCapabilities[i]);
		}
	}
	return NULL;
}

tBoolean bUnitUpdateCapability(tUnitCapability * pCapability, char * stateDesc, eUnitState state)
{
	pCapability->State = state;
	pCapability->StateDescription = stateDesc;

	return true;
}

tBoolean bUnitUnlinkCapability(tUnitCapability * pCapability)
{
	*(pCapability->Name) = 0;
	return true;
}


tBoolean bUnitSendTo(struct muXMLTreeElement * xjob, int uid, uip_udp_endpoint_t * dest)
{
	int i;
	struct tUnitJobHandler * handler = NULL;
	void * pp, * p;
	char tmpStr[UNIT_MIDDLE_STRING];

	if(!xSemaphoreTake(xJobHandlerMutex, portMAX_DELAY))
	{
		return false;
	}

	for(i=0; i<UNIT_MAX_GLOBAL_JOBS_PARALLEL; i++)
	{
		if(uid == xJobs[i].internal_uid)
		{
			handler = &xJobs[i];
			break;
		}
	}

	xSemaphoreGive(xJobHandlerMutex);

	if(handler->unit == NULL
	|| handler->unit == SUPER_UNIT)
		return false;

	pp = muXMLCreateTree(xmlBuffer, sizeof(xmlBuffer), "epm");
	if(handler->statusFlags & STATUS_USE_SEQ_NO)
		muXMLUpdateAttribute(&(((struct muXMLTree*)xmlBuffer)->Root), "seqno", intToStr(tmpStr, handler->seqNo++));
	if(handler->statusFlags & STATUS_USE_UID)
		muXMLUpdateAttribute(&(((struct muXMLTree*)xmlBuffer)->Root), "uid", intToStr(tmpStr, handler->uid));

	muXMLCreateElement(pp, "unit");
	muXMLUpdateAttribute(pp, "name", handler->unit->Name);
	muXMLAddElement(&(((struct muXMLTree*)xmlBuffer)->Root), pp);

	muXMLAddElement((struct muXMLTreeElement*)pp, xjob);

	muXMLTreeEncode(txBuffer, sizeof(txBuffer), (struct muXMLTree*)xmlBuffer);

	//bUdpSendAsync(txBuffer, strlen(txBuffer));
	if(dest == NULL)
		bUdpSendTo(txBuffer, strlen(txBuffer), handler->endpoint);
	else
		bUdpSendTo(txBuffer, strlen(txBuffer), *dest);

	if(!handler->store)
		handler->inUse = false;
}
tBoolean bUnitSend(struct muXMLTreeElement * xjob, int uid)
{
	bUnitSendTo(xjob, uid, NULL);
}


