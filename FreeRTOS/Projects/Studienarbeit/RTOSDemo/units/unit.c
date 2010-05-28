#include "unit.h"
#include <string.h>


struct tUnitHandler
{
	tUnit xUnit;
	tBoolean bInUse;
};
static struct tUnitHandler xUnits[UNIT_MAX_GLOBAL_UNITS];

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


tUnit * unitCreate(char * Name)
{
	int i;
	tUnit * ret;

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

	// initial setup
	memset(ret, 0, sizeof(tUnit));
	strcpy(ret->Name, Name);

	return ret;
}

tBoolean unitAddCapability(tUnit * Unit, tUnitCapability Capability)
{
	int i;

	// Validate parameters
	// -> validate Unit pointer to be one of the global unit pool
	for(i=0; i<UNIT_MAX_GLOBAL_UNITS; i++)
	{
		if(xUnits[i].bInUse && &(xUnits[i].xUnit) == Unit)
			goto parameters_valid;
	}
	return false;

	parameters_valid:
	for(i=0; i<UNIT_MAX_CAPABILITIES; i++)
	{
		if(strlen(Unit->xCapabilities[i].Type) == 0)
		{

		}
	}
}

