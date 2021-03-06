/*
    FreeRTOS V6.0.4 - Copyright (C) 2010 Real Time Engineers Ltd.

    ***************************************************************************
    *                                                                         *
    * If you are:                                                             *
    *                                                                         *
    *    + New to FreeRTOS,                                                   *
    *    + Wanting to learn FreeRTOS or multitasking in general quickly       *
    *    + Looking for basic training,                                        *
    *    + Wanting to improve your FreeRTOS skills and productivity           *
    *                                                                         *
    * then take a look at the FreeRTOS eBook                                  *
    *                                                                         *
    *        "Using the FreeRTOS Real Time Kernel - a Practical Guide"        *
    *                  http://www.FreeRTOS.org/Documentation                  *
    *                                                                         *
    * A pdf reference manual is also available.  Both are usually delivered   *
    * to your inbox within 20 minutes to two hours when purchased between 8am *
    * and 8pm GMT (although please allow up to 24 hours in case of            *
    * exceptional circumstances).  Thank you for your support!                *
    *                                                                         *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    ***NOTE*** The exception to the GPL is included to allow you to distribute
    a combined work that includes FreeRTOS without being obliged to provide the
    source code for proprietary components outside of the FreeRTOS kernel.
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public 
    License and the FreeRTOS license exception along with FreeRTOS; if not it 
    can be viewed here: http://www.freertos.org/a00114.html and also obtained 
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/
/* Standard includes. */
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "OLEDDisplay/oledDisplay.h"

/* uip includes. */
#include "hw_types.h"
#include "uip.h"
#include "uip_arp.h"
#include "uip_timer.h"
#include "clock-arch.h"
#include "hw_ethernet.h"
#include "ethernet.h"
#include "hw_memmap.h"
#include "sysctl.h"

/* Demo includes. */
#include "emac.h"
#include "tcpHandler.h"

/*-----------------------------------------------------------*/

/* IP address configuration. */
#define uipIP_ADDR0		192
#define uipIP_ADDR1		168
#define uipIP_ADDR2		0 //10
#define uipIP_ADDR3		13 //227

/* How long to wait before attempting to connect the MAC again. */
#define uipINIT_WAIT    100

/* Shortcut to the header within the Rx buffer. */
#define xHeader ((struct uip_eth_hdr *) &uip_buf[ 0 ])

/* Standard constant. */
#define uipTOTAL_FRAME_HEADER_SIZE	54

/*-----------------------------------------------------------*/

/*
 * Port functions required by the uIP stack.
 */
void clock_init( void );
clock_time_t clock_time( void );

/*-----------------------------------------------------------*/

/* The semaphore used by the ISR to wake the uIP task. */
extern xSemaphoreHandle xEMACSemaphore;

/*-----------------------------------------------------------*/

void clock_init(void)
{
	/* This is done when the scheduler starts. */
}
/*-----------------------------------------------------------*/

clock_time_t clock_time( void )
{
	return xTaskGetTickCount();
}

/*
 * Setup the MAC address in the MAC itself, and in the uIP stack.
 */
void prvSetMACAddress( void )
{
unsigned portLONG ulUser0, ulUser1;
unsigned char pucMACArray[8];
struct uip_eth_addr xAddr;

	/* Get the device MAC address from flash */
    FlashUserGet(&ulUser0, &ulUser1);

	/* Convert the MAC address from flash into sequence of bytes. */
    pucMACArray[0] = ((ulUser0 >>  0) & 0xff);
    pucMACArray[1] = ((ulUser0 >>  8) & 0xff);
    pucMACArray[2] = ((ulUser0 >> 16) & 0xff);
    pucMACArray[3] = ((ulUser1 >>  0) & 0xff);
    pucMACArray[4] = ((ulUser1 >>  8) & 0xff);
    pucMACArray[5] = ((ulUser1 >> 16) & 0xff);

	/* Program the MAC address. */
    EthernetMACAddrSet(ETH_BASE, pucMACArray);

	xAddr.addr[ 0 ] = pucMACArray[0];
	xAddr.addr[ 1 ] = pucMACArray[1];
	xAddr.addr[ 2 ] = pucMACArray[2];
	xAddr.addr[ 3 ] = pucMACArray[3];
	xAddr.addr[ 4 ] = pucMACArray[4];
	xAddr.addr[ 5 ] = pucMACArray[5];
	uip_setethaddr( xAddr );
}

