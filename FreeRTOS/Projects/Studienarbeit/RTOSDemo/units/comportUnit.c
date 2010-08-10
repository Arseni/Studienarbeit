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
int stx = -1, etx = -1, stxSync = -1, etxSync = -1,  sendoutImmediateUid, readRequestUid;
tBoolean sendoutImmediate = false, readRequest = false;
char rxBuf[100];

/**
 * Callback Funktion für Eintreffen eines Jobs
 * Diese Funktion wird vom UDP Task als Job Handler aufgerufen
 * und muss den darin befindlichen Job in den Unit Task
 * synchronisieren
 */
static eUnitJobState vComportUnitJobReceived(struct muXMLTreeElement * job, int uid);

int readCom(int stop, char * buffer, int * cnt)
{
	int c = -1;
	*cnt = 0;
	if(stop == -1)
		return 1;

	c = xComGetChar(1,0);
	while(c != -1 && c != stop)
	{
		*buffer++ = (char)(c & 0xFF);
		(*cnt)++;
		c = xComGetChar(1,0);
	}
	*buffer = 0;

	if(c == -1) // timeout
		return 1;

	return 0;
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

		if(sendoutImmediate || readRequest)
		{
			int cnt = 0, c;

			c = xComGetChar(1,0);
			while(c != stx && c != stxSync)
				c = xComGetChar(1,0);

			if(c == stxSync && readRequest)
			{
				readCom(etxSync, rxBuf, &cnt);
				if(cnt > 0)
				{
					static struct muXMLTreeElement element;
					char * next;

					// if an error accured, add error attribute to unit or something
					next = muXMLCreateElement(&element, "read");// ButtonStateCapability->Name);
					muXMLUpdateAttribute(&element, "data", "string");

					next = muXMLUpdateData(&element, rxBuf);

					bUnitSend(&element, readRequestUid);//xBtnUnit, xQueueItem.xValue.xJob);
					readRequest = false;
				}
			}
			else if(c == stx && sendoutImmediate)
			{
				readCom(etx, rxBuf, &cnt);
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
		int tmpStx, tmpEtx;

		if(strcmp(muXMLGetAttributeByName(job, "reply"), "never") == 0)
		{
			sendoutImmediate = false;
			return JOB_ACK | JOB_COMPLETE;
		}

		tmpVal = muXMLGetAttributeByName(job, "stx");
		if(tmpVal != NULL)
			tmpStx = atoi(tmpVal);
		else
			return 0;

		tmpVal = muXMLGetAttributeByName(job, "etx");
		if(tmpVal != NULL)
			tmpEtx = atoi(tmpVal);
		else
			return 0;

		if(strcmp(muXMLGetAttributeByName(job, "reply"), "onchange") == 0)
		{
			sendoutImmediateUid = uid;
			stx = tmpStx;
			etx = tmpEtx;
			sendoutImmediate = true;
			ret |= JOB_STORE;
		}
		else
		{
			readRequestUid = uid;
			stxSync = tmpStx;
			etxSync = tmpEtx;
			readRequest = true;
		}
	}
	if(strcmp(job->Element.Name, "write") == 0)
	{
		for(i=0; i<job->Data.DataSize; i++)
			vComPutChar(1, job->Data.Data[i], 0);
		ret | JOB_COMPLETE;
	}

	return ret | JOB_ACK;
}
