// Keyboard Preferences
// (C)opyright 2007 Flemming H. Sørensen
// (C)opyright 2002 Daryl Dudey
// Based on work by Kurt Skuaen
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>
#include "mainwindow.h"
#include "messages.h"
#include "resources/Keyboard.h"
#include <iostream>


MainWindow::MainWindow() : os::Window( os::Rect( 0, 0, 350, 300 ), "main_wnd", MSG_MAINWND_TITLE )
{
	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );

	/* Stuff */
	os::LayoutView* pcView = new os::LayoutView( GetBounds(), "layout_view" );
	#include "mainwindowLayout.cpp"
	pcView->SetRoot( m_pcRoot );
	AddChild( pcView );

	// Set defaults
	this->SetDefaultButton(m_pcButtonApply);
	m_pcKeyboardLayoutShowAll->SetTarget( this );
	m_pcKeyboardLayoutList->InsertColumn( "", 1000 );

	// Set NodeMonitor
	m_pcMonitor = new NodeMonitor( "/system/keymaps",NWATCH_ALL,this );

	// We want to know the users primary language
	try {
		os::Settings* pcSettings = new os::Settings( new os::File( os::String( getenv( "HOME" ) ) + os::String( "/Settings/System/Locale" ) ) );
		pcSettings->Load();
		cPrimaryLanguage = pcSettings->GetString("LANG","",0);
		delete( pcSettings );
	} catch(...) { }
	if( cPrimaryLanguage == "" ) {
		m_pcKeyboardLayoutShowAll->SetValue( true );
		m_pcKeyboardLayoutFrameLayout->RemoveChild( m_pcKeyboardLayoutShowAll );
	}

	// Get current delay and repeat settings and set slider values
	os::Application::GetInstance()->GetKeyboardConfig( &cKeymap, &iDelay, &iRepeat );
	/* As of Syllable 0.6.6, GetKeyboardConfig() returns the full path to the keymap.
	   We only want the name, so we strip off the path.
	   This might change in future releases.
	*/
	size_t nSlash = cKeymap.str().find_last_of( '/' );
	if( nSlash != std::string::npos ) cKeymap = cKeymap.str().substr( nSlash + 1 );

	iDelay2 = iDelay;
	iRepeat2 = iRepeat;
	iOrigRow = iOrigRow2 = -1;
	m_pcInitialSlider->SetValue( (float)iDelay, true);
	m_pcRepeatSlider->SetValue( (float)iRepeat, true);

	LoadDatabase();
	ShowData();
	SetCurrent();
	iOrigRow2 = iOrigRow;
}



void MainWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_NODE_MONITOR:
		case M_SHOWALL:
		{
			ShowData();
			SetCurrent();
			break;
		}

		case M_BUTTON_APPLY:
		{
			Apply();
			break;
		}

		case M_BUTTON_UNDO:
		{
			Undo();
			break;
		}

		case M_BUTTON_DEFAULT:
		{
			Default();
			break;
		}

		case M_APP_QUIT:
		{
			OkToQuit();
		}
		break;
	}
}

void MainWindow::LoadDatabase()
{
	/* Load database */
	os::File cSelf( open_image_file( get_image_id() ) );
	os::Resources cCol( &cSelf );		
	os::ResStream *pcDatabase = cCol.GetResourceStream( "keymaps.db" );
	if( pcDatabase == NULL )
		return;
	uint n, lp = 0;
	char chr, linebfr[ 512 ];
	do {
		n = pcDatabase->Read( &chr, sizeof( char ) );
		if( n != 1 || chr == '\n' ) {
			linebfr[ lp ] = 0;
			lp = 0;
			LanguageInfo li( linebfr );
			if( li.IsValid() ) {
				m_cDatabase.push_back( li );
			}
								
		} else {
			linebfr[ lp++ ] = chr;
			if( lp > sizeof( linebfr ) - 1 ) {
				lp = sizeof( linebfr ) - 1;
			}
		}
	} while( n > 0 );
	delete( pcDatabase );
}

