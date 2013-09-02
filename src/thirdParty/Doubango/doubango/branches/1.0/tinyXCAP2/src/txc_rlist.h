/*
* Copyright (C) 2009 Mamadou Diop.
*
* Contact: Mamadou Diop <diopmamadou@yahoo.fr>
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
* GNU Lesser General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*
*/

/**@file txc_rlist.h
 * @brief RFC 4826 subclause 3: <a href="http://tools.ietf.org/html/rfc4826#section-3">Resource Lists Documents</a>
 *
 * @author Mamadou Diop <diopmamadou(at)yahoo.fr>
 *
 * @date Created: Sat Nov 8 16:54:58 2009 mdiop
 */
#ifndef _TINYXCAP_TXC_RLIST_H_
#define _TINYXCAP_TXC_RLIST_H_

#include "tinyxcap_config.h"
#include "tsk_xml.h"

#include "tsk_object.h"

#define TXC_RLIST_CREATE(buffer, size)				tsk_object_new(txc_rlist_def_t, (const char*)buffer, (size_t)size)
#define TXC_RLIST_SAFE_FREE(self)					tsk_object_unref(self), self = 0

/**@def TXC_RLIST_ENTRY_CREATE
* Create a new @ref txc_rlist_entry_t object. You must call @ref TXC_RLIST_ENTRY_SAFE_FREE to free the newly created object.
* @sa @ref TXC_RLIST_ENTRY_SAFE_FREE
*/
/**@def TXC_RLIST_ENTRY_SAFE_FREE
* Safely free @ref txc_rlist_entry_t object previously created using @ref TXC_RLIST_ENTRY_CREATE.
* @sa @ref TXC_RLIST_ENTRY_CREATE
*/

#define TXC_RLIST_ENTRY_CREATE()	tsk_object_new(txc_rlist_entry_def_t)
#define TXC_RLIST_ENTRY_SAFE_FREE(self)	tsk_object_unref(self), self = 0

/**@def TXC_RLIST_LIST_CREATE
* Create a new @ref txc_rlist_list_t object. You must call @ref TXC_RLIST_LIST_SAFE_FREE to free the newly created object.
* @sa @ref TXC_RLIST_LIST_SAFE_FREE
*/
/**@def TXC_RLIST_LIST_SAFE_FREE
* Safely free @ref txc_rlist_list_t object previously created using @ref TXC_RLIST_LIST_CREATE.
* @sa @ref TXC_RLIST_LIST_CREATE
*/
#define TXC_RLIST_LIST_CREATE()		tsk_object_new(txc_rlist_list_def_t)
#define TXC_RLIST_LIST_SAFE_FREE(self)	tsk_object_unref(self), self = 0

/**@def TXC_RLIST_LIST2_CREATE
* Create a new @ref txc_rlist_list2_t object. You must call @ref TXC_RLIST_LIST2_SAFE_FREE to free the newly created object.
* @sa @ref TXC_RLIST_LIST2_SAFE_FREE
*/
/**@def TXC_RLIST_LIST2_SAFE_FREE
* Safely free @ref txc_rlist_list2_t object previously created using @ref TXC_RLIST_LIST2_CREATE.
* @sa @ref TXC_RLIST_LIST2_CREATE
*/
#define TXC_RLIST_LIST2_CREATE()		tsk_object_new(txc_rlist_list2_def_t)
#define TXC_RLIST_LIST2_SAFE_FREE(self)		tsk_object_unref(self), self = 0

/**@def TXC_RLIST_EXTERNAL_CREATE
* Create a new @ref txc_rlist_external_t object. You must call @ref TXC_RLIST_EXTERNAL_SAFE_FREE to free the newly created object.
* @sa @ref TXC_RLIST_EXTERNAL_SAFE_FREE
*/
/**@def TXC_RLIST_EXTERNAL_SAFE_FREE
* Safely free @ref txc_rlist_external_t object previously created using @ref TXC_RLIST_EXTERNAL_CREATE.
* @sa @ref TXC_RLIST_EXTERNAL_CREATE
*/
#define TXC_RLIST_EXTERNAL_CREATE()		tsk_object_new(txc_rlist_external_def_t)
#define TXC_RLIST_EXTERNAL_SAFE_FREE(self)	tsk_object_unref(self), self = 0

/** Resource-list entry element 
*/
typedef struct txc_rlist_entry_s
{
	TSK_DECLARE_OBJECT;

	char* uri; /**< The uri*/
	char* display_name; /**< The display-name*/
	char* last_modified; /**< Last modified datetime */
	char* etag; /**< The entity tag */
	char* list; /**< The parent list */
}
txc_rlist_entry_t;
typedef tsk_list_t txc_rlist_entry_L_t; /**< List of @ref txc_rlist_entry_t elements */

