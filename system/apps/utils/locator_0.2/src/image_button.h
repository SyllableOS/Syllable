/*
    Locator - A fast file finder for Syllable
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)
                                                                                                                                                                                                        
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                                                                                                                                        
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                                                                                                                                        
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <translation/translator.h>
#include <gui/button.h>
#include <storage/file.h>
#include <gui/bitmap.h>
#include <gui/font.h>
#include <gui/window.h>

using namespace os;

class ImageButton;

enum // ImageButton text position settings
{
	IB_TEXT_RIGHT = 100,
	IB_TEXT_LEFT,
	IB_TEXT_TOP,
	IB_TEXT_BOTTOM
};

class ImageButton : public Button
{
	public:
		ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage, uint32 nTextPosition = IB_TEXT_RIGHT, bool bDrawBorder = true,
		             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE);
		ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage, Bitmap *pcBitmap, uint32 nTextPosition = IB_TEXT_RIGHT, bool bDrawBorder = true,
		             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE);
		ImageButton( Rect cFrame, const char *pzName, const char *pzLabel, Message *pcMessage, uint32 nWidth, uint32 nHeight, uint8 *pnBuffer, uint32 nTextPosition = IB_TEXT_RIGHT, bool bDrawBorder = true,
		             uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_CLEAR_BACKGROUND|WID_FULL_UPDATE_ON_RESIZE);
		~ImageButton( );
		void SetImage( Bitmap *pcBitmap );
		void SetImage( uint32 nWidth, uint32 nHeight, uint8 *pnBuffer );
		bool SetImageFromFile( string zFile );
		void ClearImage( void );
		Bitmap *GetImage( void );
		void SetTextPosition( uint32 nTextPosition );
		uint32 GetTextPosition( void );
		void Paint( const Rect &cUpdateRect );
		Point GetPreferredSize( bool bLargest ) const;
				
	private:
		Bitmap *m_pcBitmap;
		uint32 m_nTextPosition;
		bool m_bDrawBorder;
};
		             












