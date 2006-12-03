//  AEdit -:-  (C)opyright 2004 Jonas Jarvoll	
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

#include <gui/image.h>
#include <util/resources.h>
#include <util/string.h>

#include "icons.h"

// Partly based on the work by Rick Caudill
os::Image* GetStockIcon(const char* zStockName, StockSize iSize)
{
	os::String cRealStockName=zStockName;

	switch(iSize)
	{
		case STOCK_SIZE_MENUITEM:
			cRealStockName+="_16";
			break;
		case STOCK_SIZE_TOOLBAR:
			cRealStockName+="_24";
			break;
		default:
			cRealStockName+="_16";
			break;
	}

	os::Image * pcImage = new os::BitmapImage();
	os::Resources cRes( get_image_id() );
	os::ResStream * pcStream = cRes.GetResourceStream( cRealStockName.c_str() );
	if( pcStream == NULL )
		return NULL;
	
	pcImage->Load( pcStream );
	return pcImage;	
}


