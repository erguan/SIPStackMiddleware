/*
* Copyright (C) 2010-2011 Mamadou Diop.
*
* Contact: Mamadou Diop <diopmamadou(at)doubango[dot]org>
*	
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* DOUBANGO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*
*/
/**@file tnet_dns_soa.c
 * @brief DNS Start Of Authority record - RR - (RFC 1035).
 *
 * @author Mamadou Diop <diopmamadou(at)doubango[dot]org>
 *

 */
#include "tnet_dns_soa.h"

#include "../tnet_types.h"
#include "../tnet_endianness.h"

#include "tsk_string.h"
#include "tsk_memory.h"

tnet_dns_soa_t* tnet_dns_soa_create(const char* name, tnet_dns_qclass_t qclass, uint32_t ttl, uint16_t rdlength, const void* data, tsk_size_t offset)
{
	return tsk_object_new(tnet_dns_soa_def_t, name, qclass, ttl, rdlength, data, offset);
}

//=================================================================================================
//	[[DNS SOA]] object definition
//
static tsk_object_t* tnet_dns_soa_ctor(tsk_object_t * self, va_list * app)
{
	tnet_dns_soa_t *soa = self;
	if(soa){
		const char* name = va_arg(*app, const char*);
		tnet_dns_qclass_t qclass = va_arg(*app, tnet_dns_qclass_t);
		uint32_t ttl = va_arg(*app, uint32_t);
#if defined(__GNUC__)
		uint16_t rdlength = (uint16_t)va_arg(*app, unsigned);
#else
		uint16_t rdlength = va_arg(*app, uint16_t);
#endif
		const void* data = va_arg(*app, const void*);
		tsk_size_t offset = va_arg(*app, tsk_size_t);

		/* init base */
		tnet_dns_rr_init(TNET_DNS_RR(soa), qtype_soa, qclass);
		TNET_DNS_RR(soa)->name = tsk_strdup(name);
		TNET_DNS_RR(soa)->rdlength = rdlength;
		TNET_DNS_RR(soa)->ttl = ttl;

		if(rdlength)
		{	// ==> DESERIALIZATION
			/* MNAME */
			tnet_dns_rr_qname_deserialize(data, &(soa->mname), &offset);
			/* RNAME */
			tnet_dns_rr_qname_deserialize(data, &(soa->rname), &offset);
			/* SERIAL */
			soa->serial = tnet_htonl_2(((uint8_t*)data) + offset),
			offset += 2;
			/* REFRESH */
			soa->refresh = tnet_htonl_2(((uint8_t*)data) + offset),
			offset += 2;
			/* RETRY */
			soa->retry = tnet_htonl_2(((uint8_t*)data) + offset),
			offset += 2;
			/* EXPIRE */
			soa->expire = tnet_htonl_2(((uint8_t*)data) + offset),
			offset += 2;
			/* MINIMUM */
			soa->minimum = tnet_htonl_2(((uint8_t*)data) + offset),
			offset += 2;
		}
	}
	return self;
}

static tsk_object_t* tnet_dns_soa_dtor(tsk_object_t * self) 
{ 
	tnet_dns_soa_t *soa = self;
	if(soa){
		/* deinit base */
		tnet_dns_rr_deinit(TNET_DNS_RR(soa));

		TSK_FREE(soa->mname);
		TSK_FREE(soa->rname);
	}
	return self;
}

static const tsk_object_def_t tnet_dns_soa_def_s =
{
	sizeof(tnet_dns_soa_t),
	tnet_dns_soa_ctor,
	tnet_dns_soa_dtor,
	tsk_null,
};
const tsk_object_def_t *tnet_dns_soa_def_t = &tnet_dns_soa_def_s;
