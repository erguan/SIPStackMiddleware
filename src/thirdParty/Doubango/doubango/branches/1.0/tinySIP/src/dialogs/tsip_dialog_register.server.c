/*
* Copyright (C) 2009-2010 Mamadou Diop.
*
* Contact: Mamadou Diop <diopmamadou(at)doubango.org>
*	
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as publishd by
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
#include "tinysip/dialogs/tsip_dialog_register.h"
#include "tinysip/dialogs/tsip_dialog_register.common.h"


/* ======================== external functions ======================== */
extern int tsip_dialog_register_send_RESPONSE(tsip_dialog_register_t *self, const tsip_request_t* request, short code, const char* phrase);

/* ======================== transitions ======================== */
int s0000_Started_2_Terminated_X_iREGISTER(va_list *app);
int s0000_Started_2_Incoming_X_iREGISTER(va_list *app);
int s0000_Incoming_2_Connected_X_Accept(va_list *app);
int s0000_Incoming_2_Terminated_X_Reject(va_list *app);
int s0000_Connected_2_Terminated_X_iREGISTER(va_list *app);


/* ======================== conds ======================== */
static tsk_bool_t _fsm_cond_not_served_here(tsip_dialog_register_t* dialog, tsip_message_t* message)
{
	if(message && TSIP_REQUEST_IS_REGISTER(message)){
		if(tsk_object_cmp(TSIP_DIALOG_GET_STACK(dialog)->network.realm, message->line.request.uri) != 0){
			tsip_dialog_register_send_RESPONSE(dialog, TSIP_MESSAGE_AS_REQUEST(message), 404, "Domain not served here");
			return tsk_true;
		}
	}
	return tsk_false;
}
static tsk_bool_t _fsm_cond_server_unregistering(tsip_dialog_register_t* dialog, tsip_message_t* message)
{
	if(message && dialog->is_server){
		int64_t expires = tsip_message_getExpires(message);
		dialog->unregistering = (expires == 0);
		return dialog->unregistering;
	}
	return tsk_false;
}
static tsk_bool_t _fsm_cond_server_registering(tsip_dialog_register_t* dialog, tsip_message_t* message)
{
	return !_fsm_cond_server_unregistering(dialog, message);
}


int tsip_dialog_register_server_init(tsip_dialog_register_t *self)
{
	return tsk_fsm_set(TSIP_DIALOG_GET_FSM(self),
	
		/*=======================
		* === Started === 
		*/
		// Started -> (Domain Not Served here) -> Terminated
		TSK_FSM_ADD(_fsm_state_Started, _fsm_action_iREGISTER, _fsm_cond_not_served_here, _fsm_state_Terminated, s0000_Started_2_Terminated_X_iREGISTER, "s0000_Started_2_Terminated_X_iREGISTER"),
		// Started -> (All is OK and we are not unRegistering) -> Trying
		TSK_FSM_ADD(_fsm_state_Started, _fsm_action_iREGISTER, _fsm_cond_server_registering, _fsm_state_Incoming, s0000_Started_2_Incoming_X_iREGISTER, "s0000_Started_2_Incoming_X_iREGISTER"),

		/*=======================
		* === Incoming === 
		*/
		// Incoming -> (Accept) -> Connected
		TSK_FSM_ADD_ALWAYS(_fsm_state_Incoming, _fsm_action_accept, _fsm_state_Connected, s0000_Incoming_2_Connected_X_Accept, "s0000_Incoming_2_Connected_X_Accept"),
		// Incoming -> (Reject) -> Terminated
		TSK_FSM_ADD_ALWAYS(_fsm_state_Incoming, _fsm_action_reject, _fsm_state_Terminated, s0000_Incoming_2_Terminated_X_Reject, "s0000_Incoming_2_Terminated_X_Reject"),

		/*=======================
		* === Connected === 
		*/
		// Connected -> (UnRegister) -> Terminated
		TSK_FSM_ADD(_fsm_state_Connected, _fsm_action_iREGISTER, _fsm_cond_server_unregistering, _fsm_state_Terminated, s0000_Connected_2_Terminated_X_iREGISTER, "s0000_Connected_2_Terminated_X_iREGISTER"),
		// Connected -> (TimedOut) -> Terminated
		// Connected -> (Refresh OK) -> Connected
		// Connected -> (Refresh NOK) -> Terminated

	TSK_FSM_ADD_NULL());
}



