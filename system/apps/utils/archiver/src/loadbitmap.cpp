#include "loadbitmap.h"
#include <gui/font.h>
#include <gui/desktop.h>

#include <stdio.h>

BitmapImage* LoadImageFromFile(const os::String& cString)
{
	os::BitmapImage * pcImage = new os::BitmapImage();
	File* pcFile = new File(cString);
	pcImage->Load(pcFile);
	delete pcFile;
	return pcImage;
}

/************************************************
* Description: Loads a bitmapimage from resource
* Author: Rick Caudill
* Date: Thu Mar 18 21:32:37 2004
*************************************************/
os::BitmapImage* LoadImageFromResource( const os::String &cName )
{
	os::BitmapImage * pcImage = new os::BitmapImage();
	os::Resources cRes( get_image_id() );
	os::ResStream * pcStream = cRes.GetResourceStream( cName.c_str() );
	if( pcStream == NULL )
	{
		throw( os::errno_exception("") );
	}
	pcImage->Load( pcStream );
	return pcImage;
}





