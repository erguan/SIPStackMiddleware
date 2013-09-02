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

/**@file tsip_header_Security_Server.h
 * @brief SIP header 'Security-Server' as per RFC 3329.
 *
 * @date Created: Sat Nov 8 16:54:58 2009 mdiop
 */
#ifndef _TSIP_HEADER_SECURITY_SERVER_H_
#define _TSIP_HEADER_SECURITY_SERVER_H_

#include "tinysip_config.h"
#include "tinysip/headers/tsip_header.h"

#include "tnet_types.h"

TSIP_BEGIN_DECLS


#define TSIP_HEADER_SECURITY_SERVER_VA_ARGS()		tsip_header_Security_Server_def_t

////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @brief	SIP header 'Security-Server' as per RFC 3329.
///
/// @par ABNF : Security-Server	= 	"Security-Server" HCOLON sec-mechanism *(COMMA sec-mechanism)
/// sec-mechanism	= 	mechanism-name *( SEMI mech-parameters )
/// mechanism-name	= 	( "digest" / "tls" / "ipsec-ike" / "ipsec-man" / token )
/// mech-parameters	= 	( preference / digest-algorithm / digest-qop / digest-verify / mech-extension )
/// preference	= 	"q" EQUAL qvalue
/// digest-algorithm	= 	"d-alg" EQUAL token
/// digest-qop	= 	"d-qop" EQUAL token
/// digest-verify	= 	"d-ver" EQUAL LDQUOT 32LHEX RDQUOT
/// mech-extension	= 	generic-param
///
/// mechanism-name   = ( "ipsec-3gpp" )
/// mech-parameters    = ( algorithm / protocol /mode /
///                              encrypt-algorithm / spi /
///                              port1 / port2 )
/// algorithm          = "alg" EQUAL ( "hmac-md5-96" /
///                                          "hmac-sha-1-96" )
/// protocol           = "prot" EQUAL ( "ah" / "esp" )
/// mode               = "mod" EQUAL ( "trans" / "tun" )
/// encrypt-algorithm  = "ealg" EQUAL ( "des-ede3-cbc" / "null" )
/// spi                = "spi" EQUAL spivalue
/// spivalue           = 10DIGIT; 0 to 4294967295
/// port1              = "port1" EQUAL port
/// port2              = "port2" EQUAL port
/// port               = 1*DIGIT
/// 
/// 	
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tsip_header_Security_Server_s
{	
	TSIP_DECLARE_HEADER;

	//! sec-mechanism (e.g. "digest" / "tls" / "ipsec-3gpp")
	char* mech;
	//! algorithm (e.g. "hmac-md5-96" / "hmac-sha-1-96")
	char* alg;
	//! protocol (e.g. "ah" / "esp")
	char* prot;
	//! mode (e.g. "trans" / "tun")
	char* mod;
	//! encrypt-algorithm (e.g. "des-ede3-cbc" / "null")
	char* ealg;
	//! client port
	tnet_port_t port_c;
	//! server port
	tnet_port_t port_s;
	//! client spi
	uint32_t spi_c;
	//! server spi
	uint32_t spi_s;
	//! preference
	double q;
}
tsip_header_Security_Server_t;

typedef tsk_list_t tsip_header_Security_Servers_L_t;

TINYSIP_API tsip_header_Security_Server_t* tsip_header_Security_Server_create();
TINYSIP_API tsip_header_Security_Server_t* tsip_header_Security_Server_create_null();

TINYSIP_API tsip_header_Security_Servers_L_t *tsip_header_Security_Server_parse(const char *data, tsk_size_t size);

TINYSIP_GEXTERN const tsk_object_def_t *tsip_header_Security_Server_def_t;

TSIP_END_DECLS

#endif /* _TSIP_HEADER_SECURITY_SERVER_H_ */

