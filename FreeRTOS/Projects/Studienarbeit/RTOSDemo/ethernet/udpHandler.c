#include "OLEDDisplay/oledDisplay.h"

#include "udpHandler.h"
#include "uip.h"

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
	if(uip_newdata())
	{
		vOledDbg1("UDP: port ", HTONS(uip_udp_conn->rport));
	}
	if(uip_udp_conn->rport == HTONS(1001)) {
		if(uip_poll()) {
			//check_entries();
		}
		if(uip_newdata()) {
			//newdata();
		}
	}
}

/*---------------------------------------------------------------------------*/
/**
 * Initalize the resolver.
 */
/*---------------------------------------------------------------------------*/
void
udpHandler_init(void)
{
	uip_ipaddr_t addr;
	struct uip_udp_conn *c;

	uip_ipaddr(&addr, 192,168,0,199);
	c = uip_udp_new(&addr, HTONS(0));

	if(c != NULL) {
		uip_udp_bind(c, HTONS(1101));
	}
}
/*---------------------------------------------------------------------------*/

/** @} */
/** @} */
