#include "drives.h"
//#include <storage/volume,h>  // working on a volume class that will aid the mount menu
#include "messages.h"

#include <gui/checkmenu.h>

/*
** name:       human
** purpose:    Convert bytes to bytes, Kilobytes, Megabytes, or GigaBytes; depending how big nValue is
** parameters: 
** returns:    
*/
void human( char* pzBuffer, off_t nValue )
{
    if ( nValue > (1024*1024*1024) )
    {
        sprintf( pzBuffer, "%.1f GB", ((double)nValue) / (1024.0*1024.0*1024.0) );
    }
    else if ( nValue > (1024*1024) )
    {
        sprintf( pzBuffer, "%.1f MB", ((double)nValue) / (1024.0*1024.0) );
    }
    else if ( nValue > 1024 )
    {
        sprintf( pzBuffer, "%.1f KB", ((double)nValue) / 1024.0 );
    }
    else
    {
        sprintf( pzBuffer, "%.1f b", ((double)nValue) );
    }
}

Drives::Drives(View* pcView) : Menu(Rect(0,0,0,0),"Mounted Drives",ITEMS_IN_COLUMN)
{
	GetDrivesInfo();
	AddItem(new MenuSeparator());
	AddItem("Settings...", new Message(M_SHOW_DRIVE_SETTINGS));
	SetTargetForItems(pcView);
}

void Drives::GetDrivesInfo()
{

    fs_info     fsInfo;
    int  nMountCount = 0;
    char szTmp[1024];
    char zSize[64];
    char zUsed[64];
    char zAvail[64];
    char zPer[64];
    int x = get_mount_point_count();
    
    nMountCount = 0;

    for( int i = 0; i < x; i++ )
    {
        if( get_mount_point( i, szTmp, PATH_MAX ) < 0 )
            continue;

        int nFD = open( szTmp, O_RDONLY );
        if( nFD < 0 )
            continue;

        if( get_fs_info( nFD, &fsInfo ) >= 0 )
            nMountCount++;

        close( nFD );
    }


	
    for ( int i = 0 ; i < nMountCount ; ++i )
    {
        if ( get_mount_point( i, szTmp, PATH_MAX ) < 0 )//
        {
        	  
     
            continue;
        }

        int nFD = open( szTmp, O_RDONLY );
        
        if ( nFD < 0 )
        {	
            continue;
        }

        if ( get_fs_info( nFD, &fsInfo ) >= 0 )
        {
            
               
        }       
        human( zSize, fsInfo.fi_total_blocks * fsInfo.fi_block_size );
        human( zUsed, (fsInfo.fi_total_blocks - fsInfo.fi_free_blocks) *fsInfo.fi_block_size );
        human( zAvail, fsInfo.fi_free_blocks * fsInfo.fi_block_size );
     	sprintf(zPer,"%.1f%%", ((double)fsInfo.fi_free_blocks / ((double)fsInfo.fi_total_blocks)) * 100.0);
       
		char  pzInfo[2048];
		sprintf(pzInfo, "FileSystem Type:      %s  \n\nSize of Partition:      %s  \n\nAmount Used:          %s  \n\nAmount Available:   %s  \n\nDevice Path:             %s   \n",fsInfo.fi_driver_name,zSize, zUsed,zAvail,fsInfo.fi_device_path);  
         
        Message *pcMsg = new Message(M_SHOW_DRIVE_INFO);
        pcMsg->AddString("Alert_Name",fsInfo.fi_volume_name);
        pcMsg->AddString("Alert_Info",pzInfo);
        pcMsg->AddString("Alert_Devuce",fsInfo.fi_device_path);
        
		Menu* pcMenu = new Menu(Rect(0,0,0,0),fsInfo.fi_volume_name,ITEMS_IN_COLUMN);
		pcMenu->AddItem(new MenuItem("Show Info...",pcMsg));
		AddItem(pcMenu);
		
		
}
}

void Drives::Unmount(int32 nUnmount)
{
}

void Drives::Mount()
{
}

void Drives::TimerTick(int nID)
{
	
}







































