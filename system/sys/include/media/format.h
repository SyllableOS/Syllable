/*  Media Format class
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
 
#ifndef __F_MEDIA_FORMAT_H_
#define __F_MEDIA_FORMAT_H_

#include <atheos/types.h>
#include <gui/guidefines.h>
#include <util/string.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


/** Media Types..
 * \ingroup media
 * \par Description:
 * Types of AV data. Used by the MediaFormat_s structure.
 *
 * \author	Arno Klenke
 *****************************************************************************/

enum media_types {
	MEDIA_TYPE_AUDIO,
	MEDIA_TYPE_VIDEO,
	MEDIA_TYPE_OTHER
};


/** Media Format.
 * \ingroup media
 * \par Description:
 * The Media Format structure describes the format of one stream. It can be 
 * allocated by calling the GetStreamFormat() method of the MediaInput class.
 * \par Note:
 * If one of the members is 0 then this information does not matter.
 *
 * \author	Arno Klenke
 *****************************************************************************/


typedef struct {
	/** \par Description: 
	* MEDIA_TYPE_* flags.
	*****************************************************************************/
	int 	nType;
	/** \par Description: 
	* Internal codec name.
	*****************************************************************************/
	String	zName;
	
	/** \par Description: 
	* Whether the stream has a variable bitrate
	*****************************************************************************/
	bool	bVBR;
	
	/** \par Description: 
	* Bit rate of this stream.
	*****************************************************************************/
	int		nBitRate;
	
	/** \par Description: 
	* Audio only : The sample rate of the stream.
	*****************************************************************************/
	int		nSampleRate;
	/** \par Description: 
	* Audio only : Number of audio channels.
	*****************************************************************************/
	int		nChannels;
	
	/** \par Description: 
	* Whether the stream has a variable framerate
	*****************************************************************************/
	bool	bVFR;
	
	/** \par Description: 
	* Video only : Framerate
	*****************************************************************************/
	float	vFrameRate;
	
	/** \par Description: 
	* Video only : Width of the video picture.
	*****************************************************************************/
	int		nWidth;
	/** \par Description: 
	* Video only : Height of the video picture.
	*****************************************************************************/
	int		nHeight;
	/** \par Description: 
	* Video only : Colorspace of the video picture.
	*****************************************************************************/
	color_space eColorSpace;
	/** \par Description: 
	* Private data for communication between the input and the codec
	*****************************************************************************/
	void *pPrivate;
} MediaFormat_s;

}

#endif



















