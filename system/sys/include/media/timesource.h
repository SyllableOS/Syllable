/*  Media Timesource class
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
 
#ifndef __F_MEDIA_TIMESOURCE_H_
#define __F_MEDIA_TIMESOURCE_H_

#include <atheos/time.h>
#include <util/string.h>


namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


/** Media Timesource.
 * \ingroup media
 * \par Description:
 * The media timesource class is a class that abstracts a time source.
 * For example, this class can be used to sync one media output with another one.
 *
 * \author	Arno Klenke
 *****************************************************************************/

class MediaTimeSource
{
public:
	MediaTimeSource() {};
	virtual ~MediaTimeSource() {};
	virtual String 			GetIdentifier() = 0;
	virtual bigtime_t		GetCurrentTime() = 0;
private:
};

}

#endif