void MainWindow::ShowData()
{
	bool bShowAll = m_pcKeyboardLayoutShowAll->GetValue();

	/* TODO: Should use os::Keymap methods here whenever possible */

	// Open up keymaps directory and check it actually contains something
	DIR *pDir = opendir("/system/keymaps");
	if (pDir == NULL) {
		return;
	}

	// Clear listview
	m_pcKeyboardLayoutList->Clear();

	// Create a directory entry object
	dirent *psEntry;

	// Loop through directory and add each keymap to list
	while ( (psEntry = readdir( pDir )) != NULL) {

		// If special directory (i.e. dot(.) and dotdot(..) then ignore
		if (strcmp( psEntry->d_name, ".") == 0 || strcmp( psEntry->d_name, ".." ) == 0) {
			continue;
		}

		// If it's a valid file, open it
		FILE *hFile = fopen( (std::string("/system/keymaps/")+psEntry->d_name).c_str(), "r" );
		if (hFile == NULL) {
			continue;
		}

		// Check the first few numbers of the file (the magic number) match that for a keymap
		uint iMagic = 0;
		fread( &iMagic, sizeof(iMagic), 1, hFile);
		fclose( hFile);
		if (iMagic != KEYMAP_MAGIC ) {
			continue;
		}

		// Its valid, add to the listview
		bool bAdded = false;
		os::String cTempKeymap;
		
		for( std::vector<LanguageInfo>::const_iterator i = m_cDatabase.begin(); i != m_cDatabase.end() && !bAdded; i++ )
		{
			LanguageInfo li = (*i);
			if( li.IsValid() &&  bShowAll == false && strcmp( psEntry->d_name, li.GetName().c_str() ) == 0 ) {
				/* If it is the active keymap, we should show it even if it doesn't match the active language */
				if( strcmp( psEntry->d_name, cKeymap.c_str() ) == 0 ) {
					os::ListViewStringRow* pcRow = new os::ListViewStringRow();
					pcRow->AppendString( li.GetAlias() );
					m_pcKeyboardLayoutList->InsertRow( pcRow, false );
					bAdded = true;
					break;
				}
				
				/* Check if this keymap is relevant to the current language */
				char delims[] = ",";
				char *result = NULL;
				char zGetCode[128];
				sprintf(zGetCode,"%s",li.GetCode().c_str());
				result = strtok( zGetCode, delims );
				while( result != NULL && bAdded == false ) {
					if( cPrimaryLanguage == result ) {
						os::ListViewStringRow* pcRow = new os::ListViewStringRow();
						pcRow->AppendString( li.GetAlias() );
						m_pcKeyboardLayoutList->InsertRow( pcRow, false );
						bAdded = true;
					}
					result = strtok( NULL, delims );
				}
			} else if( bAdded == false && bShowAll == true && strcmp( psEntry->d_name, li.GetName().c_str() ) == 0 ) {
				os::ListViewStringRow* pcRow = new os::ListViewStringRow();
				pcRow->AppendString( li.GetAlias() );
				m_pcKeyboardLayoutList->InsertRow( pcRow, false );
				bAdded = true;
			}
		}
		// It wasn't on the list, but we don't give up. We check for an attribute than can help us
		if( bAdded == false ) {
			char delims[] = ",";
			char *result = NULL;
			os::FSNode cFileNode;
			cFileNode.SetTo( (std::string("/system/keymaps/")+psEntry->d_name).c_str() );
			char zBuffer[PATH_MAX];
			memset( zBuffer, 0, PATH_MAX );
			cFileNode.ReadAttr( "Keymap::Language", ATTR_TYPE_STRING, zBuffer, 0, PATH_MAX );
			result = strtok( zBuffer, delims );
			while( result != NULL && bAdded == false ) {
				if( bShowAll == true || cPrimaryLanguage == result ) {
					os::ListViewStringRow* pcRow = new os::ListViewStringRow();
					pcRow->AppendString( psEntry->d_name );
					m_pcKeyboardLayoutList->InsertRow( pcRow, false );
					bAdded = true;
				}
				result = strtok( NULL, delims );
			}
		}

		// If we don't know the keymap, so we just add it with it's filename.
		if( bAdded == false  && (bShowAll == true || strcmp( psEntry->d_name, cKeymap.c_str() ) == 0) ) {
			os::ListViewStringRow* pcRow = new os::ListViewStringRow();
			pcRow->AppendString( psEntry->d_name );
			m_pcKeyboardLayoutList->InsertRow( pcRow, false );
			bAdded = true;
		}

	}
	closedir( pDir );
	m_pcKeyboardLayoutList->Invalidate( true );
	m_pcKeyboardLayoutList->Flush();
}


