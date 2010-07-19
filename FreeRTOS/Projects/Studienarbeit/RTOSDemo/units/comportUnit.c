#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "muXML/muXMLAccess.h"

#include "unit.h"
#include "comportUnit.h"

static tUnit * serComUnit;
static tUnitCapability * readCapability, * writeCapability;
int stx = -1, etx = -1, sendoutImmediateUid;
tBoolean sendoutImmediate = false;

/**
 * Callback Funktion für Eintreffen eines Jobs
 * Diese Funktion wird vom UDP Task als Job Handler aufgerufen
 * und muss den darin befindlichen Job in den Unit Task
 * synchronisieren
 */
static eUnitJobState vComportUnitJobReceived(struct muXMLTreeElement * job, int uid);

/**
 * Description !!!
 */
void vComportUnitTask( void *pvParameters )
{
	const portTickType xDelay = 1000 / portTICK_RATE_MS;
	int c;

	// Setup com port
	xComOpen(1,2,3,4,5,6);

	// Setup Unit
	serComUnit = xUnitCreate("SerCom", vComportUnitJobReceived);

	// Periodic
	for(;;)
	{
		c = xComGetChar(1, 0);
		vComPutChar(1, c, 0);
	}
}
/*-----------------------------------------------------------*/

/*
 * Callback für Unit müssste sowas sein wie: entsperre semaphore, schreibe daten in queue
 */
static eUnitJobState vComportUnitJobReceived(struct muXMLTreeElement * job, int uid)
{
	char * tmpVal;
	eUnitJobState ret = 0;

	if(strcmp(job->SubElements->Element.Name, readCapability->Name) == 0)
	{
		tmpVal = muXMLGetAttributeByName(job->SubElements, "stx");
		if(tmpVal != NULL)
			stx = *tmpVal;
		else
			stx = -1;

		tmpVal = muXMLGetAttributeByName(job->SubElements, "etx");
		if(tmpVal != NULL)
			etx = *tmpVal;
		else
			etx = -1;

		if(strcmp(muXMLGetAttributeByName(job->SubElements, "sendonarrival"), "yes") == 0)
		{
			sendoutImmediate = true;
			sendoutImmediateUid = uid;
			ret |= JOB_STORE;
		}
	}

	return ret | JOB_ACK;
}
