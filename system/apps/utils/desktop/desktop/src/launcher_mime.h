/*
    launcher_plugin - A plugin interface for the AtheOS Launcher
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)
 
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
 
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
 
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    See ../../include/COPYING
*/

#include <storage/fsnode.h>
#include <storage/directory.h>
#include <storage/file.h>
#include <util/regexp.h>
#include <atheos/time.h>
#include <unistd.h>


using namespace os;

class MimeInfo;
//class MimeNode;

class MimeInfo
{
public:
    MimeInfo( );											// Initialises the class with a type of application/octet-stream
    ~MimeInfo( );
    MimeInfo( const std::string &cMimeType );				// Initialises the class with the given type
    std::string GetMimeType( void );						// Returns the current type
    void SetMimeType( const std::string &cMimeType );		// Sets the mime-type
    std::string GetDefaultCommand( void );					// Returns the default command line of the mime type.
    void SetDefaultCommand( const std::string &cCommand );	// Sets the default command line of the type.
    std::string GetDefaultIcon( void );						// Returns the default icon of the type.
    void SetDefaultIcon( const std::string &cIcon );		// Sets the default icon of the type.
    int FindMimeType( const std::string &cFile );			// Attempts to identify the type of a file. Returns -1 on success.
    void AddNameMatch( const std::string &cRegex );			// Adds a regexp to the list of possible matches.
    void DelNameMatch( const std::string &cRegex );			// Removes a regexp from the list of matches.
    std::vector<std::string> GetNameMatches( void );		// Returns the list of regexp matches.

private:
    std::string m_cMimeTypePath;
    std::string m_cDefaultIconPath;

    std::string m_cMimeType;
    std::string m_cDefaultCommand;
    std::string m_cDefaultIcon;
    std::vector<std::string> m_acMatch;
    bool ReadMimeType( const std::string &cMimeType );
    bool WriteMimeType( bool bFileAndAttrs = true );
    bool MimeTypeLooksValid( const std::string &cMimeType );
    bool FileMatches( const std::string &cFile );
};



class MimeNode : public FSNode
{
public:
    MimeNode( );
    MimeNode( const std::string &cPath, int nOpenMode = O_RDONLY );
    ~MimeNode( );
    int SetTo( const std::string &cPath );
    const std::string GetMimeType( void );				// Returns the mime-type of the node.
    void SetMimeType( const std::string &cMimeType );	// Sets the mime type of the node.
    const std::string GetDefaultCommand( void );		// Returns the default command line for the node.
    const std::string GetDefaultIcon( void );			// Returns the icon of the node.
    MimeInfo *GetMimeInfo( void );						// Returns the MimeInfo object for the node.
    void Launch( void );								// Launches the default command for the node.

private:
    MimeInfo *m_pcMimeInfo;
    std::string m_cPath;
};