void MainWindow::SetCurrent()
{
	for( uint16 i = 0 ; i < m_pcKeyboardLayoutList->GetRowCount() ; i++ ) {
		// We know the alias, but we want the real name.
		os::ListViewStringRow* pcRow = static_cast<os::ListViewStringRow*>( m_pcKeyboardLayoutList->GetRow( i ) );
		os::String cKeymapName = pcRow->GetString(0);
		for( std::vector<LanguageInfo>::const_iterator j = m_cDatabase.begin(); j != m_cDatabase.end(); j++ )
		{
			LanguageInfo li = (*j);
			if( li.IsValid() && pcRow->GetString(0) == li.GetAlias() ) {
				cKeymapName = li.GetName();
				break;
			}
		}

		// ...and set select the current keymap.
		if( cKeymap == cKeymapName ) {
			m_pcKeyboardLayoutList->Select( i );
			iOrigRow = i;
			break;
		}
	}
}


void MainWindow::Apply()
{
	os::ListViewStringRow *pcRow;
	os::String cKeymapName;
	
	// Apply the keymap. We know the alias, but we want the real name
	int nLastSelected = m_pcKeyboardLayoutList->GetLastSelected();
	if( nLastSelected != -1 )	/* -1 indicates no row is selected */
	{
		pcRow = static_cast<os::ListViewStringRow*>( m_pcKeyboardLayoutList->GetRow( nLastSelected ) );
		cKeymapName = pcRow->GetString(0);
		for( std::vector<LanguageInfo>::const_iterator i = m_cDatabase.begin(); i != m_cDatabase.end(); i++ )
		{
			LanguageInfo li = (*i);
			if( li.IsValid() && pcRow->GetString(0) == li.GetAlias() ) {
				cKeymapName = li.GetName();
				break;
			}
		}
	}
	
	// Apply delay and repeat
	os::Application::GetInstance()->SetKeyboardTimings( int(m_pcInitialSlider->GetValue()), int(m_pcRepeatSlider->GetValue()) );

	if( nLastSelected != -1 )
	{
		//Apply keymap
		cKeymap = cKeymapName;
		os::Application::GetInstance()->SetKeymap( cKeymapName.c_str() );
	}

	// Save settings for undo
	iDelay2 = iDelay;
	iRepeat2 = iRepeat;
	iOrigRow2 = iOrigRow;
	iDelay = m_pcInitialSlider->GetValue();
	iRepeat = m_pcRepeatSlider->GetValue();
	iOrigRow = m_pcKeyboardLayoutList->GetFirstSelected();
}


void MainWindow::Undo()
{
	// Revert to loaded settings
	m_pcInitialSlider->SetValue( (float)iDelay2 );
	m_pcRepeatSlider->SetValue( (float)iRepeat2 );
	if( iOrigRow2 >= 0 ) m_pcKeyboardLayoutList->Select(iOrigRow2);
}


void MainWindow::Default()
{
	// Set to defaults 300/40
	m_pcInitialSlider->SetValue( (float)300.0f );
	m_pcRepeatSlider->SetValue( (float)40.0f );
}


bool MainWindow::OkToQuit()
{
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return( true );
}

