/*  Media Manager class
 *  Copyright (C) 2003 Arno Klenke
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */
 
#ifndef __F_MEDIA_MANAGER_H_
#define __F_MEDIA_MANAGER_H_

#include <gui/rect.h>
#include <atheos/types.h>
#include <atheos/image.h>
#include <util/string.h>
#include <util/messenger.h>
#include <media/packet.h>
#include <media/format.h>
#include <media/input.h>
#include <media/codec.h>
#include <media/output.h>
#include <vector>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

/** Media Manager.
 * \ingroup media
 * \par Description:
 * The Media Manager class is used to allocate input / output and codec objects.
 *
 * \author	Arno Klenke
 *****************************************************************************/

class MediaManager
{
public:
	MediaManager();
	virtual ~MediaManager();
	
	void AddRef();
	
	static MediaManager* GetInstance();
	
	static MediaManager* Get();
	void Put();
	
	bool			IsValid();
	Messenger		GetServerLink();
	
	MediaInput* 	GetBestInput( String zFileName );
	MediaInput* 	GetDefaultInput();
	void			SetDefaultInput( os::String zIdentifier );
	MediaInput* 	GetInput( uint32 nIndex );
	MediaCodec*		GetBestCodec( MediaFormat_s sInternalFormat, MediaFormat_s sExternalFormat, bool bEncode );
	MediaCodec*		GetCodec( uint32 nIndex );
	MediaOutput*	GetBestOutput( String zFileName, String zIdentifier );
	MediaOutput*	GetDefaultAudioOutput();
	void			SetDefaultAudioOutput( os::String zIdentifier );
	MediaOutput*	GetDefaultVideoOutput();
	void			SetDefaultVideoOutput( os::String zIdentifier );
	MediaOutput*	GetOutput( uint32 nIndex );
	
	virtual void _reserved1();
    virtual void _reserved2();
    virtual void _reserved3();
    virtual void _reserved4();
    virtual void _reserved5();
    virtual void _reserved6();
    virtual void _reserved7();
    virtual void _reserved8();
    virtual void _reserved9();
    virtual void _reserved10();
private:
	class Private;
	Private *m;
	static MediaManager* s_pcInstance;
};

}

#endif













































