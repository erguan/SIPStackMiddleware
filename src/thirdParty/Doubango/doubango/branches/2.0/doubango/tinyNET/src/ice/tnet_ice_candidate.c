/*
* Copyright (C) 2012 Doubango Telecom <http://www.doubango.org>.
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
#include "tnet_ice_candidate.h"
#include "tnet_ice_utils.h"
#include "tnet_utils.h"

#include "tsk_md5.h"
#include "tsk_memory.h"
#include "tsk_list.h"
#include "tsk_string.h"
#include "tsk_debug.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int _tnet_ice_candidate_tostring(
	uint8_t* foundation, 
	uint32_t comp_id,
	const char* transport_str,
	uint32_t priority,
	const char* connection_addr,
	tnet_port_t port,
	const char* cand_type_str,
	const tsk_params_L_t *extension_att_list,
	char** output);
static const char* _tnet_ice_candidate_get_foundation(tnet_ice_cand_type_t type);
static tnet_stun_message_t * _tnet_ice_candidate_stun_create_bind_request(tnet_ice_candidate_t* self, const char* username, const char* password);
static tsk_bool_t _tnet_ice_candidate_stun_transac_id_equals(const tnet_stun_transacid_t id1, const tnet_stun_transacid_t id2);
static const char* _tnet_ice_candidate_get_transport_str(tnet_socket_type_t transport_e);
static tnet_socket_type_t _tnet_ice_candidate_get_transport_type(tsk_bool_t ipv6, const char* transport_str);
static const char* _tnet_ice_candidate_get_candtype_str(tnet_ice_cand_type_t candtype_e);
static tnet_ice_cand_type_t _tnet_ice_candtype_get_transport_type(const char* candtype_str);

static tsk_object_t* tnet_ice_candidate_ctor(tsk_object_t * self, va_list * app)
{
	tnet_ice_candidate_t *candidate = self;
	if(candidate){
		candidate->extension_att_list = tsk_list_create();
	}
	return self;
}

static tsk_object_t* tnet_ice_candidate_dtor(tsk_object_t * self)
{ 
	tnet_ice_candidate_t *candidate = self;
	if(candidate){
		TSK_SAFE_FREE(candidate->transport_str);
		TSK_SAFE_FREE(candidate->cand_type_str);
		TSK_OBJECT_SAFE_FREE(candidate->extension_att_list);
		TSK_OBJECT_SAFE_FREE(candidate->socket);

		TSK_SAFE_FREE(candidate->stun.nonce);
		TSK_SAFE_FREE(candidate->stun.realm);
		TSK_SAFE_FREE(candidate->stun.srflx_addr);

		TSK_SAFE_FREE(candidate->ufrag);
		TSK_SAFE_FREE(candidate->pwd);

		TSK_SAFE_FREE(candidate->tostring);
	}
	return self;
}

static int tnet_ice_candidate_cmp(const tsk_object_t *_s1, const tsk_object_t *_s2)
{
	const tnet_ice_candidate_t *c1 = _s1;
	const tnet_ice_candidate_t *c2 = _s2;

	if(c1 && c2){
		return (c2 - c1);
	}
	else if(!c1 && !c2) return 0;
	else return -1;
}

static const tsk_object_def_t tnet_ice_candidate_def_s = 
{
	sizeof(tnet_ice_candidate_t),
	tnet_ice_candidate_ctor, 
	tnet_ice_candidate_dtor,
	tnet_ice_candidate_cmp, 
};

tnet_ice_candidate_t* tnet_ice_candidate_create(tnet_ice_cand_type_t type_e, tnet_socket_t* socket, tsk_bool_t is_ice_jingle, tsk_bool_t is_rtp, tsk_bool_t is_video, const char* ufrag, const char* pwd, const char *foundation)
{
	tnet_ice_candidate_t* candidate;

	if(!(candidate = tsk_object_new(&tnet_ice_candidate_def_s))){
		TSK_DEBUG_ERROR("Failed to create candidate");
		return tsk_null;
	}
	
	candidate->type_e = type_e;
	candidate->socket = tsk_object_ref(socket);
	candidate->local_pref = 0xFFFF;
	candidate->is_ice_jingle = is_ice_jingle;
	candidate->is_rtp = is_rtp;
	candidate->is_video = is_video;
	candidate->comp_id = is_rtp ? TNET_ICE_CANDIDATE_COMPID_RTP : TNET_ICE_CANDIDATE_COMPID_RTCP;
	if(foundation){
		memcpy(candidate->foundation, foundation, TSK_MIN(tsk_strlen(foundation), TNET_ICE_CANDIDATE_FOUND_SIZE_PREF));
	}
	else{
		tnet_ice_utils_compute_foundation((char*)candidate->foundation, TSK_MIN(sizeof(candidate->foundation), TNET_ICE_CANDIDATE_FOUND_SIZE_PREF));
	}
	candidate->priority = tnet_ice_utils_get_priority(candidate->type_e, candidate->local_pref, candidate->is_rtp);
	if(candidate->socket){
		memcpy(candidate->connection_addr, candidate->socket->ip, sizeof(candidate->socket->ip));
		candidate->port = candidate->socket->port;
		candidate->transport_e = socket->type;
	}
	tnet_ice_candidate_set_credential(candidate, ufrag, pwd);
	
	return candidate;
}

// @param str e.g. "1 1 udp 1 192.168.196.1 57806 typ host name video_rtcp network_name {0C0137CC-DB78-46B6-9B6C-7E097FFA79FE} username StFEVThMK2DHThkv password qkhKUDr4WqKRwZTo generation 0"
tnet_ice_candidate_t* tnet_ice_candidate_parse(const char* str)
{
	char *v, *copy;
	int32_t k;
	tnet_ice_candidate_t* candidate;

	if(tsk_strnullORempty(str)){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}	

	if(!(candidate = tsk_object_new(&tnet_ice_candidate_def_s))){
		TSK_DEBUG_ERROR("Failed to create candidate");
		return tsk_null;
	}

	k = 0;
	copy = tsk_strdup(str);
	v = strtok(copy, " ");

	while(v){
		switch(k){
			case 0:
				{
					memcpy(candidate->foundation, v, TSK_MIN(tsk_strlen(v), sizeof(candidate->foundation)));
					break;
				}
			case 1:
				{
					candidate->comp_id = atoi(v);
					break;
				}
			case 2:
				{
					candidate->transport_str = tsk_strdup(v);
					break;
				}
			case 3:
				{
					candidate->priority = atoi(v);
					break;
				}
			case 4:
				{
					memcpy(candidate->connection_addr, v, TSK_MIN(tsk_strlen(v), sizeof(candidate->connection_addr)));
					break;
				}
			case 5:
				{
					tnet_family_t family;
					candidate->port = atoi(v);
					family = tnet_get_family(candidate->connection_addr, candidate->port);
					candidate->transport_e = _tnet_ice_candidate_get_transport_type((family == AF_INET6), candidate->transport_str);
					break;
				}
			case 6:
				{
					v = strtok(tsk_null, " ");
					tsk_strupdate(&candidate->cand_type_str, v);
					candidate->type_e = _tnet_ice_candtype_get_transport_type(v);
					break;
				}
			default:
				{
					const char* name = v;
					const char* value = (v = strtok(tsk_null, " "));
					tsk_param_t* param = tsk_param_create(name, value);
					if(param){
						tsk_list_push_back_data(candidate->extension_att_list, (void**)&param);
					}
					break;
				}
		}

		++k;
		v = strtok(tsk_null, " ");
	}

	if(k < 6){
		TSK_DEBUG_ERROR("Failed to parse: %s", str);
		TSK_OBJECT_SAFE_FREE(candidate);
	}
	TSK_FREE(copy);

	return candidate;
}

int tnet_ice_candidate_set_credential(tnet_ice_candidate_t* self, const char* ufrag, const char* pwd)
{
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	tsk_strupdate(&self->ufrag, ufrag);
	tsk_strupdate(&self->pwd, pwd);

	return 0;
}

int tnet_ice_candidate_set_rflx_addr(tnet_ice_candidate_t* self, const char* addr, tnet_port_t port)
{
	if(!self || !addr || !port){
		TSK_DEBUG_ERROR("Invalid argument");
		return -1;
	}
	memset(self->connection_addr, 0, sizeof(self->connection_addr));
	memcpy(self->connection_addr, addr, TSK_MIN(tsk_strlen(addr), sizeof(self->connection_addr)));
	self->port = port;
	return 0;
}

const char* tnet_ice_candidate_get_att_value(const tnet_ice_candidate_t* self, const char* att_name)
{
	if(!self || !att_name){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}
	return tsk_params_get_param_value(self->extension_att_list, att_name);
}

int tnet_ice_candidate_set_local_pref(tnet_ice_candidate_t* self, uint16_t local_pref)
{
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	self->local_pref = local_pref;
	self->priority = tnet_ice_utils_get_priority(self->type_e, self->local_pref, self->is_rtp);
	return 0;
}

const char* tnet_ice_candidate_tostring(tnet_ice_candidate_t* self)
{
	const char* _transport_str;
	char __str[16]; // always allocated: bad idea :(

	if(!self){
		TSK_DEBUG_ERROR("Invalid argument");
		return tsk_null;
	}
	
	_transport_str = self->transport_str ? self->transport_str : _tnet_ice_candidate_get_transport_str(self->transport_e);
	if(self->is_ice_jingle){
		tsk_size_t i, s = tsk_strlen(_transport_str);
		memset(__str, 0, sizeof(__str));
		for(i = 0; i < s && i < sizeof(__str)/sizeof(__str[0]); ++i){
			__str[i] = tolower(_transport_str[i]);
		}
		_transport_str = &__str[0];
	}

	_tnet_ice_candidate_tostring(
		self->foundation,
		self->comp_id,
		_transport_str,
		self->priority,
		(tsk_strnullORempty(self->connection_addr) && self->socket) ? self->socket->ip : self->connection_addr,
		(self->port <= 0 && self->socket) ? self->socket->port : self->port,
		self->cand_type_str ? self->cand_type_str : _tnet_ice_candidate_get_candtype_str(self->type_e),
		self->extension_att_list,
		&self->tostring);

	/* <rel-addr> and <rel-port>:  convey transport addresses related to the
      candidate, useful for diagnostics and other purposes. <rel-addr>
      and <rel-port> MUST be present for server reflexive, peer
      reflexive, and relayed candidates. */
	switch(self->type_e){
		case tnet_ice_cand_type_srflx:
		case tnet_ice_cand_type_prflx:
		case tnet_ice_cand_type_relay:
			{
				if(self->socket){ // when called from the browser(IE, Safari, Opera or Firefox) webrtc4all
					tsk_strcat_2(&self->tostring, " raddr %s rport %d", self->socket->ip, self->socket->port);
				}
				break;
			}
	}

	// WebRTC (Chrome) specific
	if(self->is_ice_jingle){
		if(!tsk_params_have_param(self->extension_att_list, "name")){
			tsk_strcat_2(&self->tostring, " name %s", self->is_rtp ? (self->is_video ? "video_rtp" : "rtp") : (self->is_video ? "video_rtcp" : "rtcp"));
		}
		if(!tsk_params_have_param(self->extension_att_list, "username")){
			tsk_strcat_2(&self->tostring, " username %s", self->ufrag);
		}
		if(!tsk_params_have_param(self->extension_att_list, "password")){
			tsk_strcat_2(&self->tostring, " password %s", self->pwd);
		}
		if(!tsk_params_have_param(self->extension_att_list, "network_name")){
			tsk_strcat_2(&self->tostring, " network_name %s", "{9EBBE687-CCE6-42D3-87F5-B57BB30DEE23}");
		}
		if(!tsk_params_have_param(self->extension_att_list, "generation")){
			tsk_strcat_2(&self->tostring, " generation %s", "0");
		}
	}
	
	return self->tostring;
}

