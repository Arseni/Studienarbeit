/**
 * \addtogroup apps
 * @{
 */

/**
 * \defgroup resolv DNS resolver
 * @{
 *
 * The uIP DNS resolver functions are used to lookup a hostname and
 * map it to a numerical IP address. It maintains a list of resolved
 * hostnames that can be queried with the resolv_lookup()
 * function. New hostnames can be resolved using the resolv_query()
 * function.
 *
 * When a hostname has been resolved (or found to be non-existant),
 * the resolver code calls a callback function called resolv_found()
 * that must be implemented by the module that uses the resolver.
 */

/**
 * \file
 * DNS host name to IP address resolver.
 * \author Adam Dunkels <adam@dunkels.com>
 *
 * This file implements a DNS host name to IP address resolver.
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
 * $Id: resolv.c,v 1.5 2006/06/11 21:46:37 adam Exp $
 *
 */

#include "resolv.h"
#include "uip.h"

#include <string.h>

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

static u8_t seqno;

static struct uip_udp_conn *resolv_conn = NULL;

/*---------------------------------------------------------------------------*/
/** \internal
 * The main UDP function.
 */
/*---------------------------------------------------------------------------*/
void
resolv_appcall(void)
{
  if(uip_udp_conn->rport == HTONS(53)) {
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
resolv_init(void)
{
}
/*---------------------------------------------------------------------------*/

/** @} */
/** @} */
