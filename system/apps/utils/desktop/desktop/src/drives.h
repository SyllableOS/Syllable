#ifndef DRIVES_H
#define DRIVES_H

#include "include.h"

using namespace os;

class Drives : public Menu {
	
	public:
		Drives(View* pcView);
		void Unmount(int32 nUnmount);
		void Mount();
		void GetDrivesInfo();
		void TimerTick(int nID);
};
#endif











