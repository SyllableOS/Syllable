#include "WallpaperChangerLooper.h"
#include "messages.h"

/*************************************************
* Description: Looper
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
DockWallpaperChangerLooper::DockWallpaperChangerLooper(View* pcParent) : Looper("DockWallpaperChangerLooper",IDLE_PRIORITY)
{
	pcParentView = pcParent;
}

/*************************************************
* Description: Destructor
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
DockWallpaperChangerLooper::~DockWallpaperChangerLooper()
{
}

/*************************************************
* Description: Timer
* Author: Rick Caudill
* Date: Thu Mar 18 20:17:32 2004
**************************************************/
void DockWallpaperChangerLooper::TimerTick(int nID)
{
	pcParentView->GetLooper()->PostMessage(new Message(M_CHANGE_FILE),pcParentView);
}