void vEthernetTask( void *pvParameters )
{
	portBASE_TYPE i;
	uip_ipaddr_t xIPAddr;
	struct timer periodic_timer, arp_timer;

	/* Enable/Reset the Ethernet Controller */
	SysCtlPeripheralEnable( SYSCTL_PERIPH_ETH );
	SysCtlPeripheralReset( SYSCTL_PERIPH_ETH );

	/* Create the semaphore used by the ISR to wake this task. */
	vSemaphoreCreateBinary( xEMACSemaphore );
	
	/* Initialise the uIP stack. */
	timer_set( &periodic_timer, configTICK_RATE_HZ / 2 );
	timer_set( &arp_timer, configTICK_RATE_HZ * 10 );
	uip_init();
	uip_ipaddr( xIPAddr, uipIP_ADDR0, uipIP_ADDR1, uipIP_ADDR2, uipIP_ADDR3 );
	uip_sethostaddr( xIPAddr );
	tcpHandler_init();
	udpHandler_init();

	while( vInitEMAC() != pdPASS )
    {
        vTaskDelay( uipINIT_WAIT );
    }
	prvSetMACAddress();	
	

	for( ;; )
	{
		/* Is there received data ready to be processed? */
		uip_len = uiGetEMACRxData( uip_buf );
		
		if( uip_len > 0 )
		{
			/* Standard uIP loop taken from the uIP manual. */

			if( xHeader->type == htons( UIP_ETHTYPE_IP ) )
			{
				uip_arp_ipin();
				uip_input();

				/* If the above function invocation resulted in data that
				should be sent out on the network, the global variable
				uip_len is set to a value > 0. */
				if( uip_len > 0 )
				{
					uip_arp_out();
					prvENET_Send();
				}
			}
			else if( xHeader->type == htons( UIP_ETHTYPE_ARP ) )
			{
				uip_arp_arpin();

				/* If the above function invocation resulted in data that
				should be sent out on the network, the global variable
				uip_len is set to a value > 0. */
				if( uip_len > 0 )
				{
					prvENET_Send();
				}
			}
		}
		else
		{
			if( timer_expired( &periodic_timer ) )
			{
				timer_reset( &periodic_timer );
				// Handle all TCP connections
				for( i = 0; i < UIP_CONNS; i++ )
				{
					uip_periodic( i );
	
					/* If the above function invocation resulted in data that
					should be sent out on the network, the global variable
					uip_len is set to a value > 0. */
					if( uip_len > 0 )
					{
						uip_arp_out();
						prvENET_Send();
					}
				}	

				// Handle all UDP connections
				for( i = 0; i < UIP_UDP_CONNS; i++ )
				{
					uip_udp_periodic( i );
					if( uip_len > 0 )
					{
						uip_arp_out();
						prvENET_Send();
					}
				}
			}
			else if( timer_expired( &arp_timer ) )
			{
				/* Call the ARP timer function every 10 seconds. */
				timer_reset( &arp_timer );
				uip_arp_timer();
			}
			else
			{
				/* We did not receive a packet, and there was no periodic
				processing to perform.  Block for a fixed period.  If a packet
				is received during this period we will be woken by the ISR
				giving us the Semaphore. */
				xSemaphoreTake( xEMACSemaphore, configTICK_RATE_HZ / 2 );			
			}
		}
	}
}
/*-----------------------------------------------------------*/

