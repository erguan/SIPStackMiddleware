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

/**@file tsk_semaphore.h
 * @brief Pthread Semaphore.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *
 * @date Created: Sat Nov 8 16:54:58 2009 mdiop
 */
#ifndef _TINYSAK_SEMAPHORE_H_
#define _TINYSAK_SEMAPHORE_H_

#include "tinysak_config.h"

TSK_BEGIN_DECLS

typedef void tsk_semaphore_handle_t;

TINYSAK_API tsk_semaphore_handle_t* tsk_semaphore_create();
TINYSAK_API int tsk_semaphore_increment(tsk_semaphore_handle_t* handle);
TINYSAK_API int tsk_semaphore_decrement(tsk_semaphore_handle_t* handle);
TINYSAK_API void tsk_semaphore_destroy(tsk_semaphore_handle_t** handle);

TSK_END_DECLS

#endif /* _TINYSAK_SEMAPHORE_H_ */

