// Whisper -:-  (C)opyright 2001-2002 Kristian Van Der Vliet
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <gui/gfxtypes.h>

#include "image_button.h"

ImageButton::ImageButton(Rect cFrame, const char* pzName, const char* pzLabel, Message* pcMessage) : Button(cFrame, pzName, pzLabel, pcMessage, CF_FOLLOW_LEFT, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE)
{
	pcButtonImage=new Bitmap(cFrame.Width()+1, cFrame.Height()+1, CS_RGB32, 0x0002);
	bMouseOver=false;
}

ImageButton::~ImageButton()
{
	delete pcButtonImage;
}

void ImageButton::Paint(const Rect &cUpdateRect)
{

	if(GetValue().AsBool()==true)
	{
		Rect cBounds = GetBounds();
		cBounds.top-=1;
		cBounds.left-=1;
		cBounds.bottom+=1;
		cBounds.right+=1;
		DrawFrame(cBounds,FRAME_RECESSED);

		Rect cBitmapRect=pcButtonImage->GetBounds();
		cBitmapRect.top+=1;
		cBitmapRect.left+=1;
		cBitmapRect.bottom-=1;
		cBitmapRect.right-=1;
		DrawBitmap(pcButtonImage, cBitmapRect, cBitmapRect);

		return;
	}

	if(bMouseOver==true)
	{
		Rect cBounds = GetBounds();
		cBounds.top-=1;
		cBounds.left-=1;
		cBounds.bottom+=1;
		cBounds.right+=1;
		DrawFrame(cBounds,FRAME_RAISED);

		Rect cBitmapRect=pcButtonImage->GetBounds();
		cBitmapRect.top+=1;
		cBitmapRect.left+=1;
		cBitmapRect.bottom-=1;
		cBitmapRect.right-=1;
		DrawBitmap(pcButtonImage, cBitmapRect, cBitmapRect);

		return;
	}

	Rect cBitmapRect=pcButtonImage->GetBounds();
	DrawBitmap(pcButtonImage, cBitmapRect, cBitmapRect);

	return;
}

void ImageButton::MouseMove(const Point &cNewPos, int nCode, uint32 nButtons, Message* pcData)
{
	Rect cBounds=GetBounds();

	if((cNewPos.x>=cBounds.left)&&(cNewPos.x<=cBounds.right))
	{
		if((cNewPos.y>=cBounds.top)&&(cNewPos.y<=cBounds.bottom))
		{
			if(bMouseOver==false)
			{
				bMouseOver=true;
				Invalidate(cBounds);
				Flush();
			}

			return;
		}
	}

	if(bMouseOver==true)
	{
		bMouseOver=false;
		Invalidate(cBounds);
		Flush();
	}

	return;
}

void ImageButton::SetBitmap(uint8* pnData, uint nNumBytes)
{
	uint8* nRaster;
	Color32_s cColNormal;

	nRaster=pcButtonImage->LockRaster();
	cColNormal=get_default_color(COL_NORMAL);

	for(uint nCurrentByte=0;nCurrentByte<nNumBytes;nCurrentByte+=4)
	{
		if(pnData[nCurrentByte+3]!=0)
		{
			(*nRaster++)=pnData[nCurrentByte];
			(*nRaster++)=pnData[nCurrentByte+1];
			(*nRaster++)=pnData[nCurrentByte+2];
			nRaster++;
		}
		else	// This is odd, but I can't work out why the alpha doesn't
				// work properly, so this *does* work for now at least!
		{
			(*nRaster++)=cColNormal.red;
			(*nRaster++)=cColNormal.green;
			(*nRaster++)=cColNormal.blue;
			nRaster++;
		}
	}

	pcButtonImage->UnlockRaster();

	return;
}

