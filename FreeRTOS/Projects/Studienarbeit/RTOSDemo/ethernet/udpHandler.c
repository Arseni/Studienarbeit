#include "OLEDDisplay/oledDisplay.h"

#include "udpHandler.h"
#include "uip.h"

#include "units/unit.h"

#include <string.h>

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

static u8_t seqno;

static struct uip_udp_conn *resolv_conn = NULL;

/** \internal
 * The main UDP function.
 */
void
udpHandler_appcall(void)
{
	if(uip_poll())
	{
		vUnitCheckUdpEntries();
	}
	if(uip_newdata())
	{
		//vOledDbg1("UDP: port ", HTONS(uip_udp_conn->rport));
		vUnitNewUdpData(uip_appdata, uip_datalen(), &(uip_udp_conn->ripaddr));
		//vUnitJobExtract(uip_appdata, uip_datalen());
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

/*---------------------------------------------------------------------------*/
/**
 * Initalize the resolver.
 */
/*---------------------------------------------------------------------------*/
static struct uip_udp_conn * c = NULL;

void udpHandler_init(u8_t addr0, u8_t addr1, u8_t addr2, u8_t addr3, u16_t rPort, u16_t lPort)
{
	uip_ipaddr_t addr;

	if(c != NULL)
		uip_udp_remove(c);

	uip_ipaddr(&addr, addr0,addr1,addr2,addr3);//192,168,10,222);//0xFF,0xFF,0xFF,0xFF); //0,0,0,0);
	c = uip_udp_new(&addr, HTONS(rPort));

	if(c != NULL) {
		uip_udp_bind(c, HTONS(lPort));
	}
}
/*---------------------------------------------------------------------------*/

/** @} */
/** @} */
