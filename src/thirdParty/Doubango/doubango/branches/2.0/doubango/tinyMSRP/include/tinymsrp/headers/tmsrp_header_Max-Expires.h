/*
* Copyright (C) 2009 Mamadou Diop.
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

/**@file tmsrp_header_Max_Expires.h
 * @brief MSRP 'Max-Expires' header.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *

 */
#ifndef _TMSRP_HEADER_MAX_EXPIRES_H_
#define _TMSRP_HEADER_MAX_EXPIRES_H_

#include "tinymsrp_config.h"
#include "tinymsrp/headers/tmsrp_header.h"

TMSRP_BEGIN_DECLS

#define TMSRP_HEADER_MAX_EXPIRES_VA_ARGS(value)		tmsrp_header_Max_Expires_def_t, (int64_t)value

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @struct	
///
/// @brief	MSRP 'Max-Expires' header.
///
/// @par ABNF :  Max-Expires	=  	"Max-Expires:" SP 1*DIGIT
///
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tmsrp_header_Max_Expires_s
{	
	TMSRP_DECLARE_HEADER;
	
	int64_t value;
}
tmsrp_header_Max_Expires_t;

typedef tsk_list_t tmsrp_headers_Max_Expires_L_t;


TINYMSRP_API tmsrp_header_Max_Expires_t* tmsrp_header_Max_Expires_create(int64_t value);
TINYMSRP_API tmsrp_header_Max_Expires_t* tmsrp_header_Max_Expires_create_null();

TINYMSRP_API tmsrp_header_Max_Expires_t *tmsrp_header_Max_Expires_parse(const char *data, tsk_size_t size);

TINYMSRP_GEXTERN const tsk_object_def_t *tmsrp_header_Max_Expires_def_t;

TMSRP_END_DECLS

#endif /* _TMSRP_HEADER_MAX_EXPIRES_H_ */

