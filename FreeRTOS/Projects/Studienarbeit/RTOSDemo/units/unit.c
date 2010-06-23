#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "croutine.h"

#include "unit.h"
#include "udpHandler.h"
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
static xQueueHandle xJobQueue = NULL;
static unsigned char txBuffer[200];
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
	int i;
	tUnitJob xjob;

	//build job out of datengrütze and enqueue it
	char * pp;

	/*pp = xjob.unitName;
	while(*data != ';')
		*pp++ = *data++;
	*pp=0;data++;
	pp = xjob.xCapability.Type;
	while(*data != ';')
		*pp++ = *data++;
	*pp=0;data++;
	pp = xjob.xCapability.Data;
	while(*data != ';')
		*pp++ = *data++;
	*pp=0;*/
	strcpy(xjob.unitName, "Buttons");
	strcpy(xjob.xCapability.Type, data);
	strcpy(xjob.xCapability.Data, "none");

	//enqueue job to corresponding unit
	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		if(xUnits[i].bInUse && strcmp(xUnits[i].xUnit.Name, xjob.unitName) == 0)
		{
			xUnits[i].xUnit.vNewJob(xjob);
		}
	}
}
static int unitBuildInitialEpm(unsigned char * buffer)
{
	strcpy(buffer, "<epm>FIXME</epm>");
	return strlen(buffer);
}

void vUnitHandlerTask(void * pvParameters)
{
	int i, dataLen;
	tUnitJob xNewJob;

	xJobQueue = xQueueCreate(UNIT_JOB_QUEUE_LENGTH, sizeof(tUnitJob));

	// Init State
	dataLen = unitBuildInitialEpm(txBuffer);
	bUdpReceiveAsync(InitXmitRxCallback, 1);
	while(!ack)
	{
		bUdpSendAsync(txBuffer, dataLen);
		vTaskDelay(1000/portTICK_RATE_MS);
	}

	// Unit dispatch state
	bUdpReceiveAsync(DispatchXmitRxCallback, -1);


	while(true);
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
