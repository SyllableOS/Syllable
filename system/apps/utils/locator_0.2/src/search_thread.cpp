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

/***********************************************
**
** Title:  search_thread.cpp
**
** Author: Andrew Kennan
**
** Date:   25/10/2001
**
**
***********************************************/ 

#include "locator.h"



/***********************************************
**
**
**
***********************************************/ 
SearchThread::SearchThread( Looper *pcParent ) : Looper( "locator_search" )
{

  Log( "Creating search thread." );
  char zLine[1024];
  FILE *phIndex;

  m_pcParent = pcParent;

  bFollowSubDirs = true;
  bMatchDirNames = true;

  string zIndexFile = app_path() + (string)INDEX_FILE;

  // Read the directory index
  if( ( phIndex = fopen( zIndexFile.c_str(), "r" ) ) )
  {

    Log( "Reading index file." );

    while( fscanf( phIndex, "%s", zLine ) != EOF )
    {

      cDirList.push_back( (string)zLine );
    
    }

    Log( "Finished reading index file." );

    fclose( phIndex );

    this->Run( );

  } else {

    SendMessage( ERR_INDEX_FILE );
    Quit( );

  }

}



/***********************************************
**
**
**
***********************************************/ 
SearchThread::~SearchThread( )
{

  Log( "Exiting." );

}


/***********************************************
**
**
**
***********************************************/ 
void SearchThread::HandleMessage( Message *pcMessage )
{

  int nId = pcMessage->GetCode( );

  switch( nId )
  {

    case EV_START_SEARCH:
      Lock( );
      Log( "Starting Search." );
      bFoundDir = false;
      pcMessage->FindBool( "FollowSubDirs", &bFollowSubDirs );
      pcMessage->FindBool( "MatchDirNames", &bMatchDirNames );
      pcMessage->FindBool( "UseRegex", &bUseRegex );
      pcMessage->FindString( "search", &zSearch );

      if( (bUseRegex == true) && (regcomp( &sRX, zSearch.c_str( ), REG_EXTENDED | REG_NOSUB ) != 0 ) )
      {
        SendMessage( ERR_REGEX );
      } else {
        SendMessage( FindMatch( pcMessage ) );
      }
      Log( "Finished Search." );
      Unlock( );
      break;

  }

}