int tnet_ice_candidate_send_stun_bind_request(tnet_ice_candidate_t* self, struct sockaddr_storage* server_addr, const char* username, const char* password)
{
	tnet_stun_message_t *request = tsk_null; 
	tsk_buffer_t *buffer = tsk_null;
	int ret, sendBytes;

	if(!self || !server_addr || !TNET_SOCKET_IS_VALID(self->socket)){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	request = _tnet_ice_candidate_stun_create_bind_request(self, username, password);
	if(!request){
		TSK_DEBUG_ERROR("Failed to create STUN request");
		ret = -2;
		goto bail;
	}

	if(!(buffer = tnet_stun_message_serialize(request))){
		TSK_DEBUG_ERROR("Failed to serialize STUN request");
		ret = -3;
		goto bail;
	}

	sendBytes = tnet_sockfd_sendto(self->socket->fd, (const struct sockaddr*)server_addr, buffer->data, buffer->size);// return number of sent bytes
	if(sendBytes == buffer->size){
		memcpy(self->stun.transac_id, request->transaction_id, sizeof(tnet_stun_transacid_t));
		ret = 0;
	}
	else{
		TSK_DEBUG_ERROR("Only %d bytes sent", sendBytes);
		ret = -4;
		goto bail;
	}

bail:
	TSK_OBJECT_SAFE_FREE(request);
	TSK_OBJECT_SAFE_FREE(buffer);

	return 0;
}

int tnet_ice_candidate_process_stun_response(tnet_ice_candidate_t* self,  const tnet_stun_response_t* response, tnet_fd_t fd)
{
	int ret = 0;

	if(!self || !response){
		TSK_DEBUG_ERROR("Inavlid parameter");
		return -1;
	}
	
	//if(!(_tnet_ice_candidate_stun_transac_id_equals(response->transaction_id, self->stun.transac_id))){
	//	TSK_DEBUG_ERROR("Transaction id mismatch");
	//	return -2;
	//}

	if(TNET_STUN_RESPONSE_IS_ERROR(response)){
		short code = tnet_stun_message_get_errorcode(response);
		const char* realm = tnet_stun_message_get_realm(response);
		const char* nonce = tnet_stun_message_get_nonce(response);

		if(code == 401 && realm && nonce){
			if(!self->stun.nonce){
				/* First time we get a nonce */
				tsk_strupdate(&self->stun.nonce, nonce);
				tsk_strupdate(&self->stun.realm, realm);
				return 0;
			}
			else{
				TSK_DEBUG_ERROR("Authentication failed");
				return -3;
			}
		}
		else{
			TSK_DEBUG_ERROR("STUN error: %hi", code);
			return -4;
		}
	}
	else if(TNET_STUN_RESPONSE_IS_SUCCESS(response)){
		const tnet_stun_attribute_t *attribute;

		if((attribute = tnet_stun_message_get_attribute(response, stun_xor_mapped_address))){
			const tnet_stun_attribute_xmapped_addr_t *xmaddr = (const tnet_stun_attribute_xmapped_addr_t *)attribute;
			tnet_ice_utils_stun_address_tostring(xmaddr->xaddress, xmaddr->family, &self->stun.srflx_addr);
			self->stun.srflx_port = xmaddr->xport;
		}
		else if((attribute = tnet_stun_message_get_attribute(response, stun_mapped_address))){
			const tnet_stun_attribute_mapped_addr_t *maddr = (const tnet_stun_attribute_mapped_addr_t *)attribute;
			ret = tnet_ice_utils_stun_address_tostring(maddr->address, maddr->family, &self->stun.srflx_addr);
			self->stun.srflx_port = maddr->port;
		}
	}

	return ret;
}

const tnet_ice_candidate_t* tnet_ice_candidate_find_by_fd(tnet_ice_candidates_L_t* candidates, tnet_fd_t fd)
{
	if(candidates){
		const tsk_list_item_t *item;
		const tnet_ice_candidate_t* candidate;

		tsk_list_lock(candidates);
		tsk_list_foreach(item, candidates){
			if(!(candidate = item->data)){
				continue;
			}
			if(candidate->socket && (candidate->socket->fd == fd)){
				tsk_list_unlock(candidates);
				return candidate;
			}
		}
	}

	return tsk_null;
}

const char* tnet_ice_candidate_get_ufrag(const tnet_ice_candidate_t* self)
{
	if(self){
		return self->ufrag ? self->ufrag : tnet_ice_candidate_get_att_value(self, "username");
	}
	return tsk_null;
}

const char* tnet_ice_candidate_get_pwd(const tnet_ice_candidate_t* self)
{
	if(self){
		return self->pwd ? self->pwd : tnet_ice_candidate_get_att_value(self, "password");
	}
	return tsk_null;
}

static int _tnet_ice_candidate_tostring(
	uint8_t* foundation, 
	uint32_t comp_id,
	const char* transport_str,
	uint32_t priority,
	const char* connection_addr,
	tnet_port_t port,
	const char* cand_type_str,
	const tsk_params_L_t *extension_att_list,
	char** output)
{
	if(!output){
		TSK_DEBUG_ERROR("Invalid argument");
		return -1;
	}
	tsk_sprintf(output, "%s %d %s %d %s %d typ %s",
		foundation,
		comp_id,
		transport_str,
		priority,
		connection_addr,
		port, 
		cand_type_str);
	
	if(extension_att_list){
		const tsk_list_item_t *item;
		tsk_list_foreach(item, extension_att_list){
			tsk_strcat_2(output, " %s %s", TSK_PARAM(item->data)->name, TSK_PARAM(item->data)->value);
		}
	}
	return 0;
}

static const char* _tnet_ice_candidate_get_foundation(tnet_ice_cand_type_t type)
{
	switch(type){
		case tnet_ice_cand_type_host: return TNET_ICE_CANDIDATE_FOUNDATION_HOST;
		case tnet_ice_cand_type_srflx: return TNET_ICE_CANDIDATE_FOUNDATION_SRFLX;
		case tnet_ice_cand_type_prflx: return TNET_ICE_CANDIDATE_FOUNDATION_PRFLX;
		case tnet_ice_cand_type_relay: default: return TNET_ICE_CANDIDATE_FOUNDATION_RELAY;
	}
}


static tsk_bool_t _tnet_ice_candidate_stun_transac_id_equals(const tnet_stun_transacid_t id1, const tnet_stun_transacid_t id2)
{
	tsk_size_t i;
	static const tsk_size_t size = sizeof(tnet_stun_transacid_t);
	for(i = 0; i < size; i++){
		if(id1[i] != id2[i]){
			return tsk_false;
		}
	}
	return tsk_true;
}

static tnet_stun_message_t * _tnet_ice_candidate_stun_create_bind_request(tnet_ice_candidate_t* self, const char* username, const char* password)
{
	tnet_stun_message_t *request = tsk_null;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}

	request = tnet_stun_message_create(username, password);

	if(request){
		tnet_stun_attribute_t* attribute;
		request->realm = tsk_strdup(self->stun.realm);
		request->nonce = tsk_strdup(self->stun.nonce);

		/* Set the request type (RFC 5389 defines only one type) */
		request->type = stun_binding_request;

		/* Add software attribute */
		if((attribute = (tnet_stun_attribute_t*)tnet_stun_attribute_software_create(TNET_SOFTWARE, tsk_strlen(TNET_SOFTWARE)))){
			tnet_stun_message_add_attribute(request, &attribute);
		}
	}
	
	return request;
}

