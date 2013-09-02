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

/**@file tsdp_header_M.h
 * @brief SDP "m=" header (Media Descriptions).
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *
 * @date Created: Iat Nov 8 16:54:58 2009 mdiop
 */
#ifndef _TSDP_HEADER_M_H_
#define _TSDP_HEADER_M_H_

#include "tinysdp_config.h"
#include "tinysdp/headers/tsdp_header.h"

#include "tinysdp/headers/tsdp_header_A.h"
#include "tinysdp/headers/tsdp_header_B.h"
#include "tinysdp/headers/tsdp_header_C.h"
#include "tinysdp/headers/tsdp_header_I.h"
#include "tinysdp/headers/tsdp_header_K.h"

#include "tsk_string.h"

TSDP_BEGIN_DECLS

#define TSDP_HEADER_M_VA_ARGS(media, port, proto)		tsdp_header_M_def_t, (const char*)media, (uint32_t)port, (const char*)proto

#define TSDP_HEADER_M(self)					((tsdp_header_M_t*)(self))

#define TSDP_FMT_VA_ARGS(fmt)				tsdp_fmt_def_t, (const char*)fmt

typedef tsk_string_t tsdp_fmt_t;
typedef tsk_strings_L_t tsk_fmts_L_t;
#define tsdp_fmt_def_t tsk_string_def_t
#define TSDP_FMT_STR(self) TSK_STRING_STR(self)

TINYSDP_API tsdp_fmt_t* tsdp_fmt_create(const char* fmt);

//#define TSDP_HEADER_M_SET_FMT(fmt)			(int)0x01, (const char*)fmt
//#define TSDP_HEADER_M_SET_A(field, value)	(int)0x02, (const char*)field, (const char*)value
//#define TSDP_HEADER_M_SET_NULL()			(int)0x00

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @struct	
///
/// @brief	SDP "m=" header (Media Descriptions).
///
/// @par ABNF :  m=*(media-field
/// information-field
/// *connection-field
/// bandwidth-fields
/// key-field
/// attribute-fields)
///
/// media-field	=  	%x6d "=" media SP port ["/" integer] SP proto 1*(SP fmt) CRLF 
/// media	=  	token 
/// port	=  	1*DIGIT
/// proto	=  	token *("/" token) 
/// fmt	=  	token 
/// 	
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tsdp_header_M_s
{	
	TSDP_DECLARE_HEADER;
	
	char* media;
	uint32_t port;
	uint32_t nports; /**< <number of ports> */
	char* proto;
	tsk_fmts_L_t* FMTs;
	
	// Fields below will be set by the message parser.
	tsdp_header_I_t* I;
	tsdp_header_C_t* C;
	tsdp_headers_B_L_t* Bandwidths; // (zero or more bandwidth information lines)
	tsdp_header_K_t* K; // (encryption key)
	tsdp_headers_A_L_t* Attributes; // (zero or more media attribute lines)
}
tsdp_header_M_t;

typedef tsk_list_t tsdp_headers_M_L_t;

TINYSDP_API tsdp_header_M_t* tsdp_header_M_create(const char* media, uint32_t port, const char* proto);
TINYSDP_API tsdp_header_M_t* tsdp_header_M_create_null();

TINYSDP_API tsdp_header_M_t *tsdp_header_M_parse(const char *data, tsk_size_t size);
TINYSDP_API int tsdp_header_M_add(tsdp_header_M_t* self, const tsdp_header_t* header);
TINYSDP_API int tsdp_header_M_add_headers(tsdp_header_M_t* self, ...);
TINYSDP_API int tsdp_header_M_add_headers_2(tsdp_header_M_t* self, const tsdp_headers_L_t* headers);
TINYSDP_API int tsdp_header_M_add_fmt(tsdp_header_M_t* self, const char* fmt);
TINYSDP_API const tsdp_header_A_t* tsdp_header_M_findA_at(const tsdp_header_M_t* self, const char* field, tsk_size_t index);
TINYSDP_API const tsdp_header_A_t* tsdp_header_M_findA(const tsdp_header_M_t* self, const char* field);
TINYSDP_API char* tsdp_header_M_get_rtpmap(const tsdp_header_M_t* self, const char* fmt);
TINYSDP_API char* tsdp_header_M_get_fmtp(const tsdp_header_M_t* self, const char* fmt);
TINYSDP_API int tsdp_header_M_hold(tsdp_header_M_t* self, tsk_bool_t local);
TINYSDP_API tsk_bool_t tsdp_header_M_is_held(const tsdp_header_M_t* self, tsk_bool_t local);
TINYSDP_API int tsdp_header_M_resume(tsdp_header_M_t* self, tsk_bool_t local);

TINYSDP_GEXTERN const tsk_object_def_t *tsdp_header_M_def_t;

TSDP_END_DECLS

#endif /* _TSDP_HEADER_M_H_ */