/***********************************************
**
** 
**
***********************************************/ 
int SearchThread::FindMatch( Message *pcMessage )
{

  bool bDontMatchFileNames = false;
  bool bDontMatchDirNames = false;

  FILE *pfData;

  string zSearchDir;
  string zAlias;
  string zTemp;

  pcMessage->FindString( "dir", &zSearchDir );

  // Strip extra slashes from the end of the string.
  if( zSearchDir != "/" )
    while( (zSearchDir.substr(zSearchDir.length( ) -1, 1) == "/") && (zSearchDir.length( ) > 1) )
      zSearchDir.erase( zSearchDir.length( ) -1, 1 );

  zAlias = zSearchDir;

  Log( "Searching Dir " + zSearchDir );

  // Attempt to expand symlinks to destination directory names
  if( zSearchDir != "/" )
  {

    Log( "Expanding Symlinks." );
    unsigned int nLoc = zSearchDir.find( "/", 1 );

    if( nLoc == string::npos )
      nLoc = zSearchDir.length( );

    while( nLoc != string::npos )
    {
      
      zTemp = ResolveLinks( zAlias.substr( 0, nLoc ) );
      zAlias = zTemp + zSearchDir.substr( nLoc, zSearchDir.length( ) - nLoc );
      Log( "zAlias set to " + zAlias );

      nLoc = zSearchDir.find( "/", zTemp.length( ) + 1 );

    }

    Log( "Finished expanding symlinks." );

  }


  string zDataFile = app_path() + (string)DATA_FILE;
  // Process each line of the dir index
  if( (pfData = fopen( zDataFile.c_str(), "r" )) )
  {

    Log( "Opened the data file" );

    bool bExit = false;
    unsigned int n = 0;


    Log( "Processing Directory List." );

    while( ( n < cDirList.size( ) ) && ( bExit == false ))
    {

      
      if( cDirList[n].substr(0,1) != "L" )
      {

        unsigned int nLoc = cDirList[n].find( ",", 0 );

        if( nLoc != string::npos )
        {

          if( zAlias == cDirList[n].substr( nLoc + 1, cDirList[n].length( ) - nLoc - 1 ) )
          {

            bFoundDir = true;    
            bExit = true;
            fseek( pfData, atol( cDirList[n].c_str( ) ), SEEK_SET );
 
          }

        }

      }

      n++;

    }

    if( bFoundDir == false )
    {

      fclose( pfData );
      return ERR_DIR_NOT_FOUND;

    }

    string zCurDir = zAlias; 
    string zLine;
    bool bEndOfDir = false;
 
    // Process each line of the data file
    char zTemp[1024];
    while( (fscanf( pfData, "%s", zTemp ) != EOF) && ( bEndOfDir == false ) )
    {

      zLine = (string)zTemp;

//      Log( "Line: " + zLine );

      if( zLine.substr(0,1) == "F" )
      {
        if( bDontMatchFileNames == false )
        {
//          Log( "Comparing " + zLine.substr( 1, zLine.length( ) - 1 ) );
          if( Match( zLine.substr( 1, zLine.length( ) - 1 ) ) == true )
            SendFoundItem( L_FILE, zLine.substr( 1, zLine.length( ) - 1 ), zCurDir );
        }

      } else {
         
        if( zLine.substr(0,1) == "D" )
        {

          if( bFollowSubDirs == false )
          {

            bDontMatchDirNames = false;
            bDontMatchFileNames = false;
      
            int nSubDirCount = 0;
            unsigned int nLoc = zLine.find( "/", zAlias.length( ) );
            if( nLoc != string::npos )
            {

              bDontMatchFileNames = true;
              nSubDirCount = 1;
              if( zLine.find( "/", zAlias.length( ) + nLoc ) != string::npos )
                nSubDirCount++;

            }

            if( nSubDirCount > 1 )
              bDontMatchDirNames = true;

          }
              
           
          if( zLine.substr( 1, zAlias.length( ) ) == zAlias )
          {

            Log( "Directory: " + zLine.substr(1, zLine.length( ) - 1 ) );

            if( ( Match( zLine.substr( 1 + zAlias.length( ), zLine.length( ) - zAlias.length( ) + 1 ) ) == true ) &&  bMatchDirNames && ( bDontMatchDirNames == false ))
              SendFoundItem( L_DIR, zLine.substr( 1 + zAlias.length( ), zLine.length( ) - zAlias.length( ) + 1 ), zSearchDir );

            zCurDir = zSearchDir + zLine.substr( 1 + zAlias.length( ), zLine.length( ) - zAlias.length( ) + 1 );
  
            Log( "Set zCurDir to " + zCurDir );
                   
          } else {

            Log( "End of directory." );
            bEndOfDir = true;

          }

        }

      }

    }

    Log( "Finishing..." );
 
    fclose( pfData );

  } else return ERR_DATA_FILE;

  return EV_SEARCH_FINISHED;

}


/***********************************************
**
**
**
***********************************************/ 
string SearchThread::ResolveLinks( string zSearchDir )
{

  zSearchDir += ">";

  unsigned int n = 0;

  Log( "Resolving symlinks in " + zSearchDir );

  while( n < cDirList.size( ) )
  {

    if( cDirList[n].substr(0,1) == "L" )
    {

      Log( cDirList[n].substr(1, zSearchDir.length( ) - 1) + "Is A Link" );

      if( cDirList[n].substr(1, zSearchDir.length( )) == zSearchDir )
      {

        return cDirList[n].substr( cDirList[n].find( ">", 0 ) + 1, cDirList[n].length( ) - cDirList[n].find( ">", 0 ) - 1 );
  
      }

    }

    n++;

  }

}




/***********************************************
**
**
**
***********************************************/         
void SearchThread::SendFoundItem( int nType, string zFileName, string zDirName )
{

  Log( "Sending Found Item Message..." );

  Message *pcFoundMessage = new Message( EV_ITEM_FOUND );
  pcFoundMessage->AddInt32( "type", nType );
  pcFoundMessage->AddString( "name", zFileName );
  pcFoundMessage->AddString( "dir", zDirName );
  Invoker *pcFoundInvoker = new Invoker( pcFoundMessage, m_pcParent );
  pcFoundInvoker->Invoke( );

  Log( "Sent." );

}



/***********************************************
**
**
**
***********************************************/ 
void SearchThread::SendMessage( int nId )
{

  Log( "Sending Message" );

  Invoker *pcFailInvoker = new Invoker( new Message( nId ), m_pcParent );
  pcFailInvoker->Invoke( );

  Log( "Sent." );

}



/***********************************************
**
**
**
***********************************************/ 
bool SearchThread::Match( string zString )
{

  if( bUseRegex == true )
  {

    if( regexec( &sRX, zString.c_str( ),0,NULL,0 ) == 0 )
      return true;

  } else {

    if( zString.find( zSearch, 0 ) != string::npos )
      return true;
  }

  return false;

}



/***********************************************
**
**
**
***********************************************/ 
void SearchThread::Log( string zMessage )
{

  if( DEBUG_MODE == 1 )
    cout << "SearchThread: " << zMessage << endl;

}