static const char* _tnet_ice_candidate_get_transport_str(tnet_socket_type_t transport_e)
{
#define TRANSPORT_GET(STR) \
	if(TNET_SOCKET_TYPE_IS_##STR(transport_e)){ \
		return TNET_ICE_CANDIDATE_TRANSPORT_##STR; \
	}
	TRANSPORT_GET(UDP);
	TRANSPORT_GET(TCP);
	TRANSPORT_GET(TLS);
	TRANSPORT_GET(SCTP);
	TRANSPORT_GET(WS);
	TRANSPORT_GET(WSS);
	return "UNKNOWN";

#undef TRANSPORT_GET
}

static tnet_socket_type_t _tnet_ice_candidate_get_transport_type(tsk_bool_t ipv6, const char* transport_str)
{
#define TRANSPORT_GET(STR, str) \
	if(tsk_striequals(TNET_ICE_CANDIDATE_TRANSPORT_##STR, transport_str)){ \
		return tnet_socket_type_##str##_ipv4; \
	}
	
	TRANSPORT_GET(UDP, udp);
	TRANSPORT_GET(TCP, tcp);
	TRANSPORT_GET(TLS, tls);
	TRANSPORT_GET(SCTP, sctp);
	TRANSPORT_GET(WS, ws);
	TRANSPORT_GET(WSS, wss);
	return tnet_socket_type_invalid;

#undef TRANSPORT_GET
}

static const char* _tnet_ice_candidate_get_candtype_str(tnet_ice_cand_type_t candtype_e)
{
	switch(candtype_e){
		case tnet_ice_cand_type_unknown:
		default: return "unknown";
		case tnet_ice_cand_type_host: return "host";
		case tnet_ice_cand_type_srflx: return "srflx";
		case tnet_ice_cand_type_prflx: return "prflx";
		case tnet_ice_cand_type_relay: return "relay";
	}
}

static tnet_ice_cand_type_t _tnet_ice_candtype_get_transport_type(const char* candtype_str)
{
	if(tsk_striequals(TNET_ICE_CANDIDATE_TYPE_HOST, candtype_str)){
		return tnet_ice_cand_type_host;
	}
	else if(tsk_striequals(TNET_ICE_CANDIDATE_TYPE_SRFLX, candtype_str)){
		return tnet_ice_cand_type_srflx;
	}
	else if(tsk_striequals(TNET_ICE_CANDIDATE_TYPE_PRFLX, candtype_str)){
		return tnet_ice_cand_type_prflx;
	}
	else if(tsk_striequals(TNET_ICE_CANDIDATE_TYPE_RELAY, candtype_str)){
		return tnet_ice_cand_type_relay;
	}
	else{
		return tnet_ice_cand_type_unknown;
	}
}