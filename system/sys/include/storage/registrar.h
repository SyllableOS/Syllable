/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef __F_STORAGE_FILETYPE_H__
#define __F_STORAGE_FILETYPE_H__


#include <gui/image.h>
#include <util/string.h>
#include <util/locker.h>
#include <util/message.h>
#include <util/looper.h>
#include <storage/path.h>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

enum
{
	REGISTRAR_OK,
	REGISTRAR_ERROR,
	REGISTRAR_LOGIN,
	REGISTRAR_LOGOUT,
	REGISTRAR_INVALIDATE,
	REGISTRAR_REGISTER_TYPE,
	REGISTRAR_UNREGISTER_TYPE,
	REGISTRAR_REGISTER_TYPE_ICON,
	REGISTRAR_REGISTER_TYPE_EXTENSION,
	REGISTRAR_CLEAR_TYPE_EXTENSIONS,
	REGISTRAR_REGISTER_TYPE_HANDLER,
	REGISTRAR_CLEAR_TYPE_HANDLERS,
	REGISTRAR_GET_TYPE_COUNT,
	REGISTRAR_GET_TYPE,
	REGISTRAR_SET_DEFAULT_HANDLER,
	REGISTRAR_GET_TYPE_AND_ICON,
	REGISTRAR_REGISTER_CALL,
	REGISTRAR_UNREGISTER_CALL,
	REGISTRAR_QUERY_CALL,
	REGISTRAR_INVOKE_CALL
};

struct RegistrarCall_s
{
	int64 m_nProcess;
	String m_zID;
	int m_nTargetPort;
	int m_nMessageCode;
	String m_zDescription;
};

/** Class to access the registrar server.
 * \ingroup storage
 * \par Description:
 * The RegistrarManager class provides a way to access the registrar, a server
 * which manages the known filetypes. It is especially useful for applications
 * to register the filetypes that they can handle. It can also be used to get 
 * the filetype and icon for files or to launch files with their associated
 * application.
 *
 *
 * \par 
 * An example to register the current application as a handler for jpeg images:
 * \code
 * ...
 * #include <atheos/image.h>
 * #include <storage/registrar.h>
 *
 * MyApp::MyApp() : os::Application( "application/x-vnd-MyApp" )
 * {
 *  try
 *  {
 *   os::RegistrarManager* pcManager = os::RegistrarManager::Get();
 *	 pcManager->RegisterType( "image/x-jpg", "JPEG image" );
 *   pcManager->RegisterAsTypeHandler( "image/x-jpg" );
 *   // Assumes that you added the filetype icon as a resource
 *   pcManager->RegisterTypeIconFromRes( "image/x-jpg", "image_jpg.png" );
 *   pcManager->Put();
 *  }
 * }
 * \endcode
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

class RegistrarManager : public Looper
{
public:
	RegistrarManager();
	~RegistrarManager();
	
	void AddRef();
	
	static RegistrarManager* Get();
	void Put( bool bAll = false );
	
	void HandleMessage( Message* pcMessage );
	
	void SetSynchronousMode( bool bSync );
	
	status_t RegisterType( String zMimeType, String zIdentifier, bool bOverwrite = false );
	status_t UnregisterType( String zMimeType );
	status_t RegisterTypeIcon( String zMimeType, Path cIcon, bool bOverwrite = false );
	status_t RegisterTypeIconFromRes( String zMimeType, String zIconRes, bool bOverwrite = false );
	status_t RegisterTypeExtension( String zMimeType, String zExtension );
	status_t ClearTypeExtensions( String zMimeType );
	status_t RegisterTypeHandler( String zMimeType, Path cHandler );
	status_t RegisterAsTypeHandler( String zMimeType );
	status_t ClearTypeHandlers( String zMimeType );

	int32 GetTypeCount();
	Message GetType( int32 nIndex );
	
	status_t SetDefaultHandler( String zMimeType, Path zHandler );
	
	status_t GetTypeAndIcon( String zFile, Point cIconSize, String* zTypeName, Image** pcIcon, Message* pcMessage = NULL );
	status_t Launch( Window* pcParentWindow, String zFile, bool bVerbose = false, String zTitle = "Select handler", bool bDefaultHandler = true );


	status_t RegisterCall( String zID, String zDescription, os::Looper* pcTarget, int nMessageCode, bool bPersistent = false );
	status_t UnregisterCall( String zID );
	status_t QueryCall( String zID, int nIndex, RegistrarCall_s *psCallInfo );
	status_t InvokeCall( RegistrarCall_s *sCallInfo, Message* pcData, Message* pcReply );
private:
	
	class Private;
	Private* m;
	static RegistrarManager* s_pcInstance;
};

};

#endif







