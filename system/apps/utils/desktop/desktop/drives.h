#ifndef DRIVES_H
#define DRIVES_H

#include "include.h"

using namespace os;


static struct mounted_drives
{
    char zMenu[1024];
    char zSize[64];
    char zUsed[64];
    char zAvail[64];
    char vol_name[256];
    char zType[64];
    char zPer[1024];
}drives;

//typedef vector <mounted_drives> t_Info;



class Drives : public Menu {
	
	public:
		Drives(View* pcView);
		void Unmount(int32 nUnmount);
		void Mount();
		void GetDrivesInfo();
};
#endif








