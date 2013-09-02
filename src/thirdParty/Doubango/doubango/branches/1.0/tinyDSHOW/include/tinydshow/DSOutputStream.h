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
#ifndef DSOUTPUTSTREAM_H
#define DSOUTPUTSTREAM_H

#include <tinydshow_config.h>

#include <streams.h>

#include "tsk_mutex.h"

class DSOutputFilter;

class DSOutputStream : public CSourceStream
{
public:
	DSOutputStream(HRESULT *phr, DSOutputFilter *pParent, LPCWSTR pPinName);
	virtual ~DSOutputStream();

	void setFps(int fps_);
	void showOverlay(int value);

	HRESULT setImageFormat(UINT width, UINT height);
	bool getImageFormat(UINT &width, UINT &height);
	STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q) { return E_NOTIMPL; };
	inline bool lockBuffer(){
		if(this->mutex){
			return tsk_mutex_lock(this->mutex) == 0;
		}
		return false;
	}
	inline bool unlockBuffer(){
		if(this->mutex){
			return tsk_mutex_unlock(this->mutex) == 0;
		}
		return false;
	}

public:
	void			*buffer;
	int				buffer_size;
	LONGLONG		frameNumber;

protected: // Overrides
	HRESULT GetMediaType(CMediaType *pMediaType);
	HRESULT DecideBufferSize(IMemAllocator *pMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT OnThreadCreate();
	HRESULT OnThreadDestroy();
	HRESULT FillBuffer(IMediaSample *pSample);

private:
	/*inline */void TransfertBuffer(void* src, void* dest, long lSize);
	inline HRESULT DrawOverLay(void *pBuffer, long lSize);

private:
	// TIMING
	REFERENCE_TIME	frameLength;
	int				fps;

	// sizing
	UINT width;
	UINT height;
	
	// overlaying
	bool			overlay;
	BITMAPINFO		bitmapInfo;
	void			*paintBuffer;
	HDC				paintDC;
	HBITMAP			hDibSection;
	HGDIOBJ			hObject;

	tsk_mutex_handle_t* mutex;
};


#endif
