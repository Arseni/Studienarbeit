#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "muXML/muXMLAccess.h"
#include "OLEDDisplay/oledDisplay.h"

#include "unit.h"
#include "comportUnit.h"

static tUnit * serComUnit;
//static tUnitCapability * readCapability, * writeCapability;
int stx = -1, etx = -1, sendoutImmediateUid;
tBoolean sendoutImmediate = false;
char rxBuf[100];

/**
 * Callback Funktion für Eintreffen eines Jobs
 * Diese Funktion wird vom UDP Task als Job Handler aufgerufen
 * und muss den darin befindlichen Job in den Unit Task
 * synchronisieren
 */
static eUnitJobState vComportUnitJobReceived(struct muXMLTreeElement * job, int uid);

void readCom(int start, int stop, char * buffer, int * cnt)
{
	int c = -1;
	*cnt = 0;
	if(start == -1 || stop == -1)
		return;


	while(c != start)
	{
		c = xComGetChar(1,0);
	}

	c = xComGetChar(1,0);
	while(c != -1 && c != stop)
	{
		*buffer++ = (char)(c & 0xFF);
		(*cnt)++;
		c = xComGetChar(1,0);
	}
	*buffer = 0;
	return;
}

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
		if(sendoutImmediate)
		{
			int cnt = 0;
			readCom(stx, etx, rxBuf, &cnt);
			if(cnt > 0)
			{
				static struct muXMLTreeElement element;
				char * next;

				// if an error accured, add error attribute to unit or something
				next = muXMLCreateElement(&element, "read");// ButtonStateCapability->Name);
				muXMLUpdateAttribute(&element, "data", "string");

				next = muXMLUpdateData(&element, rxBuf);
				bUnitSend(&element, sendoutImmediateUid);//xBtnUnit, xQueueItem.xValue.xJob);
			}
		}
	}
}
/*-----------------------------------------------------------*/

/*
 * Callback für Unit müssste sowas sein wie: entsperre semaphore, schreibe daten in queue
 */
static eUnitJobState vComportUnitJobReceived(struct muXMLTreeElement * job, int uid)
{
	char * tmpVal;
	int i;
	eUnitJobState ret = 0;

	if(strcmp(job->Element.Name, "read") == 0)// readCapability->Name) == 0)
	{
		tmpVal = muXMLGetAttributeByName(job, "stx");
		if(tmpVal != NULL)
			stx = atoi(tmpVal);
		else
			stx = -1;

		tmpVal = muXMLGetAttributeByName(job, "etx");
		if(tmpVal != NULL)
			etx = atoi(tmpVal);
		else
			etx = -1;

		if(strcmp(muXMLGetAttributeByName(job, "onchange"), "yes") == 0)
		{
			sendoutImmediateUid = uid;
			sendoutImmediate = true;
			ret |= JOB_STORE;
		}
		else
		{
			// HIER WEITER und zwar die read synchron dings
			//readCom(stx, etx, rxBuf, &cnt);
		}
	}
	if(strcmp(job->Element.Name, "write") == 0)
	{
		for(i=0; i<job->Data.DataSize; i++)
			vComPutChar(1, job->Data.Data[i], 0);
	}

	return ret | JOB_ACK;
}
