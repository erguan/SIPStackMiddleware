/*
* Copyright (C) 2009-2010 Mamadou Diop.
*
* Contact: Mamadou Diop <diopmamadou(at)doubango.org>
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

/**@file tsip_header_SIP_ETag.h
 * @brief SIP header 'SIP-ETag'.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *
 * @date Created: Sat Nov 8 16:54:58 2009 mdiop
 */
#ifndef _TSIP_HEADER_SIP_ETAG_H_
#define _TSIP_HEADER_SIP_ETAG_H_

#include "tinysip_config.h"
#include "tinysip/headers/tsip_header.h"

TSIP_BEGIN_DECLS


#define TSIP_HEADER_SIP_ETAG_VA_ARGS(etag)		tsip_header_SIP_ETag_def_t, (const char*)etag

/**
 * @struct	tsip_header_SIP_ETag_s
 *
 * 	SIP header 'SIP-ETag' as per RFC 3903.
 * 	@par ABNF 
 *	SIP-ETag	= 	"SIP-ETag" HCOLON entity-tag
 *	entity-tag = token 
**/
typedef struct tsip_header_SIP_ETag_s
{	
	TSIP_DECLARE_HEADER;
	char *value;
}
tsip_header_SIP_ETag_t;

TINYSIP_API tsip_header_SIP_ETag_t* tsip_header_SIP_ETag_create(const char* etag);
TINYSIP_API tsip_header_SIP_ETag_t* tsip_header_SIP_ETag_create_null();

TINYSIP_API tsip_header_SIP_ETag_t *tsip_header_SIP_ETag_parse(const char *data, tsk_size_t size);

TINYSIP_GEXTERN const tsk_object_def_t *tsip_header_SIP_ETag_def_t;

TSIP_END_DECLS

#endif /* _TSIP_HEADER_SIP_ETAG_H_ */

