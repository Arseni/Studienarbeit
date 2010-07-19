#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "hw_types.h"
#include "hw_memmap.h"
#include "ethernet.h"

#include "OLEDDisplay/oledDisplay.h"

#include "udpHandler.h"
#include "uip.h"

#include "units/unit.h"

#include <string.h>

extern xSemaphoreHandle xEMACSemaphore;

static struct uip_udp_conn * c = NULL;
static xSemaphoreHandle xSmphrSendComplete = NULL;
static unsigned char * txBuffer = NULL;
static unsigned int txBufferLen = 0;

static struct ReceiveCompleteCallbackHandler
{
	tOnReceiveComplete callback;
	int packsToReceive;
}rxCbHandler[MAX_RX_HANDLERS];

/**
 * Wird aufgerufen, wenn neue Daten über UDP angekommen sind [EthernetTask]
 */
static void vUdpNewData()
{
	int i;
	uip_udp_endpoint_t sender;
	uip_ipaddr_copy(sender.rAddr, uip_udp_conn->appstate);
	sender.lPort = HTONS(uip_udp_conn->lport);
	sender.rPort = HTONS(uip_udp_conn->rport);

	for(i=0; i<MAX_RX_HANDLERS; i++)
	{
		if(rxCbHandler[i].packsToReceive != 0 && rxCbHandler[i].callback != NULL)
			rxCbHandler[i].callback(uip_appdata, uip_datalen(), sender);
		if(rxCbHandler[i].packsToReceive > 0)
			rxCbHandler[i].packsToReceive--;
	}
}

/**
 * Wird aufgerufen, wenn der timer im ethernet task abläuft. Verschickt ggf. Nachrichten
 */
static void vUdpCheckEntries(void)
{
	if(txBufferLen != 0 && txBuffer != NULL)
	{
		memcpy(uip_appdata, txBuffer, txBufferLen);
		uip_udp_send(txBufferLen);
		txBufferLen = 0;
		txBuffer = NULL;
		if(xSmphrSendComplete != NULL)
			xSemaphoreGive(xSmphrSendComplete);
	}
}



/** \internal
 * The main UDP function.
 */
void udpHandler_appcall()
{
	if(uip_poll())
	{
		vUdpCheckEntries();
	}
	if(uip_newdata())
	{
		vOledDbg(uip_appdata);
		vUdpNewData();
	}
//	EthernetIntEnable( ETH_BASE, ETH_INT_RX );
}


/**
 * Initalize the udpHandler.
 */
void udpHandler_init(uip_udp_endpoint_t endpoint)
{
	//uip_ipaddr_t addr;

	if(c != NULL)
	{
		uip_udp_remove(c);
	}

	//uip_ipaddr(&addr, 192,168,0,194);//10,222);//0xFF,0xFF,0xFF,0xFF); //0,0,0,0);
	c = uip_udp_new(&(endpoint.rAddr), HTONS(endpoint.rPort));

	if(c != NULL) {
		uip_udp_bind(c, HTONS(endpoint.lPort));
	}

	if(xSmphrSendComplete == NULL)
	{
		vSemaphoreCreateBinary(xSmphrSendComplete);
		xSemaphoreGive(xSmphrSendComplete);
	}
}


/* -------------------------------   public API   ------------------------------- */

/**
 * schickt Daten an buffer, der weiter an buffer und so weiter... am ende über UDP am Empfänger juhu
 */
tBoolean bUdpSendAsync(const unsigned char * data, int dataLen)//const tUnit * pUnit, const tUnitCapability * Capability, const tUnitValue value)
{
	if(xSmphrSendComplete != NULL && xSemaphoreTake(xSmphrSendComplete, portMAX_DELAY))
	{
		txBuffer = (unsigned char*)data;
		txBufferLen = dataLen;

		// force send instantly
		if(c != NULL)
		{
			uip_udp_periodic_conn( c );
			if( uip_len > 0 )
			{
				uip_arp_out();
				prvENET_Send();
			}
		}
		else
			return false;
	}
	else
	{
		return false;
	}

	return true;
}

tBoolean bUdpReceiveAsync(tOnReceiveComplete callback, int packages)
{
	int i;
	for(i=0; i<MAX_RX_HANDLERS; i++)
	{
		if(rxCbHandler[i].packsToReceive == 0)
		{
			rxCbHandler[i].callback = callback;
			rxCbHandler[i].packsToReceive = packages;
			return true;
		}
	}
	return false;
}
tBoolean bUdpCancelReceive(tOnReceiveComplete callback)
{
	int i;
	for(i=0; i<MAX_RX_HANDLERS; i++)
	{
		if(rxCbHandler[i].callback == callback)
		{
			rxCbHandler[i].packsToReceive = 0;
			rxCbHandler[i].callback = NULL;
			return true;
		}
	}
	return false;
}
