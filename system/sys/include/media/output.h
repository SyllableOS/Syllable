/*  Media Output class
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
 
#ifndef __F_MEDIA_OUTPUT_H_
#define __F_MEDIA_OUTPUT_H_

#include <util/resource.h>
#include <atheos/types.h>
#include <atheos/time.h>
#include <gui/view.h>
#include <util/string.h>
#include <media/packet.h>
#include <media/format.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


/** Media Output.
 * \ingroup media
 * \par Description:
 * The Media Output class can be used to output audio or video data. It should
 * only be used together with the Media Manager class.
 *
 * \author	Arno Klenke
 *****************************************************************************/

class MediaOutput : public Resource
{
protected:
	virtual ~MediaOutput();
public:
	MediaOutput();
	
	virtual String 			GetIdentifier();
	virtual uint32			GetPhysicalType();
	virtual View*			GetConfigurationView();
	
	virtual bool			FileNameRequired();
	virtual status_t		Open( String zFileName );
	virtual void			Close();
	virtual void			Clear();
	
	virtual uint32			GetOutputFormatCount();
	virtual MediaFormat_s	GetOutputFormat( uint32 nIndex );
	virtual uint32			GetSupportedStreamCount();
	virtual status_t		AddStream( String zName, MediaFormat_s sFormat );
	
	
	virtual View*			GetVideoView( uint32 nStream );
	virtual void			UpdateView( uint32 nStream );
	
	virtual status_t		WritePacket( uint32 nStream, MediaPacket_s* psPacket );
	virtual uint64			GetDelay( bool bNonSharedOnly = false );
	virtual uint64			GetBufferSize( bool bNonSharedOnly = false );
private:
};

typedef MediaOutput* init_media_output();

}

#endif


































