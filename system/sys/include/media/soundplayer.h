/*  Media Sound Player class
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
 
#ifndef __F_MEDIA_SOUNDPLAYER_H_
#define __F_MEDIA_SOUNDPLAYER_H_

#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/threads.h>
#include <util/string.h>
#include <media/packet.h>
#include <media/format.h>
#include <media/manager.h>
#include <media/input.h>
#include <media/output.h>
#include <media/codec.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


/** Media Sound Player.
 * \ingroup media
 * \par Description:
 * The Sound Player class provides an easy way to play sound files.
 * \par Note:
 * Only the first track of the file / device is played.
 *
 * \author	Arno Klenke
 *****************************************************************************/

class MediaSoundPlayer
{
public:
	MediaSoundPlayer();
	virtual ~MediaSoundPlayer();
	
	virtual status_t 	SetFile( String zFileName );
	virtual void 		Play();
	virtual bool		IsPlaying();
	virtual void		Stop();
	
	virtual void PlayThread();
	
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
	virtual void 		Clear();
	class Private;
	Private* m;
	
};

}

#endif





































