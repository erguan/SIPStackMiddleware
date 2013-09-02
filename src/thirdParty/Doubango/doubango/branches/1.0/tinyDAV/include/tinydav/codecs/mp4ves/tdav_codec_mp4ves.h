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

/**@file tdav_codec_mp4ves.h
 * @brief MP4V-ES codec plugin
 * RTP payloader/depayloader follows RFC 3016.
 * ISO-IEC-14496-2: http://www.csus.edu/indiv/p/pangj/aresearch/video_compression/presentation/ISO-IEC-14496-2_2001_MPEG4_Visual.pdf
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *
 * @date Created: Th Dec 2 16:54:58 2010 mdiop
 */
#ifndef TINYDAV_CODEC_MP4VES_H
#define TINYDAV_CODEC_MP4VES_H

#include "tinydav_config.h"

#if HAVE_FFMPEG

#include "tinymedia/tmedia_codec.h"

#include <libavcodec/avcodec.h>

TDAV_BEGIN_DECLS

typedef struct tdav_codec_mp4ves_s
{
	TMEDIA_DECLARE_CODEC_VIDEO;

	int profile;

	struct{
		uint8_t* ptr;
		tsk_size_t size;
	} rtp;

	// Encoder
	struct{
		AVCodec* codec;
		AVCodecContext* context;
		AVFrame* picture;
		void* buffer;
	} encoder;
	
	// decoder
	struct{
		AVCodec* codec;
		AVCodecContext* context;
		AVFrame* picture;
		
		void* accumulator;
		uint8_t ebit;
		tsk_size_t accumulator_pos;
		uint16_t last_seq;
	} decoder;
}
tdav_codec_mp4ves_t;

TINYDAV_GEXTERN const tmedia_codec_plugin_def_t *tdav_codec_mp4ves_plugin_def_t;

TDAV_END_DECLS

#endif /* HAVE_FFMPEG */

#endif /* TINYDAV_CODEC_MP4VES_H */

