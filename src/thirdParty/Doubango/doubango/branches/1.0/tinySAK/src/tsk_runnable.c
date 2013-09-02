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

/**@file tsk_runnable.c
 * @brief Base class for runnable object.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *
 * @date Created: Sat Nov 8 16:54:58 2009 mdiop
 */
#include "tsk_runnable.h"
#include "tsk_thread.h"
#include "tsk_debug.h"

/**@defgroup tsk_runnable_group Base class for runnable object.
*/

/**@ingroup tsk_runnable_group
* Creates new Runnable object.
* @retval @ref tsk_runnable_t.
*/
tsk_runnable_t* tsk_runnable_create()
{
	return tsk_object_new(tsk_runnable_def_t);
}

/**@ingroup tsk_runnable_group
* Initializes a runnable object and allocate it's internal fields.
* @param self The runnable object to initialize.
* @param objdef Internal objects definition class.
* @retval Zero if succeed and nonzero error code otherwise.
* @sa tsk_runnable_deinit.
*/
static int tsk_runnable_init(tsk_runnable_t *self, const tsk_object_def_t *objdef)
{
	if(self && objdef){
		if(self->initialized){
			TSK_DEBUG_ERROR("Already initialized");
			return -2;
		}
		
		self->semaphore = tsk_semaphore_create();
		self->objdef = objdef;
		self->objects = tsk_list_create();

		self->initialized = tsk_true;
		return 0;
	}
	TSK_DEBUG_ERROR("Invalid Parameter");
	return -1;
}

/**@ingroup tsk_runnable_group
* DeInitializes a runnable object.
* @param self The runnable object to deinitialize.
* @retval Zero if succeed and nonzero error code otherwise.
* @sa tsk_runnable_init.
*/
static int tsk_runnable_deinit(tsk_runnable_t *self)
{
	if(self){
		if(!self->initialized){
			return 0; /* Already deinitialized */
		}
		else if(self->running){
			TSK_DEBUG_ERROR("Cannot deinit a runnable object while running.");
			return -3;
		}

		tsk_semaphore_destroy(&self->semaphore);
		TSK_OBJECT_SAFE_FREE(self->objects);

		self->initialized = tsk_false;

		return 0;
	}
	TSK_DEBUG_ERROR("Invalid parameter");
	return -1;
}

/**@ingroup tsk_runnable_group
* Starts a runnable object.
* @param self The runnable object to start.
* @param objdef Internal objects definition (meta-data) class.
* @retval Zero if succeed and nonzero error code otherwise.
* @sa tsk_runnable_stop.
*/
int tsk_runnable_start(tsk_runnable_t *self, const tsk_object_def_t *objdef)
{
	if(self){
		int ret = -1;
		if(self->running) return -2;
		else if(!self->run) return -3;
		else if(tsk_runnable_init(self, objdef)) return -4;

		/* call run() function in new thread. */
		if((ret = tsk_thread_create(&(self->tid[0]), self->run, self))){
			TSK_DEBUG_ERROR("Failed to start new thread.");
			return ret;
		}
		// Do not set "running" to true here
		// Problem: When you try to stop the thread before it start
		// Will be done by "TSK_RUNNABLE_RUN_BEGIN" which is called into the thread
		//self->running = tsk_true;

		self->started = tsk_true;
		return 0;
	}
	else{
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
}

/**@ingroup tsk_runnable_group
* To indicate whether the enqueued data is important or not.
* @param self The runnable object
* @param important @a tsk_true or @a tsk_false. If set to @a tsk_true, the thread will not be joined 
* until all data in the queue have been consumed.
*/
int tsk_runnable_set_important(tsk_runnable_t *self, tsk_bool_t important)
{
	if(self){
		self->important = important;
		return 0;
	}
	else{
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
}

/**@ingroup tsk_runnable_group
* Stops a runnable object.
* @param self The runnable object to stop.
* @retval Zero if succeed and nonzero error code otherwise.
* @sa tsk_runnable_start.
*/
int tsk_runnable_stop(tsk_runnable_t *self)
{
	int ret = -1;
	if(self){
		if(!self->initialized) {
			if(!self->running){
				/* already deinitialized */
				return 0;
			}
			else{
				/* should never happen */
				TSK_DEBUG_ERROR("Not initialized.");
				return -2;
			}
		}
		else if(!self->running) {

			if(self->started){
				tsk_size_t count = 0;
				/* Thread is started but not running ==> Give it time.*/
				while(++count <= 5){
					tsk_thread_sleep(count * 200);
					if(self->running){
						goto stop;
					}
				}
			}
			else{
				return 0; /* already stopped */
			} 
			return -3;
		}

stop:
		self->running = tsk_false;
		tsk_semaphore_increment(self->semaphore);

		if((ret = tsk_thread_join(&(self->tid[0])))){
			self->running = tsk_true;
			TSK_DEBUG_ERROR("Failed to join the thread.");
			return ret;
		}
		tsk_runnable_deinit(self);

		self->started = tsk_false;
	}
	return ret;
}

//=================================================================================================
//	Runnable object definition
//
static tsk_object_t* tsk_runnable_ctor(tsk_object_t * self, va_list * app)
{
	tsk_runnable_t* runnable = self;
	if(runnable){
	}
	return self;
}

static tsk_object_t* tsk_runnable_dtor(tsk_object_t * self)
{ 
	tsk_runnable_t* runnable = self;
	if(runnable){
		/* stops runnable object (if running or started) */
		tsk_runnable_stop(runnable);
	}
	return self;
}

static const tsk_object_def_t tsk_runnable_def_s = 
{
	sizeof(tsk_runnable_t),
	tsk_runnable_ctor, 
	tsk_runnable_dtor,
	tsk_null, 
};
const tsk_object_def_t *tsk_runnable_def_t = &tsk_runnable_def_s;

