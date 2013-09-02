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

/**@file tmedia_session.h
 * @brief Base session object.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango[dot]org>
 *

 */
#include "tinymedia/tmedia_session.h"

#include "tinymedia/tmedia_session_ghost.h"
#include "tinymedia/tmedia_defaults.h"

#include "tinysdp/headers/tsdp_header_O.h"

#include "ice/tnet_ice_ctx.h"

#include "tsk_memory.h"
#include "tsk_debug.h"

/**@defgroup tmedia_session_group Media Session
* For more information about the SOA, please refer to http://betelco.blogspot.com/2010/03/sdp-offeranswer-soa_2993.html
*/

#if !defined(va_copy)
#	define va_copy(D, S)       ((D) = (S))
#endif

extern const tmedia_codec_plugin_def_t* __tmedia_codec_plugins[TMED_CODEC_MAX_PLUGINS];

/* pointer to all registered sessions */
const tmedia_session_plugin_def_t* __tmedia_session_plugins[TMED_SESSION_MAX_PLUGINS] = {0};

/* === local functions === */
static int _tmedia_session_mgr_load_sessions(tmedia_session_mgr_t* self);
static int _tmedia_session_mgr_clear_sessions(tmedia_session_mgr_t* self);
static int _tmedia_session_mgr_apply_params(tmedia_session_mgr_t* self);
static int _tmedia_session_prepare(tmedia_session_t* self);
static int _tmedia_session_set_ro(tmedia_session_t* self, const tsdp_header_M_t* m);
static int _tmedia_session_load_codecs(tmedia_session_t* self);

const char* tmedia_session_get_media(const tmedia_session_t* self);
const tsdp_header_M_t* tmedia_session_get_lo(tmedia_session_t* self);
int tmedia_session_set_ro(tmedia_session_t* self, const tsdp_header_M_t* m);


/*== Predicate function to find session object by media */
static int __pred_find_session_by_media(const tsk_list_item_t *item, const void *media)
{
	if(item && item->data){
		return tsk_stricmp(tmedia_session_get_media((const tmedia_session_t *)item->data), (const char*)media);
	}
	return -1;
}

/*== Predicate function to find session object by type */
static int __pred_find_session_by_type(const tsk_list_item_t *item, const void *type)
{
	if(item && item->data){
		return ((const tmedia_session_t *)item->data)->type - *((tmedia_type_t*)type);
	}
	return -1;
}

/*== Predicate function to find codec object by format */
static int __pred_find_codec_by_format(const tsk_list_item_t *item, const void *codec)
{
	if(item && item->data && codec){
		return tsk_stricmp(((const tmedia_codec_t*)item->data)->format, ((const tmedia_codec_t*)codec)->format);
	}
	return -1;
}

/*== Predicate function to find codec object by id */
static int __pred_find_codec_by_id(const tsk_list_item_t *item, const void *id)
{
	if(item && item->data && id){
		if(((const tmedia_codec_t*)item->data)->id == *((const tmedia_codec_id_t*)id)){
			return 0;
		}
	}
	return -1;
}

uint64_t tmedia_session_get_unique_id(){
	static uint64_t __UniqueId = 1; // MUST not be equal to zero
	return __UniqueId++;
}

/**@ingroup tmedia_session_group
* Initializes a newly created media session.
* @param self the media session to initialize.
* @param type the type of the session to initialize.
* @retval Zero if succeed and non-zero error code otherwise.
*/
int tmedia_session_init(tmedia_session_t* self, tmedia_type_t type)
{
	int ret = 0;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	
	if(!self->initialized){
		/* set values */
		if(!self->id){
			self->id = tmedia_session_get_unique_id();
		}
		self->type = type;
		self->initialized = tsk_true;
		self->bl = tmedia_defaults_get_bl();
		self->codecs_allowed = tmedia_codec_id_all;
		self->bypass_encoding = tmedia_defaults_get_bypass_encoding();
		self->bypass_decoding = tmedia_defaults_get_bypass_decoding();
		/* load associated codecs */
		ret = _tmedia_session_load_codecs(self);
	}

	return 0;
}

