#include "drives.h"
#include "messages.h"

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
        sprintf( pzBuffer, "%.1f Gb", ((double)nValue) / (1024.0*1024.0*1024.0) );
    }
    else if ( nValue > (1024*1024) )
    {
        sprintf( pzBuffer, "%.1f Mb", ((double)nValue) / (1024.0*1024.0) );
    }
    else if ( nValue > 1024 )
    {
        sprintf( pzBuffer, "%.1f Kb", ((double)nValue) / 1024.0 );
    }
    else
    {
        sprintf( pzBuffer, "%.1f b", ((double)nValue) );
    }
}

Drives::Drives(View* pcView) : Menu(Rect(0,0,0,0),"Mounted Drives",ITEMS_IN_COLUMN)
{
GetDrivesInfo();
SetTargetForItems(pcView);
}

void Drives::GetDrivesInfo()
{

    mounted_drives   d_drives;
    fs_info     fsInfo;
    int		 nMountCount = 0;
    char        szTmp[1024];
    char zSize[64];
    char        zUsed[64];
    char zAvail[64];
    //int nInt = 1;
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
        if ( get_mount_point( i, szTmp, PATH_MAX ) < 0 )
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
     	sprintf(d_drives.zPer,"%.1f%%", ((double)fsInfo.fi_free_blocks / ((double)fsInfo.fi_total_blocks)) * 100.0);
     
		
		char  pzInfo[2048];
		sprintf(pzInfo, "FileSystem Type:    %s  \n\nSize of Partition:    %s  \n\nPercent Used:         %s  \n\nPercent Available:  %s  \n",fsInfo.fi_driver_name,zSize, zUsed,zAvail);  
        
     	   
        Message *pcMsg = new Message(M_SHOW_DRIVE_INFO);
        pcMsg->AddString("Alert_Name",fsInfo.fi_volume_name);
        pcMsg->AddString("Alert_Info",pzInfo);
        
        
		Menu* pcMenu = new Menu(Rect(0,0,0,0),fsInfo.fi_volume_name,ITEMS_IN_COLUMN);
		pcMenu->AddItem(new MenuItem("Show Info...",pcMsg));
		
		
		if (strcmp(fsInfo.fi_driver_name,"afs")!= 0) //test to make sure unmount option is not for afs partitions
			pcMenu->AddItem(new MenuItem("Unmount",NULL));
		AddItem(pcMenu);
		
}
}

void Drives::Unmount(int32 nUnmount)
{
}

void Drives::Mount()
{
}















