#ifndef _WALL_PAPER_LOOPER_H
#define _WALL_PAPER_LOOPER_H

#include <util/looper.h>
#include <gui/view.h>
#include <util/message.h>

using namespace os;

class DockWallpaperChangerLooper : public Looper
{
public:
	DockWallpaperChangerLooper(View*);
	~DockWallpaperChangerLooper();
	
	virtual void TimerTick(int);
private:
	View* pcParentView;
};

#endif		

