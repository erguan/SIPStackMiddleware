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

/**@file thttp_parser_url.h
 * @brief HTTP/HTTPS URL parser.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango[dot]org>
 *

 */
#ifndef TINYHTTP_PARSER_URL_H
#define TINYHTTP_PARSER_URL_H

#include "tinyhttp_config.h"
#include "tinyhttp/thttp_url.h"

#include "tsk_ragel_state.h"

THTTP_BEGIN_DECLS

TINYHTTP_API thttp_url_t *thttp_url_parse(const char *data, tsk_size_t size);

THTTP_END_DECLS

#endif /* TINYHTTP_PARSER_URL_H */

