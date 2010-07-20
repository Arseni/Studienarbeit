/**
 * \addtogroup resolv
 * @{
 */
/**
 * \file
 * DNS resolver code header file.
 * \author Adam Dunkels <adam@dunkels.com>
 */

/*
 * Copyright (c) 2002-2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: resolv.h,v 1.4 2006/06/11 21:46:37 adam Exp $
 *
 */
#ifndef UDP_HANDLER_H
#define UDP_HANDLER_H

#include "uipopt.h"
#include "hw_types.h"

#define MAX_RX_HANDLERS	5

typedef struct
{
	u16_t rAddr[2];
	u16_t rPort;
	u16_t lPort;
}uip_udp_endpoint_t;

typedef void (* tOnSendComplete) (tBoolean success);
typedef void (* tOnReceiveComplete) (u8_t * data, int dataLen, uip_udp_endpoint_t sender);

typedef u16_t uip_udp_appstate_t[2];

void udpHandler_appcall();
#define UIP_UDP_APPCALL udpHandler_appcall

/* Functions. */
void udpHandler_init(uip_udp_endpoint_t endpoint);
tBoolean bUdpSendAsync(const unsigned char * data, int dataLen);
tBoolean bUdpSendTo(const unsigned char * data, int dataLen, uip_udp_endpoint_t dest);
tBoolean bUdpReceiveAsync(tOnReceiveComplete callback, int packages);
tBoolean bUdpCancelReceive(tOnReceiveComplete callback);

#endif /* UDP_HANDLER_H */

/** @} */
