#include "drives.h"


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

Drives::Drives() : Menu(Rect(0,0,0,0),"Mounted Drives  ",ITEMS_IN_COLUMN)
{
	
	t_Info me = GetDrivesInfo();
	for (uint32 i =0; i < me.size(); i++){
		printf("%s\n",me[i].vol_name);
		}
}

t_Info Drives::GetDrivesInfo()
{

    mounted_drives   d_drives;
    fs_info     fsInfo;
    int		 nMountCount = 0;
    char        szTmp[1024];
    char zSize[64];
    char        zUsed[64];
    char zAvail[64];
    int nInt = 1;
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


	    for ( int i = 0 ; i < nMountCount ; ++i ) {
      if ( get_mount_point( i, szTmp, PATH_MAX ) < 0 ) {
        continue;
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
            
               
               
        
        
       
        d_drives.vol_name = fsInfo.fi_volume_name;
        printf(d_drives.vol_name);
        d_drives.zSize = zSize;
        d_drives.zUsed = zUsed;
        d_drives.zAvail = zAvail;
        d_drives.zType = fsInfo.fi_driver_name;

        sprintf(d_drives.zPer,"%.1f%%", ((double)fsInfo.fi_free_blocks / ((double)fsInfo.fi_total_blocks)) * 100.0);

        sprintf(d_drives.zMenu,"%s     ",fsInfo.fi_volume_name);
			nInt = nInt + 1;
            //human( zSize, fsInfo.fi_total_blocks * fsInfo.fi_block_size );
            //human( zUsed, (fsInfo.fi_total_blocks - fsInfo.fi_free_blocks) *fsInfo.fi_block_size );
            //human( zAvail, fsInfo.fi_free_blocks * fsInfo.fi_block_size );
        }

     
   		
   }
}
}

void Drives::Unmount(int32 nUnmount)
{
}

void Drives::Mount()
{
}