int tmedia_session_set(tmedia_session_t* self, ...)
{
	va_list ap;
	tmedia_params_L_t* params;

	if(!self || !self->plugin || !self->plugin->set){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	va_start(ap, self);
	if((params = tmedia_params_create_2(&ap))){
		const tsk_list_item_t *item;
		const tmedia_param_t* param;
		tsk_list_foreach(item, params){
			if(!(param = item->data)){
				continue;
			}
			if((self->type & param->media_type)){
				self->plugin->set(self, param);
			}
		}
		TSK_OBJECT_SAFE_FREE(params);
	}
	va_end(ap);
	return 0;
}

tsk_bool_t tmedia_session_set_2(tmedia_session_t* self, const tmedia_param_t* param)
{
	if(!self || !param){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_false;
	}

	if(param->plugin_type == tmedia_ppt_session){
		if(param->value_type == tmedia_pvt_int32){
			if(tsk_striequals(param->key, "codecs-supported")){
				//if(self->M.lo){
				//	TSK_DEBUG_WARN("Cannot change codec values at this stage");
				//}
				//else{
					int32_t codecs_allowed = *((int32_t*)param->value);
					if(self->codecs_allowed != codecs_allowed){
						self->codecs_allowed = codecs_allowed;
						return (_tmedia_session_load_codecs(self) == 0);
					}
					return 0;
				//}
				return tsk_true;
			}
			else if(tsk_striequals(param->key, "bypass-encoding")){
				self->bypass_encoding = *((int32_t*)param->value);
				return tsk_true;
			}
			else if(tsk_striequals(param->key, "bypass-decoding")){
				self->bypass_decoding = *((int32_t*)param->value);
				return tsk_true;
			}
			else if(tsk_striequals(param->key, "dtls-cert-verify")){
				self->dtls.verify = *((int32_t*)param->value) ? tsk_true : tsk_false;
				return tsk_true;
			}
		}
		else if(param->value_type == tmedia_pvt_pchar){
			if(tsk_striequals(param->key, "dtls-file-ca")){
				tsk_strupdate(&self->dtls.file_ca, param->value);
				return tsk_true;
			}
			else if(tsk_striequals(param->key, "dtls-file-pbk")){
				tsk_strupdate(&self->dtls.file_pbk, param->value);
				return tsk_true;
			}
			else if(tsk_striequals(param->key, "dtls-file-pvk")){
				tsk_strupdate(&self->dtls.file_pvk, param->value);
				return tsk_true;
			}
		}
	}
	
	return tsk_false;
}

tsk_bool_t tmedia_session_get(tmedia_session_t* self, tmedia_param_t* param)
{
	if(!self || !param){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_false;
	}

	if(param->plugin_type == tmedia_ppt_session){
		if(param->value_type == tmedia_pvt_int32){
			if(tsk_striequals(param->key, "codecs-negotiated")){ // negotiated codecs
				tmedia_codecs_L_t* neg_codecs = tsk_object_ref(self->neg_codecs);
				if(neg_codecs){
					const tsk_list_item_t* item;
					tsk_list_foreach(item, neg_codecs){
						((int32_t*)param->value)[0] |= TMEDIA_CODEC(item->data)->id;
					}
					TSK_OBJECT_SAFE_FREE(neg_codecs);
				}
				return tsk_true;
			}
		}
	}

	return tsk_false;
}

/**@ingroup tmedia_session_group
* Generic function to compare two sessions.
* @param sess1 The first session to compare.
* @param sess2 The second session to compare.
* @retval Returns an integral value indicating the relationship between the two sessions:
* <0 : @a sess1 less than @a sess2.<br>
* 0  : @a sess1 identical to @a sess2.<br>
* >0 : @a sess1 greater than @a sess2.<br>
*/
int tmedia_session_cmp(const tsk_object_t* sess1, const tsk_object_t* sess2)
{	
	return (TMEDIA_SESSION(sess1) - TMEDIA_SESSION(sess2));
}

/**@ingroup tmedia_session_group
* Registers a session plugin.
* @param plugin the definition of the plugin.
* @retval Zero if succeed and non-zero error code otherwise.
* @sa @ref tmedia_session_create()
*/
int tmedia_session_plugin_register(const tmedia_session_plugin_def_t* plugin)
{
	tsk_size_t i;
	if(!plugin){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	/* add or replace the plugin */
	for(i = 0; i<TMED_SESSION_MAX_PLUGINS; i++){
		if(!__tmedia_session_plugins[i] || (__tmedia_session_plugins[i] == plugin)){
			__tmedia_session_plugins[i] = plugin;
			return 0;
		}
	}
	
	TSK_DEBUG_ERROR("There are already %d plugins.", TMED_SESSION_MAX_PLUGINS);
	return -2;
}

/**@ingroup tmedia_session_group
* Finds a plugin by media.
*/
const tmedia_session_plugin_def_t* tmedia_session_plugin_find_by_media(const char* media)
{
	tsk_size_t i = 0;
	if(tsk_strnullORempty(media)){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}

	/* add or replace the plugin */
	while((i<TMED_SESSION_MAX_PLUGINS) && (__tmedia_session_plugins[i])){
		if(tsk_striequals(__tmedia_session_plugins[i]->media, media)){
			return __tmedia_session_plugins[i];
		}
		i++;
	}
	return tsk_null;
}

/**@ingroup tmedia_session_group
* UnRegisters a session plugin.
* @param plugin the definition of the plugin.
* @retval Zero if succeed and non-zero error code otherwise.
*/
int tmedia_session_plugin_unregister(const tmedia_session_plugin_def_t* plugin)
{
	tsk_size_t i;
	tsk_bool_t found = tsk_false;
	if(!plugin){
		TSK_DEBUG_ERROR("Invalid Parameter");
		return -1;
	}
	
	/* find the plugin to unregister */
	for(i = 0; i<TMED_SESSION_MAX_PLUGINS && __tmedia_session_plugins[i]; i++){
		if(__tmedia_session_plugins[i] == plugin){
			__tmedia_session_plugins[i] = tsk_null;
			found = tsk_true;
			break;
		}
	}
	
	/* compact */
	if(found){
		for(; i<(TMED_SESSION_MAX_PLUGINS - 1); i++){
			if(__tmedia_session_plugins[i+1]){
				__tmedia_session_plugins[i] = __tmedia_session_plugins[i+1];
			}
			else{
				break;
			}
		}
		__tmedia_session_plugins[i] = tsk_null;
	}
	return (found ? 0 : -2);
}

/**@ingroup tmedia_session_group
* Creates a new session using an already registered plugin.
* @param format The type of the codec to create.
* @sa @ref tmedia_codec_plugin_register()
*/
tmedia_session_t* tmedia_session_create(tmedia_type_t type)
{
	tmedia_session_t* session = tsk_null;
	const tmedia_session_plugin_def_t* plugin;
	tsk_size_t i = 0;

	while((i < TMED_SESSION_MAX_PLUGINS) && (plugin = __tmedia_session_plugins[i++])){
		if(plugin->objdef && (plugin->type == type)){
			if((session = tsk_object_new(plugin->objdef))){
				if(!session->initialized){
					tmedia_session_init(session, type);
				}
				session->plugin = plugin;
			}
			break;
		}
	}
	return session;
}

/* internal funtion: prepare lo */
static int _tmedia_session_prepare(tmedia_session_t* self)
{
	int ret;
	if(!self || !self->plugin || !self->plugin->prepare){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	if(self->prepared){
		TSK_DEBUG_WARN("Session already prepared");
		return 0;
	}
	if((ret = self->plugin->prepare(self))){
		TSK_DEBUG_ERROR("Failed to prepare the session");
	}
	else{
		self->prepared = tsk_true;
	}
	return ret;
}

/* internal function used to set remote offer */
int _tmedia_session_set_ro(tmedia_session_t* self, const tsdp_header_M_t* m)
{
	int ret;
	if(!self || !self->plugin || !self->plugin->set_remote_offer){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	if(!(ret = self->plugin->set_remote_offer(self, m))){
		self->ro_changed = tsk_true;
		self->ro_held = tsdp_header_M_is_held(m, tsk_false);
	}
	return ret;
}

/* internal function: get media */
const char* tmedia_session_get_media(const tmedia_session_t* self)
{
	if(!self || !self->plugin){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}

	/* ghost? */
	if(self->plugin == tmedia_session_ghost_plugin_def_t){
		return ((const tmedia_session_ghost_t*)self)->media;
	}
	else{
		return self->plugin->media;
	}
}
/* internal function: get local offer */
const tsdp_header_M_t* tmedia_session_get_lo(tmedia_session_t* self)
{
	const tsdp_header_M_t* m;

	if(!self || !self->plugin || !self->plugin->get_local_offer){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}
	
	if((m = self->plugin->get_local_offer(self))){
		self->ro_changed = tsk_false; /* we should have a fresh local offer (based on the latest ro) */
	}
	return m;
}

/* Match a codec */
tmedia_codecs_L_t* tmedia_session_match_codec(tmedia_session_t* self, const tsdp_header_M_t* M)
{
	const tmedia_codec_t *codec;
	char *rtpmap = tsk_null, *fmtp = tsk_null, *image_attr = tsk_null, *name = tsk_null;
	const tsdp_fmt_t* fmt;
	const tsk_list_item_t *it1, *it2;
	tsk_bool_t found = tsk_false;
	tmedia_codecs_L_t* matchingCodecs = tsk_null;
	
	if(!self || !M){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}


	/* foreach format */
	tsk_list_foreach(it1, M->FMTs){
		fmt = it1->data;
		
		/* foreach codec */
		tsk_list_foreach(it2, self->codecs){
			/* 'tmedia_codec_id_none' is used for fake codecs (e.g. dtmf or msrp) and should not be filtered beacuse of backward compatibility*/
			if(!(codec = it2->data) || !codec->plugin || !(codec->id == tmedia_codec_id_none || (codec->id & self->codecs_allowed))){
				continue;
			}
			
			// Guard to avoid matching a codec more than once
			// For example, H.264 codecs without profiles (Jitsi, Tiscali PC client) to distinguish them could match more than once
			if(matchingCodecs && tsk_list_find_object_by_pred(matchingCodecs, __pred_find_codec_by_format, codec)){
				continue;
			}
			
			// Dyn. payload type
			if(codec->dyn && (rtpmap = tsdp_header_M_get_rtpmap(M, fmt->value))){
				int32_t rate, channels;
				/* parse rtpmap */
				if(tmedia_parse_rtpmap(rtpmap, &name, &rate, &channels)){
					goto next;
				}
				
				/* compare name and rate... what about channels? */
				if(tsk_striequals(name, codec->name) && (!rate || !codec->plugin->rate || (codec->plugin->rate == rate))){
					goto compare_fmtp;
				}
			}
			// Fixed payload type
			else{
				if(tsk_striequals(fmt->value, codec->format)){
					goto compare_fmtp;
				}
			}
			
			/* rtpmap do not match: free strings and try next codec */
			goto next;

compare_fmtp:
			if((fmtp = tsdp_header_M_get_fmtp(M, fmt->value))){ /* remote have fmtp? */
				if(tmedia_codec_sdp_att_match(codec, "fmtp", fmtp)){ /* fmtp matches? */
					if(codec->type & tmedia_video) goto compare_imageattr;
					else found = tsk_true;
				}
				else goto next;
			}
			else{ /* no fmtp -> always match */
				if(codec->type & tmedia_video) goto compare_imageattr;
				else found = tsk_true;
			}

compare_imageattr:
			if(codec->type & tmedia_video){
				if((image_attr = tsdp_header_M_get_imageattr(M, fmt->value))){
					if(tmedia_codec_sdp_att_match(codec, "imageattr", image_attr)) found = tsk_true;
				}
				else found = tsk_true;
			}

			// update neg. format
			if(found) tsk_strupdate((char**)&codec->neg_format, fmt->value);
			
next:
			TSK_FREE(name);
			TSK_FREE(fmtp);
			TSK_FREE(rtpmap);
			TSK_FREE(image_attr);
			if(found){
				tmedia_codec_t * copy;
				if(!matchingCodecs){
					matchingCodecs = tsk_list_create();
				}
				copy = tsk_object_ref((void*)codec);
				tsk_list_push_back_data(matchingCodecs, (void**)&copy);

				found = tsk_false;
				break;
			}
		}
	}
	

	return matchingCodecs;
}

int tmedia_session_set_onrtcp_cbfn(tmedia_session_t* self, const void* context, tmedia_session_rtcp_onevent_cb_f func)
{
	if(self && self->plugin && self->plugin->rtcp.set_onevent_cbfn){
		return self->plugin->rtcp.set_onevent_cbfn(self, context, func);
	}
	return -1;
}

int tmedia_session_send_rtcp_event(tmedia_session_t* self, tmedia_rtcp_event_type_t event_type, uint32_t ssrc_media)
{
	if(self && self->plugin && self->plugin->rtcp.send_event){
		return self->plugin->rtcp.send_event(self, event_type, ssrc_media);
	}
	TSK_DEBUG_INFO("Not sending RTCP event with SSRC = %u because no callback function found", ssrc_media);
	return -1;
}

int tmedia_session_set_onerror_cbfn(tmedia_session_t* self, const void* usrdata, tmedia_session_onerror_cb_f fun)
{
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	self->onerror_cb.fun = fun;
	self->onerror_cb.usrdata = usrdata;
	return 0;
}

/**@ingroup tmedia_session_group
* DeInitializes a media session.
* @param self the media session to deinitialize.
* @retval Zero if succeed and non-zero error code otherwise.
*/
int tmedia_session_deinit(tmedia_session_t* self)
{
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	
	/* free codecs */
	TSK_OBJECT_SAFE_FREE(self->codecs);
	TSK_OBJECT_SAFE_FREE(self->neg_codecs);
	
	/* free lo, no and ro */
	TSK_OBJECT_SAFE_FREE(self->M.lo);
	TSK_OBJECT_SAFE_FREE(self->M.ro);

	/* QoS */
	TSK_OBJECT_SAFE_FREE(self->qos);

	/* DTLS */
	TSK_FREE(self->dtls.file_ca);
	TSK_FREE(self->dtls.file_pbk);
	TSK_FREE(self->dtls.file_pvk);
	
	return 0;
}

/**@ingroup tmedia_session_group
* Send DTMF event
* @param self the audio session to use to send a DTMF event
* @param event the DTMF event to send (should be between 0-15)
* @retval Zero if succeed and non-zero error code otherwise.
*/
int tmedia_session_audio_send_dtmf(tmedia_session_audio_t* self, uint8_t event)
{
	if(!self || !TMEDIA_SESSION(self)->plugin || !TMEDIA_SESSION(self)->plugin->audio.send_dtmf){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	return TMEDIA_SESSION(self)->plugin->audio.send_dtmf(TMEDIA_SESSION(self), event);
}

int tmedia_session_t140_set_ondata_cbfn(tmedia_session_t* self, const void* context, tmedia_session_t140_ondata_cb_f func)
{
	if(self && self->plugin && self->plugin->t140.set_ondata_cbfn){
		return self->plugin->t140.set_ondata_cbfn(self, context, func);
	}
	return -1;
}

int tmedia_session_t140_send_data(tmedia_session_t* self, enum tmedia_t140_data_type_e data_type, const void* data_ptr, unsigned data_size)
{
	if(self && self->plugin && self->plugin->t140.send_data){
		return self->plugin->t140.send_data(self, data_type, data_ptr, data_size);
	}
	return -1;
}

/* internal function used to prepare a session */
int _tmedia_session_load_codecs(tmedia_session_t* self)
{
	tsk_size_t i = 0;
	tmedia_codec_t* codec;
	const tmedia_codec_plugin_def_t* plugin;
	const tsk_list_item_t* item;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	if(!self->codecs && !(self->codecs = tsk_list_create())){
		TSK_DEBUG_ERROR("Failed to create new list");
		return -1;
	}

	tsk_list_lock(self->codecs);

	/* remove old codecs */
	tsk_list_clear_items(self->codecs);

	/* for each registered plugin create a session instance */
	while((i < TMED_CODEC_MAX_PLUGINS) && (plugin = __tmedia_codec_plugins[i++])){
		/* 'tmedia_codec_id_none' is used for fake codecs (e.g. dtmf or msrp) and should not be filtered beacuse of backward compatibility*/
		if((plugin->type & self->type) && (plugin->codec_id == tmedia_codec_id_none || (plugin->codec_id & self->codecs_allowed))){
			if((codec = tmedia_codec_create(plugin->format))){
				if(!self->codecs){
					self->codecs = tsk_list_create();
				}
				tsk_list_push_back_data(self->codecs, (void**)(&codec));
			}
		}
	}

	// filter negotiated codecs with the newly loaded codecs
	if(1){ // code valid for all use-cases but for now it's not fully tested and not needed for the clients
filter_neg_codecs:
		tsk_list_foreach(item, self->neg_codecs){
			if(!(codec = (item->data))){
				continue;
			}
			if(!(tsk_list_find_item_by_pred(self->codecs, __pred_find_codec_by_id, &codec->id))){
				const char* codec_name = codec->plugin ? codec->plugin->name : "unknown";
				const char* neg_format = codec->neg_format ? codec->neg_format : codec->format;
				TSK_DEBUG_INFO("Codec '%s' with format '%s' was negotiated but [supported codecs] updated without it -> removing", codec_name, neg_format);
				// update sdp and remove the codec from the list
				if(self->M.lo && !TSK_LIST_IS_EMPTY(self->M.lo->FMTs)){
					if(self->M.lo->FMTs->head->next == tsk_null && tsdp_header_M_have_fmt(self->M.lo, neg_format)){ // single item?
						// rejecting a media with port equal to zero requires at least one format
						TSK_DEBUG_INFO("[supported codecs] updated but do not remove codec with name='%s' and format='%s' because it's the last one", codec_name, neg_format);
						self->M.lo->port = 0;
					}
					else{
						tsdp_header_M_remove_fmt(self->M.lo, neg_format);
					}
				}
				tsk_list_remove_item_by_data(self->neg_codecs, codec);
				goto filter_neg_codecs;
			}
		}
	}

	tsk_list_unlock(self->codecs);

	return 0;
}


/**@ingroup tmedia_session_group
* Creates new session manager.
* @param type the type of the session to create. For example, (@ref tmed_sess_type_audio | @ref tmed_sess_type_video).
* @param addr the local ip address or FQDN to use in the sdp message.
* @param ipv6 indicates whether @a addr is IPv6 address or not. Useful when @a addr is a FQDN.
* @param load_sessions Whether the offerer or not.
* will create an audio/video session.
* @retval new @ref tmedia_session_mgr_t object
*/
tmedia_session_mgr_t* tmedia_session_mgr_create(tmedia_type_t type, const char* addr, tsk_bool_t ipv6, tsk_bool_t offerer)
{
	tmedia_session_mgr_t* mgr;

	if(!(mgr = tsk_object_new(tmedia_session_mgr_def_t))){
		TSK_DEBUG_ERROR("Failed to create Media Session manager");
		return tsk_null;
	}

	/* init */
	mgr->type = type;
	mgr->addr = tsk_strdup(addr);
	mgr->ipv6 = ipv6;

	/* load sessions (will allow us to generate lo) */
	if(offerer){
		mgr->offerer = tsk_true;
		//if(_tmedia_session_mgr_load_sessions(mgr)){
			/* Do nothing */
		//	TSK_DEBUG_ERROR("Failed to load sessions");
		//}
	}

	return mgr;
}

/**@ingroup tmedia_session_group
 */
int tmedia_session_mgr_set_media_type(tmedia_session_mgr_t* self, tmedia_type_t type)
{
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	if(self->type != type){
		self->mediaType_changed = tsk_true;
		self->type = type;
	}
	return 0;
}

// special set() case
int tmedia_session_mgr_set_codecs_supported(tmedia_session_mgr_t* self, tmedia_codec_id_t codecs_supported)
{
	int ret = 0;
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	// calling set() could create zombies (media sessions with port equal to zero)
	ret = tmedia_session_mgr_set(self,
		TMEDIA_SESSION_SET_INT32(self->type, "codecs-supported", codecs_supported),
		tsk_null);
	if(ret == 0 && self->sdp.lo){
		// update type (will discard zombies)
		tmedia_type_t new_type = tmedia_type_from_sdp(self->sdp.lo);
		if(new_type != self->type){
			TSK_DEBUG_INFO("codecs-supported updated and media type changed from %d to %d", self->type, new_type);
			self->type = new_type;
		}
	}
	return ret;
}

/**@ingroup tmedia_session_group
*/
tmedia_session_t* tmedia_session_mgr_find(tmedia_session_mgr_t* self, tmedia_type_t type)
{	
	tmedia_session_t* session;

	tsk_list_lock(self->sessions);
	session = (tmedia_session_t*)tsk_list_find_object_by_pred(self->sessions, __pred_find_session_by_type, &type);
	tsk_list_unlock(self->sessions);

	return tsk_object_ref(session);
}

/**@ingroup tmedia_session_group
*/
int tmedia_session_mgr_set_natt_ctx(tmedia_session_mgr_t* self, tnet_nat_context_handle_t* natt_ctx, const char* public_addr)
{
	if(!self || !natt_ctx){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	TSK_OBJECT_SAFE_FREE(self->natt_ctx);
	self->natt_ctx = tsk_object_ref(natt_ctx);
	tsk_strupdate(&self->public_addr, public_addr);

	tmedia_session_mgr_set(self,
		TMEDIA_SESSION_SET_POBJECT(self->type, "natt-ctx", self->natt_ctx),
		TMEDIA_SESSION_SET_NULL());
	return 0;
}

int tmedia_session_mgr_set_ice_ctx(tmedia_session_mgr_t* self, struct tnet_ice_ctx_s* ctx_audio, struct tnet_ice_ctx_s* ctx_video)
{
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	TSK_OBJECT_SAFE_FREE(self->ice.ctx_audio);
	TSK_OBJECT_SAFE_FREE(self->ice.ctx_video);
	
	self->ice.ctx_audio = tsk_object_ref(ctx_audio);
	self->ice.ctx_video = tsk_object_ref(ctx_video);
	
	if(self->type & tmedia_audio){
		tmedia_session_mgr_set(self,
			TMEDIA_SESSION_SET_POBJECT(tmedia_audio, "ice-ctx", self->ice.ctx_audio),
			TMEDIA_SESSION_SET_NULL());
	}

	if(self->type & tmedia_video){
		tmedia_session_mgr_set(self,
			TMEDIA_SESSION_SET_POBJECT(tmedia_video, "ice-ctx", self->ice.ctx_video),
			TMEDIA_SESSION_SET_NULL());
	}

	return 0;
}

/**@ingroup tmedia_session_group
* Starts the session manager by starting all underlying sessions.
* You should set both remote and local offers before calling this function.
* @param self The session manager to start.
* @retval Zero if succced and non-zero error code otherwise.
*
* @sa @ref tmedia_session_mgr_stop
*/
int tmedia_session_mgr_start(tmedia_session_mgr_t* self)
{
	int ret = 0, started_count = 0;
	tsk_list_item_t* item;
	tmedia_session_t* session;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	tsk_safeobj_lock(self);
    
	if(self->started){
		goto bail;
	}

	tsk_list_foreach(item, self->sessions){
		if(!(session = item->data) || !session->plugin || !session->plugin->start){
			TSK_DEBUG_ERROR("Invalid session");
			ret = -2;
			goto bail;
		}
		if((ret = session->plugin->start(session))){
			TSK_DEBUG_ERROR("Failed to start %s session", session->plugin->media);
			continue;
		}
		++started_count;
	}

	self->started = (started_count > 0) ? tsk_true : tsk_false;

bail:
	tsk_safeobj_unlock(self);
	return (self->started ? 0 : -2);
}

/**@ingroup tmedia_session_group
* sets parameters for one or several sessions.
* @param self The session manager
* @param ... Any TMEDIA_SESSION_SET_*() macros
* @retval Zero if succeed and non-zero error code otherwise
*/
int tmedia_session_mgr_set(tmedia_session_mgr_t* self, ...)
{
	va_list ap;
	int ret;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	
	va_start(ap, self);
	ret = tmedia_session_mgr_set_2(self, &ap);
	va_end(ap);

	return ret;
}

/**@ingroup tmedia_session_group
* sets parameters for one or several sessions.
* @param self The session manager
* @param app List of parameters.
* @retval Zero if succeed and non-zero error code otherwise
*/
int tmedia_session_mgr_set_2(tmedia_session_mgr_t* self, va_list *app)
{
	tmedia_params_L_t* params;

	if(!self || !app){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	if((params = tmedia_params_create_2(app))){
		if(!self->params){
			self->params = tsk_object_ref(params);
		}
		else{
			tsk_list_pushback_list(self->params, params);
		}
		TSK_OBJECT_SAFE_FREE(params);
	}

	/* load params if we already have sessions */
	if(!TSK_LIST_IS_EMPTY(self->sessions)){
		_tmedia_session_mgr_apply_params(self);
	}

	return 0;
}

/**@ingroup tmedia_session_group
* sets parameters for one or several sessions.
* @param self The session manager
* @param params List of parameters to set
* @retval Zero if succeed and non-zero error code otherwise
*/
int tmedia_session_mgr_set_3(tmedia_session_mgr_t* self, const tmedia_params_L_t* params)
{
	if(!self || !params){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	if(!self->params){
		self->params = tsk_list_create();
	}
	tsk_list_pushback_list(self->params, params);

	/* load params if we already have sessions */
	if(!TSK_LIST_IS_EMPTY(self->sessions)){
		_tmedia_session_mgr_apply_params(self);
	}

	return 0;
}

int tmedia_session_mgr_get(tmedia_session_mgr_t* self, ...)
{
	va_list ap;
	int ret = 0;
	tmedia_params_L_t* params;
	const tsk_list_item_t *item1, *item2;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	
	va_start(ap, self);

	if((params = tmedia_params_create_2(&ap))){
		tmedia_session_t* session;
		tmedia_param_t* param;
		tsk_list_foreach(item2, params){
			if((param = item2->data)){
				tsk_list_foreach(item1, self->sessions){
					if(!(session = (tmedia_session_t*)item1->data) || !session->plugin){
						continue;
					}
					if((session->type & param->media_type) == session->type && session->plugin->set){
						ret = session->plugin->get(session, param);
					}
				}
			}
		}
		TSK_OBJECT_SAFE_FREE(params);
	}

	va_end(ap);

	return ret;
}

/**@ingroup tmedia_session_group
* Stops the session manager by stopping all underlying sessions.
* @param self The session manager to stop.
* @retval Zero if succced and non-zero error code otherwise.
*
* @sa @ref tmedia_session_mgr_start
*/
int tmedia_session_mgr_stop(tmedia_session_mgr_t* self)
{
	int ret = 0;
	tsk_list_item_t* item;
	tmedia_session_t* session;

	TSK_DEBUG_INFO("tmedia_session_mgr_stop()");

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	tsk_safeobj_lock(self);

	if(!self->started){
		goto bail;
	}

	tsk_list_foreach(item, self->sessions){
		if(!(session = item->data) || !session->plugin || !session->plugin->stop){
			TSK_DEBUG_ERROR("Invalid session");
			ret = -2;
			goto bail;
		}
		if((ret = session->plugin->stop(session))){
			TSK_DEBUG_ERROR("Failed to stop session");
			continue;
		}
	}
	self->started = tsk_false;

bail:
	tsk_safeobj_unlock(self);
	return ret;
}

/**@ingroup tmedia_session_group
* Gets local offer.
*/
const tsdp_message_t* tmedia_session_mgr_get_lo(tmedia_session_mgr_t* self)
{
	const tsk_list_item_t* item;
	const tmedia_session_t* ms;
	const tsdp_header_M_t* m;
	const tsdp_message_t* ret = tsk_null;
	
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}

	tsk_safeobj_lock(self);
	
	/* prepare the session manager if not already done (create all sessions) */
	if(TSK_LIST_IS_EMPTY(self->sessions)){
		if(_tmedia_session_mgr_load_sessions(self)){
			TSK_DEBUG_ERROR("Failed to prepare the session manager");
			goto bail;
		}
	}
	
	/* creates local sdp if not already done or update it's value (because of set_ro())*/
	if((self->ro_changed || self->state_changed || self->mediaType_changed) && self->sdp.lo){
		// delete current lo 
		TSK_OBJECT_SAFE_FREE(self->sdp.lo);
		if(self->mediaType_changed){
			// reload session with new medias and keep the old one
			_tmedia_session_mgr_load_sessions(self);
		}
		self->ro_changed = tsk_false;
		self->ro_provisional = tsk_false;
		self->state_changed = tsk_false;
		self->mediaType_changed = tsk_false;
	}

	if(self->sdp.lo){
		ret = self->sdp.lo;
		goto bail;
	}
	else if((self->sdp.lo = tsdp_message_create_empty(self->public_addr ? self->public_addr : self->addr, self->ipv6, self->sdp.lo_ver++))){	
		/* Set connection "c=" */
		tsdp_message_add_headers(self->sdp.lo,
			TSDP_HEADER_C_VA_ARGS("IN", self->ipv6 ? "IP6" : "IP4", self->public_addr ? self->public_addr : self->addr),//FIXME
			tsk_null);
	}else{
		self->sdp.lo_ver--;
		TSK_DEBUG_ERROR("Failed to create empty SDP message");
		goto bail;
	}

	/*	pass complete local sdp to the sessions to allow them to use session-level attributes */
	tmedia_session_mgr_set(self,
			TMEDIA_SESSION_SET_POBJECT(self->type, "local-sdp-message", self->sdp.lo),
			TMEDIA_SESSION_SET_NULL());

	/* gets each "m=" line from the sessions and add them to the local sdp */
	tsk_list_foreach(item, self->sessions){
		if(!(ms = item->data) || !ms->plugin){
			TSK_DEBUG_ERROR("Invalid session");
			continue;
		}
		/* prepare the media session */
		if(!ms->prepared && (_tmedia_session_prepare(TMEDIA_SESSION(ms)))){
			TSK_DEBUG_ERROR("Failed to prepare session"); /* should never happen */
			continue;
		}
		
		/* Add QoS lines to our local media */
		if((self->qos.type != tmedia_qos_stype_none) && !TMEDIA_SESSION(ms)->qos){
			TMEDIA_SESSION(ms)->qos = tmedia_qos_tline_create(self->qos.type, self->qos.strength);
		}
		
		/* add "m=" line from the session to the local sdp */
		if((m = tmedia_session_get_lo(TMEDIA_SESSION(ms)))){
			tsdp_message_add_header(self->sdp.lo, TSDP_HEADER(m));
			// media session with port equal to zero (codec mismatch) is a zombie (zombie<>ghost)
			if(ms->M.lo && ms->M.lo->port == 0){
				const tmedia_session_plugin_def_t* plugin;
				TSK_DEBUG_INFO("Media session with media type = '%s' is a zombie", ms->M.lo->media);
				if((plugin = tmedia_session_plugin_find_by_media(ms->M.lo->media))){
					self->type = (self->type & ~plugin->type);
					// do not set 'mediaType_changed' as this is not an update
				}
			}
		}
		else{
			TSK_DEBUG_ERROR("Failed to get m= line for [%s] media", ms->plugin->media);
		}
	}
	
	ret = self->sdp.lo;

bail:
	tsk_safeobj_unlock(self);

	return ret;
}


/**@ingroup tmedia_session_group
* Sets remote offer.
*/
int tmedia_session_mgr_set_ro(tmedia_session_mgr_t* self, const tsdp_message_t* sdp, tmedia_ro_type_t ro_type)
{
	const tmedia_session_t* ms;
	const tsdp_header_M_t* M;
	const tsdp_header_C_t* C; /* global "c=" line */
	const tsdp_header_O_t* O;
	tsk_size_t index = 0;
	tsk_size_t active_sessions_count = 0;
	int ret = 0;
	tsk_bool_t found;
	tsk_bool_t stopped_to_reconf = tsk_false;
	tsk_bool_t is_ro_codecs_changed = tsk_false;
	tsk_bool_t is_ro_network_info_changed = tsk_false;
	tsk_bool_t is_ro_hold_resume_changed = tsk_false;
	tsk_bool_t is_ro_loopback_address = tsk_false;
	tsk_bool_t is_media_type_changed = tsk_false;
	tsk_bool_t is_ro_media_lines_changed = tsk_false;
	tsk_bool_t is_ice_active = tsk_false;
	tsk_bool_t had_ro_sdp, had_ro_provisional, is_ro_provisional_final_matching = tsk_false;
	tmedia_qos_stype_t qos_type = tmedia_qos_stype_none;
	tmedia_type_t new_mediatype = tmedia_none;

	if(!self || !sdp){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	tsk_safeobj_lock(self);

	had_ro_sdp = (self->sdp.ro != tsk_null);
	had_ro_provisional = (had_ro_sdp && self->ro_provisional);
	is_ice_active = (self->ice.ctx_audio && (self->type & tmedia_audio) && tnet_ice_ctx_is_active(self->ice.ctx_audio))
					|| (self->ice.ctx_video && (self->type & tmedia_video) && tnet_ice_ctx_is_active(self->ice.ctx_video));

	/*	RFC 3264 subcaluse 8
		When issuing an offer that modifies the session, the "o=" line of the new SDP MUST be identical to that in the previous SDP, 
		except that the version in the origin field MUST increment by one from the previous SDP. If the version in the origin line 
		does not increment, the SDP MUST be identical to the SDP with that version number. The answerer MUST be prepared to receive 
		an offer that contains SDP with a version that has not changed; this is effectively a no-op.
	*/
	if((O = (const tsdp_header_O_t*)tsdp_message_get_header(sdp, tsdp_htype_O))){
		tsk_bool_t is_ro_provisional;
		if(self->sdp.ro_ver == (int32_t)O->sess_version){
			TSK_DEBUG_INFO("Remote offer has not changed");
			ret = 0;
			goto bail;
		}
		// Last provisional and new final sdp messages match only if:
		//  - session version diff is == 1
		//  - previous sdp was provisional and new one is final
		//  - the new final sdp is inside an answer
		is_ro_provisional = ((ro_type & tmedia_ro_type_provisional) == tmedia_ro_type_provisional);
		is_ro_provisional_final_matching = ((had_ro_provisional && !is_ro_provisional) && ((self->sdp.ro_ver + 1) == O->sess_version) && ((ro_type & tmedia_ro_type_answer) == tmedia_ro_type_answer));
		self->sdp.ro_ver = (int32_t)O->sess_version;
	}
	else{
		TSK_DEBUG_ERROR("o= line is missing");
		ret = -2;
		goto bail;
	}

	/* SDP comparison
	*/
	if((sdp && self->sdp.ro)){
		const tsdp_header_M_t *M0, *M1;
		const tsdp_header_C_t *C0, *C1;
		index = 0;
		while((M0 = (const tsdp_header_M_t*)tsdp_message_get_headerAt(self->sdp.ro, tsdp_htype_M, index))){
			M1 = (const tsdp_header_M_t*)tsdp_message_get_headerAt(sdp, tsdp_htype_M, index);
			if(!M1 || !tsk_striequals(M1->media, M0->media)){
				// media lines must be at the same index
				// (M1 == null) means media lines are not at the same index or new one have been added/removed
				is_ro_media_lines_changed = tsk_true;
			}
			
			// hold/resume
			is_ro_hold_resume_changed |= (M1 && !tsk_striequals(tsdp_header_M_get_holdresume_att(M0), tsdp_header_M_get_holdresume_att(M1)));

			// media lines
			if(!is_ro_media_lines_changed){
				is_ro_media_lines_changed
					// (M1 == null) means media lines are not at the same index or new one have been added/removed
					 |= (!M1)
					 // same media (e.g. audio)
					 || !tsk_striequals(M1->media, M0->media)
					 // same protos (e.g. SRTP)
					 || !tsk_striequals(M1->proto, M0->proto);
			}
			
			// codecs
			if(!is_ro_media_lines_changed){ // make sure it's same media at same index
				const tsk_list_item_t* codec_item0;
				const tsdp_fmt_t* codec_fmt1;
				tsk_size_t codec_index0 = 0;
				tsk_list_foreach(codec_item0, M0->FMTs){
					codec_fmt1 = (const tsdp_fmt_t*)tsk_list_find_object_by_pred_at_index(M1->FMTs, tsk_null, tsk_null, codec_index0++);
					if(!codec_fmt1 || !tsk_striequals(codec_fmt1->value, ((const tsdp_fmt_t*)codec_item0->data)->value)){
						is_ro_codecs_changed = tsk_true;
						break;
					}
				}
				// make sure M0 and M1 have same number of codecs
				is_ro_codecs_changed |= (tsk_list_find_object_by_pred_at_index(M1->FMTs, tsk_null, tsk_null, codec_index0) != tsk_null);
			}
			
			// network ports
			is_ro_network_info_changed |= ((M1 ? M1->port : 0) != (M0->port));
			
			if(!is_ro_network_info_changed){
				C0 = (const tsdp_header_C_t*)tsdp_message_get_headerAt(self->sdp.ro, tsdp_htype_C, index);
				C1 = (const tsdp_header_C_t*)tsdp_message_get_headerAt(sdp, tsdp_htype_C, index);
				// Connection informations must be both "null" or "not-null"
				if(!(is_ro_network_info_changed = !((C0 && C1) || (!C0 && !C1)))){
					if(C0){
						is_ro_network_info_changed = (!tsk_strequals(C1->addr, C0->addr) || !tsk_strequals(C1->nettype, C0->nettype) || !tsk_strequals(C1->addrtype, C0->addrtype));
					}
				}
			}
			++index;
		}
		// the index was used against current ro which means at this step there is no longer any media at "index"
		// to be sure that new and old sdp have same number of media lines, we just check that there is no media in the new sdp at "index"
		is_ro_media_lines_changed |= (tsdp_message_get_headerAt(sdp, tsdp_htype_M, index) != tsk_null);
	}

	/*
	* Make sure that the provisional response is an preview of the final as explained rfc6337 section 3.1.1. We only check the most important part (IP addr and ports).
	* It's useless to check codecs or any other caps (SRTP, ICE, DTLS...) as our offer haven't changed
	* If the preview is different than the final response than this is a bug on the remote party:
	* As per rfc6337 section 3.1.1.: 
	* - [RFC3261] requires all SDP in the responses to the INVITE request to be identical.
	* - After the UAS has sent the answer in a reliable provisional
          response to the INVITE, the UAS should not include any SDPs in
          subsequent responses to the INVITE.
    * If the remote party is buggy, then the newly generated local SDP will be sent in the ACK request
	*/
	is_ro_provisional_final_matching &= !(is_ro_media_lines_changed || is_ro_network_info_changed);
	
	/* This is to hack fake forking from ZTE => ignore SDP with loopback address in order to not start/stop the camera several
	 * times which leads to more than ten seconds for session connection.
	 * Gets the global connection line: "c="
	 * Loopback address is only invalid on 
	 */
	if((C = (const tsdp_header_C_t*)tsdp_message_get_header(sdp, tsdp_htype_C)) && C->addr){
		is_ro_loopback_address = (tsk_striequals("IP4", C->addrtype) && tsk_striequals("127.0.0.1", C->addr))
						|| (tsk_striequals("IP6", C->addrtype) && tsk_striequals("::1", C->addr));
	}
	
	/* Check if media type has changed or not
	 * For initial offer we don't need to check anything
	 */
	if(self->sdp.lo){
		new_mediatype = tmedia_type_from_sdp(sdp);
		if((is_media_type_changed = (new_mediatype != self->type))){
			tmedia_session_mgr_set_media_type(self, new_mediatype);
			TSK_DEBUG_INFO("media type has changed");
		}
	}

	TSK_DEBUG_INFO(
		"is_ice_active=%d,\n"
		"is_ro_hold_resume_changed=%d,\n"
		"is_ro_provisional_final_matching=%d,\n"
		"is_ro_media_lines_changed=%d,\n"
		"is_ro_network_info_changed=%d,\n"
		"is_ro_loopback_address=%d,\n"
		"is_media_type_changed=%d,\n"
		"is_ro_codecs_changed=%d\n",
		is_ice_active,
		is_ro_hold_resume_changed,
		is_ro_provisional_final_matching,
		is_ro_media_lines_changed,
		is_ro_network_info_changed,
		is_ro_loopback_address,
		is_media_type_changed,
		is_ro_codecs_changed
		);
	
	/*
	  * It's almost impossible to update the codecs, the connection information etc etc while the sessions are running
	  * For example, if the video producer is already started then, you probably cannot update its configuration
	  * without stoping it and restarting it again with the right config. Same for RTP Network config (ip addresses, NAT, ports, IP version, ...)
	  *
	  * "is_loopback_address" is used as a guard to avoid reconf for loopback address used for example by ZTE for fake forking. In all case
	  * loopback address won't work on embedded devices such as iOS and Android.
	  *
	 */
	if((self->started && !is_ro_loopback_address) && (is_ro_codecs_changed || is_ro_network_info_changed || is_ro_media_lines_changed || is_media_type_changed)){
		TSK_DEBUG_INFO("stopped_to_reconf=true,is_ice_active=%s", is_ice_active?"true":"false");
		if((ret = tmedia_session_mgr_stop(self))){
			TSK_DEBUG_ERROR("Failed to stop session manager");
			goto bail;
		}
		stopped_to_reconf = tsk_true;
	}

	/* update remote offer */
	TSK_OBJECT_SAFE_FREE(self->sdp.ro);
	self->sdp.ro = tsk_object_ref((void*)sdp);

	/*  - if the session is running this means no session update is required unless some important changes
	    - this check must be done after the "ro" update
		- "is_ro_hold_resume_changed" do not restart the session but updates the SDP
	*/
	if(self->started && !(is_ro_hold_resume_changed || is_ro_network_info_changed || is_ro_media_lines_changed || is_ro_codecs_changed)){
		goto end_of_sessions_update;
	}

	/* prepare the session manager if not already done (create all sessions with their codecs) 
	* if network-initiated: think about tmedia_type_from_sdp() before creating the manager */
	if(_tmedia_session_mgr_load_sessions(self)){
		TSK_DEBUG_ERROR("Failed to prepare the session manager");
		ret = -3;
		goto bail;
	}
	
	/* get global connection line (common to all sessions) 
	* Each session should override this info if it has a different one in its "m=" line
	* /!\ "remote-ip" is deprecated by "remote-sdp-message" and pending before complete remove
	*/
	if(C && C->addr){
		tmedia_session_mgr_set(self,
			TMEDIA_SESSION_SET_STR(self->type, "remote-ip", C->addr),
			TMEDIA_SESSION_SET_NULL());
	}

	/*	pass complete remote sdp to the sessions to allow them to use session-level attributes
	*/
	tmedia_session_mgr_set(self,
			TMEDIA_SESSION_SET_POBJECT(self->type, "remote-sdp-message", self->sdp.ro),
			TMEDIA_SESSION_SET_NULL());

	/* foreach "m=" line in the remote offer create/prepare a session (requires the session to be stopped)*/
	index = 0;
	while((M = (const tsdp_header_M_t*)tsdp_message_get_headerAt(sdp, tsdp_htype_M, index++))){
		found = tsk_false;
		/* Find session by media */
		if((ms = tsk_list_find_object_by_pred(self->sessions, __pred_find_session_by_media, M->media))){
			/* prepare the media session */
			if(!self->started){
				if(!ms->prepared && (_tmedia_session_prepare(TMEDIA_SESSION(ms)))){
					TSK_DEBUG_ERROR("Failed to prepare session"); /* should never happen */
					goto bail;
				}
			}
			/* set remote ro at session-level */
			if((ret = _tmedia_session_set_ro(TMEDIA_SESSION(ms), M)) == 0){
				found = tsk_true;
				++active_sessions_count;
			}
			else{
				// will send 488 Not Acceptable
				TSK_DEBUG_WARN("_tmedia_session_set_ro() failed");
				goto bail;
			}
			/* set QoS type (only if we are not the offerer) */
			if(/*!self->offerer ==> we suppose that the remote party respected our demand &&*/ qos_type == tmedia_qos_stype_none){
				tmedia_qos_tline_t* tline = tmedia_qos_tline_from_sdp(M);
				if(tline){
					qos_type = tline->type;
					TSK_OBJECT_SAFE_FREE(tline);
				}
			}
		}
		
		if(!found && (self->sdp.lo == tsk_null)){
			/* Session not supported and we are not the initial offerer ==> add ghost session */
			/*
				An offered stream MAY be rejected in the answer, for any reason.  If
			   a stream is rejected, the offerer and answerer MUST NOT generate
			   media (or RTCP packets) for that stream.  To reject an offered
			   stream, the port number in the corresponding stream in the answer
			   MUST be set to zero.  Any media formats listed are ignored.  AT LEAST
			   ONE MUST BE PRESENT, AS SPECIFIED BY SDP.
			*/
			tmedia_session_ghost_t* ghost;
			if((ghost = (tmedia_session_ghost_t*)tmedia_session_create(tmedia_ghost))){
				tsk_strupdate(&ghost->media, M->media); /* copy media */
				tsk_strupdate(&ghost->proto, M->proto); /* copy proto */
				if(!TSK_LIST_IS_EMPTY(M->FMTs)){
					tsk_strupdate(&ghost->first_format, ((const tsdp_fmt_t*)TSK_LIST_FIRST_DATA(M->FMTs))->value); /* copy format */
				}
				tsk_list_push_back_data(self->sessions, (void**)&ghost);
			}
			else{
				TSK_DEBUG_ERROR("Failed to create ghost session");
				continue;
			}
		}
	}

end_of_sessions_update:

	/* update QoS type */
	if(!self->offerer && (qos_type != tmedia_qos_stype_none)){
		self->qos.type = qos_type;
	}

	/* signal that ro has changed (will be used to update lo) unless there was no ro_sdp */
	self->ro_changed = (had_ro_sdp && (is_ro_hold_resume_changed || is_ro_network_info_changed || is_ro_media_lines_changed || is_ro_codecs_changed));

	/* update "provisional" info */
	self->ro_provisional = ((ro_type & tmedia_ro_type_provisional) == tmedia_ro_type_provisional);
	
	if(self->ro_changed){
		/* update local offer before restarting the session manager otherwise neg_codecs won't match if new codecs
		 have been added or removed */
		(tmedia_session_mgr_get_lo(self));
	}
	/* manager was started and we stopped it in order to reconfigure it (codecs, network, ....) 
	* When ICE is active ("is_ice_active" = yes), the media session will be explicitly restarted when conncheck succeed or fail.
	*/
	if(stopped_to_reconf && !is_ice_active){
		if((ret = tmedia_session_mgr_start(self))){
			TSK_DEBUG_ERROR("Failed to re-start session manager");
			goto bail;
		}
	}

	// will send [488 Not Acceptable] / [BYE] if no active session
	ret = (self->ro_changed && active_sessions_count <= 0) ? -0xFF : 0;

bail:
	tsk_safeobj_unlock(self);
	return ret;
}

const tsdp_message_t* tmedia_session_mgr_get_ro(tmedia_session_mgr_t* self)
{
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}
	return self->sdp.ro;
}

/**@ingroup tmedia_session_group
* Holds the session as per 3GPP TS 34.610
* @param self the session manager managing the session to hold.
* @param type the type of the sessions to hold (you can combine several medias. e.g. audio|video|msrp).
* @retval Zero if succeed and non zero error code otherwise.
* @sa @ref tmedia_session_mgr_resume
*/
int tmedia_session_mgr_hold(tmedia_session_mgr_t* self, tmedia_type_t type)
{
	const tsk_list_item_t* item;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	tsk_list_foreach(item, self->sessions){
		tmedia_session_t* session = TMEDIA_SESSION(item->data);
		if(((session->type & type) == session->type) && session->M.lo){
			if(tsdp_header_M_hold(session->M.lo, tsk_true) == 0){
				self->state_changed = tsk_true;
				session->lo_held = tsk_true;
			}
		}
	}
	return 0;
}

/**@ingroup tmedia_session_group
* Indicates whether the specified medias are held or not.
* @param self the session manager
* @param type the type of the medias to check (you can combine several medias. e.g. audio|video|msrp)
* @param local whether to check local or remote medias
*/
tsk_bool_t tmedia_session_mgr_is_held(tmedia_session_mgr_t* self, tmedia_type_t type, tsk_bool_t local)
{	
	const tsk_list_item_t* item;
	tsk_bool_t have_these_sessions = tsk_false;
	
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_false;
	}

	tsk_list_foreach(item, self->sessions){
		tmedia_session_t* session = TMEDIA_SESSION(item->data);
		if((session->type & type) == session->type){
			if(local && session->M.lo){
				have_these_sessions = tsk_true;
				if(!tsdp_header_M_is_held(session->M.lo, tsk_true)){
					return tsk_false;
				}
			}
			else if(!local && session->M.ro){
				have_these_sessions = tsk_true;
				if(!tsdp_header_M_is_held(session->M.ro, tsk_false)){
					return tsk_false;
				}
			}
		}
	}
	/* none is held */
	return have_these_sessions ? tsk_true : tsk_false;
}

/**@ingroup tmedia_session_group
* Resumes the session as per 3GPP TS 34.610. Should be previously held
* by using @ref tmedia_session_mgr_hold.
* @param self the session manager managing the session to resume.
* @param type the type of the sessions to resume (you can combine several medias. e.g. audio|video|msrp).
* @retval Zero if succeed and non zero error code otherwise.
* @sa @ref tmedia_session_mgr_hold
*/
int tmedia_session_mgr_resume(tmedia_session_mgr_t* self, tmedia_type_t type, tsk_bool_t local)
{
	const tsk_list_item_t* item;
	int ret = 0;
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	
	tsk_list_foreach(item, self->sessions){
		tmedia_session_t* session = TMEDIA_SESSION(item->data);
		if(((session->type & type) == session->type) && session->M.lo){
			if((ret = tsdp_header_M_resume(session->M.lo, local)) == 0){
				self->state_changed = tsk_true;
				if(local){
					session->lo_held = tsk_false;
				}
				else{
					session->ro_held = tsk_false;
				}
			}
		}
	}
	return ret;
}

/**@ingroup tmedia_session_group
* Adds new medias to the manager. A media will only be added if it is missing
* or previously removed (slot with port equal to zero).
* @param self The session manager
* @param The types of the medias to add (ou can combine several medias. e.g. audio|video|msrp)
* @retval Zero if succeed and non zero error code otherwise.
*/
int tmedia_session_mgr_add_media(tmedia_session_mgr_t* self, tmedia_type_t type)
{
	tsk_size_t i = 0;
	tmedia_session_t* session;
	const tmedia_session_plugin_def_t* plugin;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	/* for each registered plugin match with the supplied type */
	while((i < TMED_SESSION_MAX_PLUGINS) && (plugin = __tmedia_session_plugins[i++])){
		if((plugin->type & type) == plugin->type){
			/* check whether we already support this media */
			if((session = (tmedia_session_t*)tsk_list_find_object_by_pred(self->sessions, __pred_find_session_by_type, &plugin->type)) && session->plugin){
				if(session->prepared){
					TSK_DEBUG_WARN("[%s] already active", plugin->media);
				}
				else{
					/* exist but unprepared(port=0) */
					_tmedia_session_prepare(session);
					if(self->started && session->plugin->start){
						session->plugin->start(session);
					}
					self->state_changed = tsk_true;
				}
			}
			else{
				/* session not supported */
				self->state_changed = tsk_true;
				if((session = tmedia_session_create(plugin->type))){
					if(self->started && session->plugin->start){
						session->plugin->start(session);
					}
					tsk_list_push_back_data(self->sessions, (void**)(&session));
					self->state_changed = tsk_true;
				}
			}
		}
	}

	return self->state_changed ? 0 : -2;
}

/**@ingroup tmedia_session_group
* Removes medias from the manager. This action will stop the media and sets it's port value to zero (up to the session).
* @param self The session manager
* @param The types of the medias to remove (ou can combine several medias. e.g. audio|video|msrp)
* @retval Zero if succeed and non zero error code otherwise.
*/
int tmedia_session_mgr_remove_media(tmedia_session_mgr_t* self, tmedia_type_t type)
{
	const tsk_list_item_t* item;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	
	tsk_list_foreach(item, self->sessions){
		tmedia_session_t* session = TMEDIA_SESSION(item->data);
		if(((session->type & type) == session->type) && session->plugin->stop){
			if(!session->plugin->stop(session)){
				self->state_changed = tsk_true;
			}
		}
	}
	return 0;
}

/**@ingroup tmedia_session_group
* Sets QoS type and strength
* @param self The session manager
* @param qos_type The QoS type
* @param qos_strength The QoS strength
* @retval Zero if succeed and non-zero error code otherwise
*/
int tmedia_session_mgr_set_qos(tmedia_session_mgr_t* self, tmedia_qos_stype_t qos_type, tmedia_qos_strength_t qos_strength)
{
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	self->qos.type = qos_type;
	self->qos.strength = qos_strength;
	return 0;
}

/**@ingroup tmedia_session_group
* Indicates whether all preconditions are met
* @param self The session manager
* @retval @a tsk_true if all preconditions have been met and @a tsk_false otherwise
*/
tsk_bool_t tmedia_session_mgr_canresume(tmedia_session_mgr_t* self)
{
	const tsk_list_item_t* item;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_true;
	}

	tsk_list_foreach(item, self->sessions){
		tmedia_session_t* session = TMEDIA_SESSION(item->data);
		if(session && session->qos && !tmedia_qos_tline_canresume(session->qos)){
			return tsk_false;
		}
	}
	return tsk_true;
}


/**@ingroup tmedia_session_group
* Checks whether the manager holds at least one valid session (media port <> 0)
*/
tsk_bool_t tmedia_session_mgr_has_active_session(tmedia_session_mgr_t* self)
{
	const tsk_list_item_t* item;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_false;
	}

	tsk_list_foreach(item, self->sessions){
		tmedia_session_t* session = TMEDIA_SESSION(item->data);
		if(session && session->M.lo && session->M.lo->port){
			return tsk_true;
		}
	}
	return tsk_false;
}

int tmedia_session_mgr_send_dtmf(tmedia_session_mgr_t* self, uint8_t event)
{
	tmedia_session_audio_t* session;
	static const tmedia_type_t audio_type = tmedia_audio;
	int ret = -3;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	session = (tmedia_session_audio_t*)tsk_list_find_object_by_pred(self->sessions, __pred_find_session_by_type, &audio_type);
	if(session){
		session = tsk_object_ref(session);
		ret = tmedia_session_audio_send_dtmf(TMEDIA_SESSION_AUDIO(session), event);
		TSK_OBJECT_SAFE_FREE(session);
	}
	else{
		TSK_DEBUG_ERROR("No audio session associated to this manager");
	}

	return ret;
}

int tmedia_session_mgr_set_t140_ondata_cbfn(tmedia_session_mgr_t* self, const void* context, tmedia_session_t140_ondata_cb_f func)
{
	tmedia_session_t* session;
	int ret = -1;
	if((session = tmedia_session_mgr_find(self, tmedia_t140))){
		ret = tmedia_session_t140_set_ondata_cbfn(session, context, func);
		TSK_OBJECT_SAFE_FREE(session);
	}
	return ret;
}

int tmedia_session_mgr_send_t140_data(tmedia_session_mgr_t* self, enum tmedia_t140_data_type_e data_type, const void* data_ptr, unsigned data_size)
{
	tmedia_session_t* session;
	int ret = -1;
	if((session = tmedia_session_mgr_find(self, tmedia_t140))){
		ret = tmedia_session_t140_send_data(session, data_type, data_ptr, data_size);
		TSK_OBJECT_SAFE_FREE(session);
	}
	return ret;
}

int tmedia_session_mgr_set_onrtcp_cbfn(tmedia_session_mgr_t* self, tmedia_type_t media_type, const void* context, tmedia_session_rtcp_onevent_cb_f fun)
{
	tmedia_session_t* session;
	tsk_list_item_t *item;

	if(!self){
		TSK_DEBUG_ERROR("Invlid parameter");
		return -1;
	}

	tsk_list_lock(self->sessions);
	tsk_list_foreach(item, self->sessions){
		if(!(session = item->data) || !(session->type & media_type)){
			continue;
		}
		tmedia_session_set_onrtcp_cbfn(session, context, fun);
	}
	tsk_list_unlock(self->sessions);

	return 0;
}

int tmedia_session_mgr_send_rtcp_event(tmedia_session_mgr_t* self, tmedia_type_t media_type, tmedia_rtcp_event_type_t event_type, uint32_t ssrc_media)
{
	tmedia_session_t* session;
	tsk_list_item_t *item;

	if(!self){
		TSK_DEBUG_ERROR("Invlid parameter");
		return -1;
	}

	tsk_list_lock(self->sessions);
	tsk_list_foreach(item, self->sessions){
		if(!(session = item->data) || !(session->type & media_type)){
			continue;
		}
		tmedia_session_send_rtcp_event(session, event_type, ssrc_media);
	}
	tsk_list_unlock(self->sessions);

	return 0;
}

int tmedia_session_mgr_send_file(tmedia_session_mgr_t* self, const char* path, ...)
{
	tmedia_session_msrp_t* session;
	tmedia_type_t msrp_type = tmedia_msrp;
	int ret = -3;

	if(!self || !path){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	session = (tmedia_session_msrp_t*)tsk_list_find_object_by_pred(self->sessions, __pred_find_session_by_type, &msrp_type);
	if(session && session->send_file){
		va_list ap;
		va_start(ap, path);
		session = tsk_object_ref(session);
		ret = session->send_file(TMEDIA_SESSION_MSRP(session), path, &ap);
		TSK_OBJECT_SAFE_FREE(session);
		va_end(ap);
	}
	else{
		TSK_DEBUG_ERROR("No MSRP session associated to this manager or session does not support file transfer");
	}

	return ret;
}

int tmedia_session_mgr_send_message(tmedia_session_mgr_t* self, const void* data, tsk_size_t size, const tmedia_params_L_t *params)
{
	tmedia_session_msrp_t* session;
	tmedia_type_t msrp_type = tmedia_msrp;
	int ret = -3;

	if(!self || !size || !data){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	session = (tmedia_session_msrp_t*)tsk_list_find_object_by_pred(self->sessions, __pred_find_session_by_type, &msrp_type);
	if(session && session->send_message){
		session = tsk_object_ref(session);
		ret = session->send_message(TMEDIA_SESSION_MSRP(session), data, size, params);
		TSK_OBJECT_SAFE_FREE(session);
	}
	else{
		TSK_DEBUG_ERROR("No MSRP session associated to this manager or session does not support file transfer");
	}

	return ret;
}

int tmedia_session_mgr_set_msrp_cb(tmedia_session_mgr_t* self, const void* callback_data, tmedia_session_msrp_cb_f func)
{
	tmedia_session_msrp_t* session;
	tmedia_type_t msrp_type = tmedia_msrp;
	
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	
	if((session = (tmedia_session_msrp_t*)tsk_list_find_object_by_pred(self->sessions, __pred_find_session_by_type, &msrp_type))){
		session->callback.data = callback_data;
		session->callback.func = func;
		return 0;
	}
	else{
		TSK_DEBUG_ERROR("No MSRP session associated to this manager or session does not support file transfer");
		return -2;
	}
}

int tmedia_session_mgr_set_onerror_cbfn(tmedia_session_mgr_t* self, const void* usrdata, tmedia_session_onerror_cb_f fun)
{
	tmedia_session_t* session;
	tsk_list_item_t *item;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	self->onerror_cb.fun = fun;
	self->onerror_cb.usrdata = usrdata;

	tsk_list_lock(self->sessions);
	tsk_list_foreach(item, self->sessions){
		if(!(session = item->data)){
			continue;
		}
		tmedia_session_set_onerror_cbfn(session, usrdata, fun);
	}
	tsk_list_unlock(self->sessions);

	return 0;
}

/** internal function used to load sessions */
static int _tmedia_session_mgr_load_sessions(tmedia_session_mgr_t* self)
{
	tsk_size_t i = 0;
	tmedia_session_t* session;
	const tmedia_session_plugin_def_t* plugin;
	
#define has_media(media_type) (tsk_list_find_object_by_pred(self->sessions, __pred_find_session_by_type, &(media_type)))
	
	if(TSK_LIST_IS_EMPTY(self->sessions) || self->mediaType_changed){
		/* for each registered plugin create a session instance */
		while((i < TMED_SESSION_MAX_PLUGINS) && (plugin = __tmedia_session_plugins[i++])){
			if((plugin->type & self->type) == plugin->type && !has_media(plugin->type)){// we don't have a session with this media type yet
				if((session = tmedia_session_create(plugin->type))){
					/* do not call "tmedia_session_mgr_set()" here to avoid applying parms before the creation of all session */

					/* set other default values */

					// set callback functions
					tmedia_session_set_onerror_cbfn(session, self->onerror_cb.usrdata, self->onerror_cb.fun);
					
					/* push session */
					tsk_list_push_back_data(self->sessions, (void**)(&session));
				}
			}
			else if(!(plugin->type & self->type) && has_media(plugin->type)){// we have media session from previous call (before update)
				tsk_list_remove_item_by_pred(self->sessions, __pred_find_session_by_type, &(plugin->type));
			}
		}
		/* set default values and apply params*/
		tmedia_session_mgr_set(self,
				TMEDIA_SESSION_SET_POBJECT(tmedia_audio, "ice-ctx", self->ice.ctx_audio),
				TMEDIA_SESSION_SET_POBJECT(tmedia_video, "ice-ctx", self->ice.ctx_video),

				TMEDIA_SESSION_SET_STR(self->type, "local-ip", self->addr),
				TMEDIA_SESSION_SET_STR(self->type, "local-ipver", self->ipv6 ? "ipv6" : "ipv4"),
				TMEDIA_SESSION_SET_INT32(self->type, "bandwidth-level", self->bl),
				TMEDIA_SESSION_SET_NULL());
	}
#undef has_media
	return 0;
}

/* internal function */
static int _tmedia_session_mgr_clear_sessions(tmedia_session_mgr_t* self)
{
	if(self && self->sessions){
		tsk_list_clear_items(self->sessions);
	}
	return 0;
}

/* internal function */
static int _tmedia_session_mgr_apply_params(tmedia_session_mgr_t* self)
{
	tsk_list_item_t *it1, *it2;
	tmedia_param_t* param;
	tmedia_session_t* session;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	/* If no parameters ==> do nothing (not error) */
	if(TSK_LIST_IS_EMPTY(self->params)){
		return 0;
	}
	
	tsk_list_lock(self->params);

	tsk_list_foreach(it1, self->params){
		if(!(param = it1->data)){
			continue;
		}

		/* For us */
		if(param->plugin_type == tmedia_ppt_manager){
			continue;
		}
		
		/* For the session (or consumer or producer or codec) */
		tsk_list_foreach(it2, self->sessions){
			if(!(session = it2->data) || !session->plugin){
				continue;
			}
			if(session->plugin->set && (session->type & param->media_type) == session->type){
				session->plugin->set(session, param);
			}
		}
	}

	/* Clean up params */
	tsk_list_clear_items(self->params);

	tsk_list_unlock(self->params);
	
	return 0;
}

//=================================================================================================
//	Media Session Manager object definition
//
static tsk_object_t* tmedia_session_mgr_ctor(tsk_object_t * self, va_list * app)
{
	tmedia_session_mgr_t *mgr = self;
	if(mgr){
		mgr->sessions = tsk_list_create();

		mgr->sdp.lo_ver = TSDP_HEADER_O_SESS_VERSION_DEFAULT;
		mgr->sdp.ro_ver = -1;

		mgr->qos.type = tmedia_qos_stype_none;
		mgr->qos.strength = tmedia_qos_strength_optional;
		mgr->bl = tmedia_defaults_get_bl();

		tsk_safeobj_init(mgr);
	}
	return self;
}

static tsk_object_t* tmedia_session_mgr_dtor(tsk_object_t * self)
{ 
	tmedia_session_mgr_t *mgr = self;
	if(mgr){
		TSK_OBJECT_SAFE_FREE(mgr->sessions);

		TSK_OBJECT_SAFE_FREE(mgr->sdp.lo);
		TSK_OBJECT_SAFE_FREE(mgr->sdp.ro);
		
		TSK_OBJECT_SAFE_FREE(mgr->params);

		TSK_OBJECT_SAFE_FREE(mgr->natt_ctx);
		TSK_FREE(mgr->public_addr);

		TSK_OBJECT_SAFE_FREE(mgr->ice.ctx_audio);
		TSK_OBJECT_SAFE_FREE(mgr->ice.ctx_video);

		TSK_FREE(mgr->addr);

		tsk_safeobj_deinit(mgr);
	}

	return self;
}

static const tsk_object_def_t tmedia_session_mgr_def_s = 
{
	sizeof(tmedia_session_mgr_t),
	tmedia_session_mgr_ctor, 
	tmedia_session_mgr_dtor,
	tsk_null, 
};
const tsk_object_def_t *tmedia_session_mgr_def_t = &tmedia_session_mgr_def_s;

