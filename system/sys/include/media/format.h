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

/** Media physical object types.
 * \ingroup media
 * \par Description:
 * Physical object types. Returned by the GetPhysicalType() method.
 * It is a 32bit value with the following format:
 * Byte   4: Class (e.g. hardware or software )
 * Byte 2-3: Subclass (e.g. soundcard)
 * Byte   1: Connector
 *
 * \author	Arno Klenke
 *****************************************************************************/

enum media_physical_types {
	MEDIA_PHY_UNKNOWN = 0x00000000,
	
	/* Software objects */
	MEDIA_PHY_SOFTWARE = 0x01000000,
	MEDIA_PHY_SOFT_DEMUX = 0x01000100,
	MEDIA_PHY_SOFT_CODEC = 0x0100200,
	MEDIA_PHY_SOFT_MUX = 0x01000300,
	MEDIA_PHY_SOFT_FILTER = 0x01000400,
	MEDIA_PHY_SOFT_SYNC = 0x01000500,
	
	/* Hardware objects */
	MEDIA_PHY_HARDWARE = 0x02000000,
	
	/* Soundcard */
	MEDIA_PHY_SOUNDCARD = 0x02000100,
	MEDIA_PHY_SOUNDCARD_LINE_OUT = 0x02000101,
	MEDIA_PHY_SOUNDCARD_SPDIF_OUT = 0x02000102,
	MEDIA_PHY_SOUNDCARD_LINE_IN = 0x02000103,
	MEDIA_PHY_SOUNDCARD_MIC_IN = 0x02000104,
	MEDIA_PHY_SOUNDCARD_SPDIF_IN = 0x02000105,
	
	/* Videocard */
	MEDIA_PHY_VIDEOCARD = 0x02000200,
	MEDIA_PHY_VIDEOCARD_ANALOG_IN = 0x02000201,
	
	/* CDROM */
	MEDIA_PHY_CD_DVD = 0x02000300,
	MEDIA_PHY_CD_DVD_ANALOG_IN = 0x02000301,
	MEDIA_PHY_CD_DVD_DIGITAL_IN = 0x02000302,
	
	/* Screen */
	MEDIA_PHY_SCREEN = 0x02000400,
	MEDIA_PHY_SCREEN_OUT = 0x02000401
};

#define MEDIA_PHY_CLASS(x) ( x & 0xff000000 )
#define MEDIA_PHY_SUBCLASS(x) ( x & 0x00ffff00 )
#define MEDIA_PHY_CONNECTOR(x) ( x & 0x000000ff )

/** Media types.
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
	* Audio only : The resolution of one sample.
	*****************************************************************************/	
	int		nSampleResolution;
	
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
	double	vFrameRate;
	
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

#define MEDIA_CLEAR_FORMAT( s ) \
s.nType = os::MEDIA_TYPE_OTHER; \
s.zName = ""; \
s.bVBR = false; \
s.nBitRate = 0; \
s.nSampleRate = 0; \
s.nSampleResolution = 16; \
s.nChannels = 0; \
s.bVFR = false; \
s.vFrameRate = 0; \
s.nWidth = 0; \
s.nHeight = 0; \
s.eColorSpace = os::CS_NO_COLOR_SPACE; \
s.pPrivate = NULL;

}

#endif





