/** Simple resource-list list element
*/
typedef struct txc_rlist_list_s
{
	TSK_DECLARE_OBJECT;

	char* display_name; /**< The display-name*/
	char* name; /**< The name*/
}
txc_rlist_list_t;
typedef tsk_list_t txc_rlist_list_L_t; /**< List of @ref txc_rlist_list_t elements */
typedef tsk_list_t txc_rlist_listx_L_t; /**< List of @ref txc_rlist_list_t or @ref txc_rlist_list2_t elements*/

/** Resource-list external element
*/
typedef struct txc_rlist_external_s
{
	TSK_DECLARE_OBJECT;

	char* anchor; /**< The anchor*/
	char* list;	/**< The parent lis*/
}
txc_rlist_external_t;
typedef tsk_list_t txc_rlist_external_L_t; /**< List of @ref txc_rlist_external_t elements */

/** Complex resource-list list element
*/
typedef struct txc_rlist_list2_s
{
	TSK_DECLARE_OBJECT;

	char* display_name; /**< The list display-name*/
	char* name; /**< The list name */
	txc_rlist_entry_L_t*	entries; /**< All entries */
	txc_rlist_external_L_t* externals; /**< All extenals */
}
txc_rlist_list2_t;
typedef tsk_list_t txc_rlist_list2_L_t; /**< List of @ref txc_rlist_list2_t elements */
//typedef tsk_list_t txc_rlist_list2_LL_t; /**< List of @ref txc_rlist_list2_L_t elements */

/** Resource-lists document context
*/
typedef struct txc_rlist_s
{
	TSK_DECLARE_OBJECT;

	xmlDocPtr docPtr; /**< Pointer to the XML document */
}
txc_rlist_t;

//void txc_rlist_list_init(txc_rlist_list_t *list);
void txc_rlist_list_set(txc_rlist_list_t *list, const char* name, const char* display_name);
//void txc_rlist_list_free(void **list);

//void txc_rlist_list2_init(txc_rlist_list2_t *list2);
void txc_rlist_list2_set(txc_rlist_list2_t *list2, const char* name, const char* display_name);
void txc_rlist_list2_add_external(txc_rlist_list2_t *list2, const char* anchor);
void txc_rlist_list2_add_entry(txc_rlist_list2_t *list2, const char* uri, const char* display_name);
//void txc_rlist_list2_free(void **list2);

//void txc_rlist_entry_init(txc_rlist_entry_t *entry);
void txc_rlist_entry_set(txc_rlist_entry_t *entry, const char* uri, const char* display_name);
//void txc_rlist_entry_free(void **entry);

//void txc_rlist_external_init(txc_rlist_external_t *external);
void txc_rlist_external_set(txc_rlist_external_t *external, const char* anchor);
//void txc_rlist_external_free(void **external);

txc_rlist_entry_t* txc_rlist_entry_from_xml(const xmlNodePtr node, const char* lname);
txc_rlist_external_t* txc_rlist_external_from_xml(const xmlNodePtr node, const char* lname);
txc_rlist_list_t* txc_rlist_list_from_xml(const xmlNodePtr node);
txc_rlist_list2_t* txc_rlist_list2_from_xml(const xmlNodePtr node);

//TINYXCAP_API txc_rlist_t* txc_rlist_create(const char* buffer, size_t size);
TINYXCAP_API txc_rlist_listx_L_t* txc_rlist_get_all_lists(const txc_rlist_t* rlist, int type2);
TINYXCAP_API txc_rlist_entry_L_t* txc_rlist_get_entries_by_list(const txc_rlist_t* rlist, const char* lname);
TINYXCAP_API txc_rlist_external_L_t* txc_rlist_get_externals_by_list(const txc_rlist_t* rlist, const char* lname);
TINYXCAP_API char* txc_rlist_entry_serialize(const txc_rlist_entry_t *entry);
TINYXCAP_API char* txc_rlist_entry_serialize2(const char* uri, const char* displayname);
TINYXCAP_API char* txc_rlist_external_serialize(const txc_rlist_external_t *external);
TINYXCAP_API char* txc_rlist_list_serialize(const txc_rlist_list_t *list);
TINYXCAP_API char* txc_rlist_list_serialize2(const char* name, const char* displayname, const char* anchor);
TINYXCAP_API char* txc_rlist_list2_serialize(const txc_rlist_list2_t *list2);
TINYXCAP_API char* txc_rlist_list2_L_serialize(const txc_rlist_list2_L_t *list22);
TINYXCAP_API char* txc_rlist_add_xml_header(const char* xml_content);
//TINYXCAP_API void txc_rlist_free(txc_rlist_t **rlist);

TINYXCAP_API const void *txc_rlist_def_t;
TINYXCAP_API const void *txc_rlist_entry_def_t;
TINYXCAP_API const void *txc_rlist_list_def_t;
TINYXCAP_API const void *txc_rlist_list2_def_t;
TINYXCAP_API const void *txc_rlist_external_def_t;


#endif /* _TINYXCAP_TXC_RLIST_H_ */