//--------------------------------------------------------
//				== STATE MACHINE BEGIN ==
//--------------------------------------------------------

/* Started -> (Failure) -> Terminated
*/
int s0000_Started_2_Terminated_X_iREGISTER(va_list *app)
{
	return 0;
	/*tsip_dialog_register_t *self;
	const tsip_action_t* action;

	self = va_arg(*app, tsip_dialog_register_t *);
	va_arg(*app, const tsip_message_t *);
	action = va_arg(*app, const tsip_action_t *);

	TSIP_DIALOG(self)->running = tsk_true;
	tsip_dialog_set_curr_action(TSIP_DIALOG(self), action);

	// alert the user
	TSIP_DIALOG_SIGNAL(self, tsip_event_code_dialog_connecting, "Dialog connecting");

	return send_REGISTER(self, tsk_true);*/
}

/* Started -> (All is OK and we are Registering) -> Incoming
*/
int s0000_Started_2_Incoming_X_iREGISTER(va_list *app)
{
	tsip_dialog_register_t *self = va_arg(*app, tsip_dialog_register_t *);
	tsip_request_t *request = va_arg(*app, tsip_request_t *);

	// set as server side dialog
	TSIP_DIALOG_REGISTER(self)->is_server = tsk_true;

	/* update last REGISTER */
	TSK_OBJECT_SAFE_FREE(self->last_iRegister);
	self->last_iRegister = tsk_object_ref(request);

	/* alert the user (session) */
	TSIP_DIALOG_REGISTER_SIGNAL(self, tsip_i_newreg, 
			tsip_event_code_dialog_request_incoming, "Incoming New Register", request);

	return 0;
}

/* Incoming -> (Accept) -> Connected
*/
int s0000_Incoming_2_Connected_X_Accept(va_list *app)
{
	int ret;

	tsip_dialog_register_t *self;
	const tsip_action_t* action;

	self = va_arg(*app, tsip_dialog_register_t *);
	va_arg(*app, const tsip_message_t *);
	action = va_arg(*app, const tsip_action_t *);

	/* update current action */
	tsip_dialog_set_curr_action(TSIP_DIALOG(self), action);	

	/* send 2xx OK */
	if((ret = tsip_dialog_register_send_RESPONSE(self, self->last_iRegister, 200, "OK"))){
		return ret;
	}

	/* alert the user (dialog) */
	TSIP_DIALOG_SIGNAL(self, tsip_event_code_dialog_connected, "Dialog connected");

	return ret;
}

/* Incoming -> (Reject) -> Terminated
*/
int s0000_Incoming_2_Terminated_X_Reject(va_list *app)
{
	return -1;
}

/* Incoming -> (Unregister) -> Terminated
*/
int s0000_Connected_2_Terminated_X_iREGISTER(va_list *app)
{
	int ret;
	tsip_dialog_register_t *self = va_arg(*app, tsip_dialog_register_t *);
	tsip_request_t *request = va_arg(*app, tsip_request_t *);

	/* update last REGISTER */
	TSK_OBJECT_SAFE_FREE(self->last_iRegister);
	self->last_iRegister = tsk_object_ref(request);

	/* send 2xx OK */
	if((ret = tsip_dialog_register_send_RESPONSE(self, self->last_iRegister, 200, "OK"))){
		return ret;
	}

	/* alert the user (session) */
	TSIP_DIALOG_REGISTER_SIGNAL(self, tsip_i_unregister, 
			tsip_event_code_dialog_request_incoming, "Incoming Request", request);

	return 0;
}