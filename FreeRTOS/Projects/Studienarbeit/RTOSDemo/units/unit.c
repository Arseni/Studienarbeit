#include "FreeRTOS.h"
#include "queue.h"
#include "croutine.h"

#include "unit.h"
#include <string.h>


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

static void Initialize(void)
{
	static tBoolean isInitialized = false;

	if(isInitialized)
		return;

	int i;
	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		xUnits[i].bInUse = false;
	}

	isInitialized = true;
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
 * TODO: Not yet implemented, sollte aber sowas wie ein enqueue sein
 */
tBoolean bUnitSend(const tUnit * pUnit, const tUnitCapability * Capability, const tUnitValue value)
{
	return true;
}

/**
 * Es muss noch eine Funktion gebaut werden, die fungiert f�r neue jobs
 * Die kann dann co-routines erstellen und l�schen f�r periodische Jobs
 * das wars erstmal...
 */
void vUnitJobExtract(unsigned char * pData, unsigned int uDataLen)
{
	int i;
	tUnitJob xJob;

	strcpy(xJob.unitName, pData);
	strcpy(xJob.xCapability.Type, "ButtonState");

	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		if(xUnits[i].bInUse && strcmp(xUnits[i].xUnit.Name, xJob.unitName) == 0)
			xUnits[i].xUnit.vNewJob(xJob);
	}
	// TODO: xJob = blblaextract();

	// if(job == one shot)
	// create coroutine mit nem freien xjobhandler slot
	// if(job == cyclic)
	// pr�fe ob so einer existiert und passe ihn an, wenn nicht dann erstellen
	// if(job == schliessen)
	// suche den job und schlie�e ihn

}
