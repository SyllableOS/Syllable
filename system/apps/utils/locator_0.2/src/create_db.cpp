/*
    Locator - A fast file finder for Syllable
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)
                                                                                                                                                                                                        
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                                                                                                                                        
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                                                                                                                                        
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "util.h"
#include "locator.h"


//char zRoot[1024];
string zRoot;
char zFile[1024];
char zLinkBuf[1024];
int  nCCount = 0;

int nDirCount = 0;
int nFileCount = 0;

unsigned long nStartTime;
unsigned long nStopTime;

FILE *pfData;
FILE *pfIdx;  
FILE *phInfo;

void process_dir( const char *pzDir );
void resolve_link( char *pzLink );


int main( int argc, char **argv )
{

  int nResult = 0;

//  strcpy( zRoot, argv[1] ? argv[1] : "/" );

  zRoot = (string)(argv[1] ? argv[1] : "/" );

  string zDataFile = app_path() + (string)DATA_FILE;
  string zIndexFile = app_path() + (string)INDEX_FILE;
  string zInfoFile = app_path() + (string)INFO_FILE;

  if( (pfData = fopen( zDataFile.c_str(), "w" )) )
  {

    if( (pfIdx = fopen( zIndexFile.c_str(), "w" )) )
    {

      nStartTime = get_real_time( ) / 1000000;
      process_dir( zRoot.c_str() );
      nStopTime = get_real_time( ) / 1000000;
      fclose( pfIdx );

	  File *pcConfig = new File( (string)(app_path() + (string)INFO_FILE), O_WRONLY | O_CREAT );  
	  if( pcConfig ) {
	  	  Message *pcInfo = new Message();
	  	  pcInfo->AddInt32( "FileCount", nFileCount );
	  	  pcInfo->AddInt32( "DirCount", nDirCount );
	  	  pcInfo->AddInt32( "TimeStamp", nStopTime );\
	  	  
	      uint32 nSize = pcInfo->GetFlattenedSize( );
    	  void *pBuffer = malloc( nSize );
	      pcInfo->Flatten( (uint8 *)pBuffer, nSize );

    	  pcConfig->Write( pBuffer, nSize );
      
	      delete pcConfig;
    	  free( pBuffer );
  	  }

      printf("%d Directories and %d Files indexed in %ld seconds.\n", nDirCount, nFileCount, nStopTime - nStartTime );

    } else {

      printf("Could not create index file %s\n", zInfoFile.c_str() );
      nResult = 1;

    }
    
    fclose( pfData );
  
  } else {

    printf("Could not create data file %s\n", zDataFile.c_str() );
    nResult = 1;

  }

  exit( nResult );
}
 


void process_dir( const char *pzDir )
{

  struct stat sStats;
  struct dirent *psDirent;
  std::vector<String *> cDirList;
  DIR *pDir;  

  fprintf( pfData, "D%s\n", pzDir );
  fprintf( pfIdx, "%ld,%s\n", ftell( pfData ), pzDir );
  nDirCount++;

  if( (pDir = opendir( pzDir )) )
  {

    while( (psDirent = readdir( pDir )) )
    {

      if( strcmp( pzDir, "/" ) != 0 )
      {

        sprintf( zFile, "%s/%s", pzDir, psDirent->d_name );

      } else {

        sprintf( zFile, "/%s", psDirent->d_name );

      }

      if( stat( zFile, &sStats ) == 0 )
      {

        if( ! S_ISDIR(sStats.st_mode) )
        {

          fprintf( pfData, "F%s\n", psDirent->d_name );
          nFileCount++;

        } else {

          if( ( strcmp( psDirent->d_name, "." ) != 0 ) && ( strcmp( psDirent->d_name, ".." ) != 0 ) )
          {

            if( lstat( zFile, &sStats ) == 0 ) { }

            if( S_ISLNK(sStats.st_mode) == 1 )
            {

              strcpy( zLinkBuf, zFile );
              resolve_link( zLinkBuf );

              fprintf( pfIdx, "L%s>", zFile);
              fprintf( pfData, "L%s>", psDirent->d_name);

              if( strncmp( zLinkBuf, "/", 1 ) == 0 )
              {

                fprintf( pfIdx, "%s\n", zLinkBuf );
                fprintf( pfData, "%s\n", zLinkBuf );

              } else {

                if( strcmp( pzDir, "/" ) == 0 )
                {
      
                  fprintf( pfIdx, "/%s\n", zLinkBuf );
                  fprintf( pfData, "/%s\n", zLinkBuf );

                } else {

                  fprintf( pfIdx, "%s/%s\n", pzDir, zLinkBuf );
                  fprintf( pfData, "%s/%s\n", pzDir, zLinkBuf );

                }

              }

            } else {

              cDirList.push_back( new String( zFile ) );

            }

          }

        }

      } else { 

        printf("Could not stat %s. Treating as file.\n", zFile ); 
        fprintf( pfData, "F%s\n", psDirent->d_name );
        nFileCount++;

      }

    }

    closedir( pDir );

    for( unsigned int n = 0; n < cDirList.size( ); n++ )
      process_dir( cDirList[n]->c_str( ) );


  } else { printf("Could not read directory %s\n", pzDir ); }

}




void resolve_link( char *pzLink )
{

  char zTemp[1024];
  char zCpyTemp[1024];
  char zCpyTemp2[1024];
  int nLen = 0;
  char *pzPathTail = 0;
  struct stat sStats;

  nLen = readlink( pzLink, zTemp, 1024 );
  zTemp[nLen + 1] = 0;

  pzPathTail = strstr( zTemp + 1, "/" );
  if( pzPathTail )
  {

    strncpy( zCpyTemp, zTemp, pzPathTail - zTemp );
    zCpyTemp[ pzPathTail - zTemp ] = 0;
 
    if( strncmp( zCpyTemp, "/", 1 ) != 0 )
    {

      strcpy( zCpyTemp2, "/" );
      strcat( zCpyTemp2, zCpyTemp );
      strcpy( zCpyTemp, zCpyTemp2 );

    }

    if( lstat( zCpyTemp, &sStats ) == 0 ) 
    { 

      if( S_ISLNK(sStats.st_mode) == 1 )
      {

        resolve_link( zCpyTemp );
        strcpy( pzLink, zCpyTemp );
        strcat( pzLink, pzPathTail );
        strcpy( zTemp, pzLink );

      }

    }

  }

  if( strncmp( zTemp, "/", 1 ) != 0 )
  {

    strcpy( zCpyTemp, "/" );
    strcat( zCpyTemp, zTemp );
    strcpy( zTemp, zCpyTemp );

  }

  if( nLen > 0 )
  {

    strcpy( pzLink, zTemp );

    if( lstat( zFile, &sStats ) == 0 ) 
    { 

      if( S_ISLNK(sStats.st_mode) == 1 )
      {

        resolve_link( pzLink );

      }

    }

  }

}
