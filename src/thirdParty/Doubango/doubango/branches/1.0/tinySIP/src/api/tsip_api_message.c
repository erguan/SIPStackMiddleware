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

/**@file tsip_api_message.c
 * @brief Public short messaging (MESSAGE) functions.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *
 * @date Created: Sat Nov 8 16:54:58 2009 mdiop
 */
#include "tinysip/api/tsip_api_message.h"

#include "tinysip/dialogs/tsip_dialog_layer.h"
#include "tinysip/dialogs/tsip_dialog_message.h"

#include "tsip.h"

#include "tsk_runnable.h"
#include "tsk_debug.h"

#define TSIP_MESSAGE_EVENT_CREATE( type)		tsk_object_new(tsip_message_event_def_t, type)

extern tsip_action_t* _tsip_action_create(tsip_action_type_t type, va_list* app);

int tsip_message_event_signal(tsip_message_event_type_t type, tsip_ssession_handle_t* ss, short status_code, const char *phrase, const tsip_message_t* sipmessage)
{
	tsip_message_event_t* sipevent = TSIP_MESSAGE_EVENT_CREATE(type);
	tsip_event_init(TSIP_EVENT(sipevent), ss, status_code, phrase, sipmessage, tsip_event_message);

	TSK_RUNNABLE_ENQUEUE_OBJECT(TSK_RUNNABLE(TSIP_SSESSION(ss)->stack), sipevent);

	return 0;
}

int tsip_action_MESSAGE(const tsip_ssession_handle_t *ss, ...)
{
	const tsip_ssession_t* _ss;
	va_list ap;
	tsip_action_t* action;
	tsip_dialog_t* dialog;
	int ret = -1;

	if(!(_ss = ss) || !_ss->stack){
		TSK_DEBUG_ERROR("Invalid parameter.");
		return ret;
	}
	
	/* Checks if the stack has been started */
	if(!TSK_RUNNABLE(_ss->stack)->started){
		TSK_DEBUG_ERROR("Stack not started.");
		return -2;
	}

	/* action */
	va_start(ap, ss);
	if((action = _tsip_action_create(tsip_atype_message_send, &ap))){
		if(!(dialog = tsip_dialog_layer_find_by_ss(_ss->stack->layer_dialog, ss))){
			dialog = tsip_dialog_layer_new(_ss->stack->layer_dialog, tsip_dialog_MESSAGE, ss);
		}
		ret = tsip_dialog_fsm_act(dialog, action->type, tsk_null, action);
		
		tsk_object_unref(dialog);
		TSK_OBJECT_SAFE_FREE(action);
	}
	va_end(ap);

	return ret;
}













//========================================================
//	SIP MESSAGE event object definition
//
static tsk_object_t* tsip_message_event_ctor(tsk_object_t * self, va_list * app)
{
	tsip_message_event_t *sipevent = self;
	if(sipevent){
		sipevent->type = va_arg(*app, tsip_message_event_type_t);
	}
	return self;
}

static tsk_object_t* tsip_message_event_dtor(tsk_object_t * self)
{ 
	tsip_message_event_t *sipevent = self;
	if(sipevent){
		tsip_event_deinit(TSIP_EVENT(sipevent));
	}
	return self;
}

static int tsip_message_event_cmp(const tsk_object_t *obj1, const tsk_object_t *obj2)
{
	return -1;
}

static const tsk_object_def_t tsip_message_event_def_s = 
{
	sizeof(tsip_message_event_t),
	tsip_message_event_ctor, 
	tsip_message_event_dtor,
	tsip_message_event_cmp, 
};
const tsk_object_def_t *tsip_message_event_def_t = &tsip_message_event_def_s;
