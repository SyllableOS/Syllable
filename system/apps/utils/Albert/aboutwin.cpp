/*
 * Albert 0.4 * Copyright (C)2002 Henrik Isaksson
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *
 * AboutWindow class - window that shows a 'splash' image and version
 * information.
 *
 */

#include "aboutwin.h"
#include <gui/font.h>
#include <gui/desktop.h>

#include <stdio.h>

class BitmapView : public View {
	public:
	BitmapView(const Rect &cFrame, const char *cName, Bitmap *bitmap)
		: View(cFrame, cName, CF_FOLLOW_NONE, WID_WILL_DRAW)
	{
		m_Bitmap = bitmap;
		m_Font = new Font;
		m_Font->SetFamilyAndStyle("Lucida Sans", "Regular");
		m_Font->SetSize(7);
	}

	~BitmapView()
	{
		m_Font->Release();
	}

	void Paint(const Rect &cUpdateRect)
	{
		char *text1 = "Albert V0.4 2002-07-04 Copyright (C) 2002 Henrik Isaksson.";
		char *text2 = "Albert is released under the GNU General Public License.";
		SetDrawingMode(DM_COPY);
		DrawBitmap(m_Bitmap, cUpdateRect, cUpdateRect);

		SetDrawingMode(DM_OVER);
		SetFont(m_Font);
		
		font_height fh;
		GetFontHeight(&fh);
		
		MovePenTo(GetBounds().Width()/2 - GetStringWidth(text1)/2, 2 + fh.ascender);
		DrawString(text1);

		MovePenTo(GetBounds().Width()/2 - GetStringWidth(text2)/2, GetBounds().Height() - 2 - fh.descender);
		DrawString(text2);

	}

	private:
	Bitmap		*m_Bitmap;
	Font		*m_Font;
};

AboutWindow::AboutWindow(const Rect & r)
	:Window(r, "AboutAlbert", "About Albert",
//	WND_NO_CLOSE_BUT | WND_NO_ZOOM_BUT | WND_NO_DEPTH_BUT |
//	WND_NO_BORDER | WND_NO_TITLE |
	WND_NOT_RESIZABLE,
	CURRENT_DESKTOP)
{

    os::Resources cRes(get_image_id());
	os::ResStream* pcStream = cRes.GetResourceStream("albert.png");
	if(pcStream == NULL) {
		throw(os::errno_exception(""));
    } else {
		m_Bitmap = LoadImage( pcStream );
		if(m_Bitmap) {
			Rect	frame = m_Bitmap->GetBounds();
			Desktop	desktop;

			BitmapView *bmv = new BitmapView(frame, "bitmap", m_Bitmap);
			AddChild(bmv);

			Point offset(desktop.GetResolution());
	
			offset.x = offset.x/2 - frame.Width()/2;
			offset.y = offset.y/2 - frame.Height()/2;

			SetFrame(frame + offset);
		}
		delete pcStream;
    }
}

AboutWindow::~AboutWindow()
{
	delete m_Bitmap;
}

bool AboutWindow::OkToQuit()
{
	return true;
}

void AboutWindow::HandleMessage(Message *msg)
{
	Window::HandleMessage(msg);
}

Bitmap* AboutWindow::LoadImage(StreamableIO* pcStream)
{
    os::Translator* pcTrans = NULL;

    os::TranslatorFactory* pcFactory = new os::TranslatorFactory;
    pcFactory->LoadAll();
    
    os::Bitmap* pcBitmap = NULL;
    
    try {
	uint8  anBuffer[8192];
	uint   nFrameSize = 0;
	int    x = 0;
	int    y = 0;
	
	os::BitmapFrameHeader sFrameHeader;
	for (;;) {
	    int nBytesRead = pcStream->Read( anBuffer, sizeof(anBuffer) );

	    if (nBytesRead == 0 ) {
		break;
	    }

	    if ( pcTrans == NULL ) {
		int nError = pcFactory->FindTranslator( "", os::TRANSLATOR_TYPE_IMAGE, anBuffer, nBytesRead, &pcTrans );
		if ( nError < 0 ) {
		    return( NULL );
		}
	    }
	    
	    pcTrans->AddData( anBuffer, nBytesRead, nBytesRead != sizeof(anBuffer) );

	    while( true ) {
	    if ( pcBitmap == NULL ) {
		os::BitmapHeader sBmHeader;
		if ( pcTrans->Read( &sBmHeader, sizeof(sBmHeader) ) != sizeof( sBmHeader ) ) {
		    break;
		}
		pcBitmap = new os::Bitmap( sBmHeader.bh_bounds.Width() + 1, sBmHeader.bh_bounds.Height() + 1, os::CS_RGB32 );
	    }
	    if ( nFrameSize == 0 ) {
		if ( pcTrans->Read( &sFrameHeader, sizeof(sFrameHeader) ) != sizeof( sFrameHeader ) ) {
		    break;
		}
		x = sFrameHeader.bf_frame.left * 4;
		y = sFrameHeader.bf_frame.top;
		nFrameSize = sFrameHeader.bf_data_size;
	    }
	    uint8 pFrameBuffer[8192];
	    nBytesRead = pcTrans->Read( pFrameBuffer, min( nFrameSize, sizeof(pFrameBuffer) ) );
	    if ( nBytesRead <= 0 ) {
		break;
	    }
	    nFrameSize -= nBytesRead;
	    uint8* pDstRaster = pcBitmap->LockRaster();
	    int nSrcOffset = 0;
	    for ( ; y <= sFrameHeader.bf_frame.bottom && nBytesRead > 0 ; ) {
		int nLen = min( nBytesRead, sFrameHeader.bf_bytes_per_row - x );
		memcpy( pDstRaster + y * pcBitmap->GetBytesPerRow() + x, pFrameBuffer + nSrcOffset, nLen );
		if ( x + nLen == sFrameHeader.bf_bytes_per_row ) {
		    x = 0;
		    y++;
		} else {
		    x += nLen;
		}
		nBytesRead -= nLen;
		nSrcOffset += nLen;
	    }
	    }
	}
	return( pcBitmap );
    }
    catch( ... ) {
	return( NULL );
    }
}


























































