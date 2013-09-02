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
/**@file tdav_win32.c
 * @brief tinyDAV WIN32 helper functions.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *
 * @date Created: Sat Nov 8 16:54:58 2009 mdiop
 */
#include "tinydav/tdav_win32.h"

#if TDAV_UNDER_WINDOWS

#include "tsk_debug.h"

void tdav_win32_print_error(const char* func, HRESULT hr)
{
	CHAR* message = tsk_null;
#ifdef _WIN32_WCE
	FormatMessage
#else
	FormatMessageA
#endif
	(
	  FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
	  tsk_null,
	  hr,
	  0,
	  message, 
	  0,
	  tsk_null);

	TSK_DEBUG_ERROR("%s():", message);

	LocalFree(message);
}


#endif /* TDAV_UNDER_WINDOWS */