#include "FreeRTOS.h"
#include "semphr.h"

#include "OLEDDisplay/oledDisplay.h"

#include "udpHandler.h"
#include "uip.h"

#include "units/unit.h"

#include <string.h>

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

static struct uip_udp_conn * c = NULL;
static xSemaphoreHandle xSmphrSendComplete;
static xQueueHandle xRxQueue = NULL;
static unsigned char * txBuffer = NULL;
static unsigned int txBufferLen = 0;
tBoolean syncRxFlag = false;

struct ReceiveCompleteCallbackHandler
{
	tOnReceiveComplete callback;
	int packsToReceive;
}rxCbHandler[MAX_RX_HANDLERS];

struct tData
{
	unsigned char * data;
	int dataLen;
	uip_ipaddr_t sender;
};

/**
 * Es muss noch eine Funktion gebaut werden, die fungiert für neue jobs
 * Die kann dann co-routines erstellen und löschen für periodische Jobs
 * das wars erstmal...
 */
static void vUdpNewData(unsigned char * pData, unsigned int uDataLen, uip_ipaddr_t * sender)
{
	int i;
	tUnitJob xJob;

	if(syncRxFlag)
	{
		struct tData enqueueData;
		enqueueData.data = pData;
		enqueueData.dataLen = uDataLen;
		uip_ipaddr_copy(&(enqueueData.sender), sender);
		if(xRxQueue != NULL)
			xQueueSend(xRxQueue, &enqueueData, 0);
		syncRxFlag = false;
	}

	for(i=0; i<MAX_RX_HANDLERS; i++)
	{
		if(rxCbHandler[i].packsToReceive != 0 && rxCbHandler[i].callback != NULL)
			rxCbHandler[i].callback(pData, uDataLen);
		if(rxCbHandler[i].packsToReceive > 0)
			rxCbHandler[i].packsToReceive--;
	}
	/* TODO callbacks abklappern
	if(memcmp(pData, "TEL:", 4) == 0)
	{
		pData += 4;
		strcpy(xJob.unitName, pData);
		strcpy(xJob.xCapability.Type, pData+strlen(pData)+1);

		// TODO: xJob = blblaextract();

		if(xJobQueue != NULL)
			xQueueSend(xJobQueue, &xJob, UNIT_MAX_WAITTIME_ON_FULL_QUEUE/portTICK_RATE_MS );
	}
	*/
}

static void vUdpCheckEntries(void)
{
	if(txBufferLen != 0 && txBuffer != NULL)
	{
		memcpy(uip_appdata, txBuffer, txBufferLen);
		uip_udp_send(txBufferLen);
	}
	txBufferLen = 0;
	txBuffer = NULL;
	if(xSmphrSendComplete != NULL)
		xSemaphoreGive(xSmphrSendComplete);
}



/** \internal
 * The main UDP function.
 */
void udpHandler_appcall(void)
{
	if(uip_poll())
	{
		vUdpCheckEntries();
	}
	if(uip_newdata())
	{
		vOledDbg(uip_appdata);
		vUdpNewData(uip_appdata, uip_datalen(), &(uip_udp_conn->ripaddr));
		//vUnitJobExtract(uip_appdata, uip_datalen());
		strcpy(uip_appdata, "OK");
		uip_udp_send(2);
	}
	/*
	if(uip_udp_conn->rport == HTONS(1001)) {
		if(uip_poll()) {
			check_entries();
		}
		if(uip_newdata()) {
			newdata();
		}
	}
	*/
}


/**
 * Initalize the udpHandler.
 */
void udpHandler_init()
{
	uip_ipaddr_t addr;

	vSemaphoreCreateBinary(xSmphrSendComplete);
	xSemaphoreGive(xSmphrSendComplete);
	xRxQueue = xQueueCreate(1, sizeof(struct tData));

	if(c != NULL)
		uip_udp_remove(c);

	uip_ipaddr(&addr, 192,168,10,222);//0xFF,0xFF,0xFF,0xFF); //0,0,0,0);
	c = uip_udp_new(&addr, HTONS(50001));

	if(c != NULL) {
		uip_udp_bind(c, HTONS(50001));
	}
}

/**
 * schickt Daten an buffer, der weiter an buffer und so weiter... am ende über UDP am Empfänger juhu
 */
tBoolean bUdpSendAsync(const unsigned char * data, int dataLen)//const tUnit * pUnit, const tUnitCapability * Capability, const tUnitValue value)
{
	if(xSmphrSendComplete != NULL)
		xSemaphoreTake(xSmphrSendComplete, portMAX_DELAY);
	else
		return false;

	txBuffer = (unsigned char*)data;
	txBufferLen = dataLen;
}

tBoolean bUdpRead(unsigned char * buffer, int * dataLen, uip_ipaddr_t * sender, portTickType timeout)
{
	syncRxFlag = true;
	struct tData xQElmnt;

	if(xRxQueue != NULL && xQueueReceive(xRxQueue, &xQElmnt, timeout/configTICK_RATE_HZ))
	{

		memcpy(buffer, xQElmnt.data, xQElmnt.dataLen);
		*dataLen = xQElmnt.dataLen;
		uip_ipaddr_copy(sender, &(xQElmnt.sender));
	}
	else
	{
		return false;
	}
}
