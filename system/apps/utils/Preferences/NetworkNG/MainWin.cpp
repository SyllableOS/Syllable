// Syllable Network Preferences - Copyright 2006 Andrew Kennan
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

#include "MainWin.h"
#include "ListViewEditRow.h"
#include "IFaceWin.h"
#include "Event.h"
#include "Strings.h"

#include <unistd.h>

#include <util/application.h>
#include <util/resources.h>

#include <gui/layoutview.h>
#include <gui/tabview.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/frameview.h>
#include <gui/image.h>

#define ICON_NAME "icon24x24.png"

NetworkPrefs::NetworkPrefs()
{
	m_bIsRoot = getuid() == 0;
	LoadInterfaces();
}

NetworkPrefs::~NetworkPrefs()
{
	InterfaceManager im;

	im.ClearInterfaceList( m_cInterfaces );
}

void NetworkPrefs::LoadInterfaces( void )
{
	InterfaceManager im;

	im.LoadConfig( m_cInterfaces );
	if( m_cInterfaces.size() == 0 )
	{
		im.DiscoverInterfaces( m_cInterfaces );
	}
	else
	{
		InterfaceList_t cDetect;

		im.DiscoverInterfaces( cDetect );
		if( im.InterfacesHaveChanged( m_cInterfaces, cDetect ) )
		{
			im.DiscoverInterfaces( m_cInterfaces );
		}
		im.ClearInterfaceList( cDetect );
	}
}

void NetworkPrefs::ApplyChanges( void )
{
	InterfaceManager im;

	im.SaveConfig( m_cInterfaces );
	m_cHostname.Save();
	m_cNameserver.Save();
	m_cHosts.Save();

	im.WriteNetworkInit( m_cInterfaces );
	im.InvokeNetworkInit();
}

void NetworkPrefs::RevertChanges( void )
{
	LoadInterfaces();
	m_cHostname.Load();
	m_cNameserver.Load();
	m_cHosts.Load();
}

Hostname & NetworkPrefs::GetHostname( void )
{
	return m_cHostname;
}

Nameserver & NetworkPrefs::GetNameserver( void )
{
	return m_cNameserver;
}

Hosts & NetworkPrefs::GetHosts( void )
{
	return m_cHosts;
}

const InterfaceList_t & NetworkPrefs::GetInterfaces( void )
{
	return m_cInterfaces;
}

/////////////////////////////////////

// Set a window icon from a resource
void set_icon( Window * pcWindow )
{
	Resources cCol( get_image_id() );
	ResStream *pcStream = cCol.GetResourceStream( ICON_NAME );
	BitmapImage *pcIcon = new os::BitmapImage( pcStream );

	pcWindow->SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
}

/////////////////////////////////////

MainWin::MainWin():Window( Rect( 0, 0, 300, 300 ), "NetworkPrefs", MSG_MAINWIN_TITLE )
{
	// Top level layout view.
	VLayoutNode *pcRoot = new VLayoutNode( "pcRoot" );
	LayoutView *pcLayout = new LayoutView( Rect( 0, 0, 300, 300 ), "pcLayout", pcRoot );

	// Main tab view
	TabView *pcTabs = new TabView( Rect(), "pcTabs" );

	// "Interfaces" tab.
	VLayoutNode *pcIRoot = new VLayoutNode( "pcIRoot" );
	LayoutView *pcIFaceTab = new LayoutView( Rect(), "pcIFaceTab", pcIRoot );

	// Interface list
	m_pcLstIFace = new ListView( Rect(), "m_pcLstIFace", ListView::F_RENDER_BORDER );
	m_pcLstIFace->InsertColumn( MSG_MAINWIN_IFACE_COL_NAME.c_str(), 150 );
	m_pcLstIFace->InsertColumn( MSG_MAINWIN_IFACE_COL_ENABLED.c_str(), 150 );
	pcIRoot->AddChild( m_pcLstIFace );
	pcTabs->AppendTab( MSG_MAINWIN_IFACE_TITLE, pcIFaceTab );

	HLayoutNode *pcIBtnRow = new HLayoutNode( "pcIBtnRow" );
	Button *pcBtnIFProp = new Button( Rect(), "pcBtnIFProp", MSG_MAINWIN_IFACE_PROPERTIES, new Message( InterfaceProperties ) );

	pcIBtnRow->AddChild( pcBtnIFProp );
	pcIBtnRow->LimitMaxSize( Point( INT_MAX, pcBtnIFProp->GetPreferredSize( false ).y ) );
	pcIRoot->AddChild( pcIBtnRow );

	// Default interface row
	HLayoutNode *pcDefaultIFRow = new HLayoutNode( "pcDefaultIFRow" );
	StringView *pcDefaultIFLabel = new StringView( Rect(), "pcDefaultIFLabel", MSG_MAINWIN_IFACE_DEFAULT_GW );

	m_pcDdmDefaultIFace = new DropdownMenu( Rect(), "m_pcDdmDefaultIFace" );
	m_pcDdmDefaultIFace->SetReadOnly( true );
	pcDefaultIFRow->AddChild( pcDefaultIFLabel );
	pcDefaultIFRow->AddChild( m_pcDdmDefaultIFace );
	pcDefaultIFRow->LimitMaxSize( Point( INT_MAX, m_pcDdmDefaultIFace->GetPreferredSize( false ).y ) );
	pcIRoot->AddChild( pcDefaultIFRow );
	pcIRoot->SetBorders( Rect( 2, 2, 2, 2 ), "m_pcLstIFace", "pcBtnIFProp", "pcDefaultIFLabel", "m_pcDdmDefaultIFace", NULL );

	// "General" tab
	VLayoutNode *pcGRoot = new VLayoutNode( "pcGRoot" );
	LayoutView *pcGeneralTab = new LayoutView( Rect(), "pcGeneralTab", pcGRoot );
	HLayoutNode *pcHostnameRow = new HLayoutNode( "pcHostnameRow" );
	StringView *pcHostnameLabel = new StringView( Rect(), "pcHostnameLabel", MSG_MAINWIN_GENERAL_HOSTNAME );

	// Hostname
	m_pcHostname = new TextView( Rect(), "m_pcHostname", "" );
	pcHostnameRow->LimitMaxSize( Point( INT_MAX, m_pcHostname->GetPreferredSize( false ).y ) );
	pcHostnameRow->AddChild( pcHostnameLabel );
	pcHostnameRow->AddChild( m_pcHostname );
	pcGRoot->AddChild( pcHostnameRow );
	HLayoutNode *pcDomainNameRow = new HLayoutNode( "pcDomainNameRow" );
	StringView *pcDomainNameLabel = new StringView( Rect(), "pcDomainNameLabel", MSG_MAINWIN_GENERAL_DOMAIN );

	// Domain
	m_pcDomainName = new TextView( Rect(), "m_pcDomainName", "" );
	pcDomainNameRow->AddChild( pcDomainNameLabel );
	pcDomainNameRow->AddChild( m_pcDomainName );
	pcDomainNameRow->LimitMaxSize( Point( INT_MAX, m_pcDomainName->GetPreferredSize( false ).y ) );
	pcGRoot->AddChild( pcDomainNameRow );
	pcGRoot->AddChild( new VLayoutSpacer( "spacer_1" ) );
	pcGRoot->SameWidth( "pcHostnameLabel", "pcDomainNameLabel", NULL );
	pcGRoot->SetBorders( Rect( 2, 2, 2, 2 ), "pcHostnameLabel", "m_pcHostname", "pcDomainNameLabel", "m_pcDomainName" );
	pcTabs->AppendTab( MSG_MAINWIN_GENERAL_TITLE, pcGeneralTab );

	// "DNS" tab
	VLayoutNode *pcDnsRoot = new VLayoutNode( "pcDnsRoot" );
	LayoutView *pcDnsTab = new LayoutView( Rect(), "pcDnsTab", pcDnsRoot );
	VLayoutNode *pcNsRoot = new VLayoutNode( "pcNsRoot" );
	FrameView *pcNameservers = new FrameView( Rect(), "pcNameservers", MSG_MAINWIN_DNS_NS_FRAME );

	// Nameservers
	pcNameservers->SetRoot( pcNsRoot );
	m_pcLstNs = new ListView( Rect(), "m_pcLstNs", ListView::F_RENDER_BORDER );
	m_pcLstNs->InsertColumn( MSG_MAINWIN_DNS_NS_COL.c_str(), 280 );
	pcNsRoot->AddChild( m_pcLstNs );
	pcDnsRoot->AddChild( pcNameservers );
	HLayoutNode *pcDnsBtnRow = new HLayoutNode( "pcDnsBtnRow" );
	Button *pcBtnDnsAdd = new Button( Rect(), "pcBtnDnsAdd", MSG_MAINWIN_DNS_NS_ADD, new Message( AddNameserver ) );
	Button *pcBtnDnsDel = new Button( Rect(), "pcBtnDnsDel", MSG_MAINWIN_DNS_NS_DELETE, new Message( DeleteNameserver ) );

	pcDnsBtnRow->AddChild( new HLayoutSpacer( "spacer_2" ) );
	pcDnsBtnRow->AddChild( pcBtnDnsAdd );
	pcDnsBtnRow->AddChild( pcBtnDnsDel );
	pcDnsBtnRow->LimitMaxSize( Point( INT_MAX, pcBtnDnsDel->GetPreferredSize( false ).y ) );
	pcNsRoot->AddChild( pcDnsBtnRow );
	pcNsRoot->SetBorders( Rect( 2, 2, 2, 2 ), "m_pcLstNs", "pcBtnDnsAdd", "pcBtnDnsDel", NULL );

	// Search domains
	VLayoutNode *pcSrcRoot = new VLayoutNode( "pcSrcRoot" );
	FrameView *pcDomains = new FrameView( Rect(), "pcDomains", MSG_MAINWIN_DNS_SD_FRAME );

	pcDomains->SetRoot( pcSrcRoot );
	m_pcLstSrc = new ListView( Rect(), "m_pcLstSrc", ListView::F_RENDER_BORDER );
	m_pcLstSrc->InsertColumn( MSG_MAINWIN_DNS_SD_COL.c_str(), 280 );
	pcSrcRoot->AddChild( m_pcLstSrc );
	pcDnsRoot->AddChild( pcDomains );
	HLayoutNode *pcSrcBtnRow = new HLayoutNode( "pcSrcBtnRow" );
	Button *pcBtnSrcAdd = new Button( Rect(), "pcBtnSrcAdd", MSG_MAINWIN_DNS_SD_ADD, new Message( AddSearchDomain ) );
	Button *pcBtnSrcDel = new Button( Rect(), "pcBtnSrcDel", MSG_MAINWIN_DNS_SD_DELETE, new Message( DeleteSearchDomain ) );

	pcSrcBtnRow->AddChild( new HLayoutSpacer( "spacer_3" ) );
	pcSrcBtnRow->AddChild( pcBtnSrcAdd );
	pcSrcBtnRow->AddChild( pcBtnSrcDel );
	pcSrcBtnRow->LimitMaxSize( Point( INT_MAX, pcBtnSrcDel->GetPreferredSize( false ).y ) );
	pcSrcRoot->AddChild( pcSrcBtnRow );
	pcSrcRoot->SetBorders( Rect( 2, 2, 2, 2 ), "m_pcLstSrc", "pcBtnSrcAdd", "pcBtnSrcDel", NULL );

	pcDnsRoot->SetBorders( Rect( 2, 2, 2, 2 ), "pcNameservers", "pcDomains", NULL );

	pcTabs->AppendTab( MSG_MAINWIN_DNS_TITLE, pcDnsTab );

	// "Hosts" tab
	VLayoutNode *pcHostsRoot = new VLayoutNode( "pcHostsRoot" );
	LayoutView *pcHostsTab = new LayoutView( Rect(), "pcHostsTab", pcHostsRoot );

	m_pcLstHost = new ListView( Rect(), "m_pcLstHost", ListView::F_RENDER_BORDER );
	m_pcLstHost->InsertColumn( MSG_MAINWIN_HOSTS_COL_ADDRESS.c_str(), 100 );
	m_pcLstHost->InsertColumn( MSG_MAINWIN_HOSTS_COL_ALIASES.c_str(), 200 );
	pcHostsRoot->AddChild( m_pcLstHost );

	HLayoutNode *pcHostsBtnRow = new HLayoutNode( "pcHostsBtnRow" );
	Button *pcBtnHostsAdd = new Button( Rect(), "pcBtnHostsAdd", MSG_MAINWIN_HOSTS_ADD, new Message( AddHost ) );
	Button *pcBtnHostsDel = new Button( Rect(), "pcBtnHostsDel", MSG_MAINWIN_HOSTS_DELETE, new Message( DeleteHost ) );

	pcHostsBtnRow->AddChild( new HLayoutSpacer( "spacer_4" ) );
	pcHostsBtnRow->AddChild( pcBtnHostsAdd );
	pcHostsBtnRow->AddChild( pcBtnHostsDel );
	pcHostsBtnRow->LimitMaxSize( Point( INT_MAX, pcBtnHostsDel->GetPreferredSize( false ).y ) );
	pcHostsRoot->AddChild( pcHostsBtnRow );
	pcHostsRoot->SetBorders( Rect( 2, 2, 2, 2 ), "m_pcLstHost", "pcBtnHostsDel", "pcBtnHostsAdd", "pcHostsBtnRow", NULL );
	pcTabs->AppendTab( MSG_MAINWIN_HOSTS_TITLE, pcHostsTab );

	pcRoot->AddChild( pcTabs, 4.0f );

	// Main button row.
	HLayoutNode *pcBtnNode = new HLayoutNode( "pcBtnNode", 0.5f );
	Button *pcBtnApply = new Button( Rect(), "pcBtnApply", MSG_MAINWIN_APPLY, new Message( ApplyChanges ) );
	Button *pcBtnRevert = new Button( Rect(), "pcBtnRevert", MSG_MAINWIN_REVERT, new Message( RevertChanges ) );
	Button *pcBtnClose = new Button( Rect(), "pcBtnClose", MSG_MAINWIN_CLOSE, new Message( Close ) );

	pcBtnNode->AddChild( new HLayoutSpacer( "spacer_0", 2.0f ) );
	pcBtnNode->AddChild( pcBtnApply, 0.5f );
	pcBtnNode->AddChild( pcBtnRevert, 0.5f );
	pcBtnNode->AddChild( pcBtnClose, 0.5f );
	pcBtnNode->SetBorders( Rect( 2, 2, 2, 2 ), "pcBtnApply", "pcBtnRevert", "pcBtnClose", NULL );
	pcBtnNode->LimitMaxSize( Point( INT_MAX, pcBtnApply->GetPreferredSize( false ).y ) );

	pcRoot->AddChild( pcBtnNode );
	pcRoot->SetBorders( Rect( 2, 2, 2, 2 ), "pcTabs", "pcBtnNode", NULL );
	AddChild( pcLayout );

	// Set up tab orders
	View *apcViews[] = {
		pcTabs,
		m_pcLstIFace,
		pcBtnIFProp,
		m_pcDdmDefaultIFace,
		pcBtnApply,
		pcBtnRevert,
		pcBtnClose,
		m_pcHostname,
		m_pcDomainName,
		m_pcLstNs,
		pcBtnDnsAdd,
		pcBtnDnsDel,
		m_pcLstSrc,
		pcBtnSrcAdd,
		pcBtnSrcDel,
		m_pcLstHost,
		pcBtnHostsAdd,
		pcBtnHostsDel
	};
	int nViewCount = 18;
	for( int i = 0; i < nViewCount; i++ ) {
		apcViews[i]->SetTabOrder(i);
	}
	m_pcLstIFace->MakeFocus();
	
	set_icon( this );

	if( !m_cPrefs.IsRoot() )
	{
		// If the current user is not root, 
		// disable controls that can modify the settings.
		Control *apcDisabled[] = {
			pcBtnDnsAdd,
			pcBtnDnsDel,
			pcBtnSrcAdd,
			pcBtnSrcDel,
			pcBtnHostsAdd,
			pcBtnHostsDel,
			pcBtnApply,
			pcBtnRevert,
			m_pcHostname,
			m_pcDomainName
		};
		int nDisabledCount = 10;
		for( int i = 0; i < nDisabledCount; i++ ) {
			apcDisabled[i]->SetEnable(false);
		}
		// Why does DropDownMenu inherit from View instead of Control?
		m_pcDdmDefaultIFace->SetEnable(false);
	}

	Load();
}

MainWin::~MainWin()
{
}

void MainWin::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case ApplyChanges:
		Save();
		break;

	case RevertChanges:
		m_cPrefs.RevertChanges();
		Load();
		break;

	case AddNameserver:
		{
			ListViewEditRow *pcRow = new ListViewEditRow();

			pcRow->AppendString( MSG_MAINWIN_NEW_NAMESERVER );
			m_pcLstNs->InsertRow( m_pcLstNs->GetRowCount(), pcRow );
			m_pcLstNs->Select( m_pcLstNs->GetRowCount() - 1 );
			break;
		}

	case DeleteNameserver:
		{
			int n = m_pcLstNs->GetFirstSelected();

			if( n >= 0 )
				delete( m_pcLstNs->RemoveRow( n ) );
			break;
		}

	case AddSearchDomain:
		{
			ListViewEditRow *pcRow = new ListViewEditRow();

			pcRow->AppendString( MSG_MAINWIN_NEW_SEARCH_DOMAIN );
			m_pcLstSrc->InsertRow( m_pcLstSrc->GetRowCount(), pcRow );
			m_pcLstSrc->Select( m_pcLstSrc->GetRowCount() - 1 );
			break;
		}

	case DeleteSearchDomain:
		{
			int n = m_pcLstSrc->GetFirstSelected();

			if( n >= 0 )
				delete( m_pcLstSrc->RemoveRow( n ) );
			break;
		}

	case AddHost:
		{
			ListViewEditRow *pcRow = new ListViewEditRow();

			pcRow->AppendString( MSG_MAINWIN_NEW_HOST_ADDRESS );
			pcRow->AppendString( MSG_MAINWIN_NEW_HOST_ALIASES );
			m_pcLstHost->InsertRow( m_pcLstHost->GetRowCount(), pcRow );
			m_pcLstHost->Select( m_pcLstHost->GetRowCount() - 1 );
			break;
		}

	case DeleteHost:
		{
			int n = m_pcLstHost->GetFirstSelected();

			if( n >= 0 )
				delete( m_pcLstHost->RemoveRow( n ) );
			break;
		}

	case Close:
		Application::GetInstance()->PostMessage( M_QUIT );
		break;

	case InterfaceProperties:
		{
			int n = m_pcLstIFace->GetFirstSelected();

			if( n >= 0 )
			{
				Interface *pcIFace = m_cPrefs.GetInterfaces()[n];
				IFaceWinMap_t::iterator i = m_cWindows.find( pcIFace->GetName() );

				if( i != m_cWindows.end() )
				{
					( *i ).second->MakeFocus();
				}
				else
				{
					IFaceWin *pcWin = new IFaceWin( this, pcIFace, !m_cPrefs.IsRoot() );

					set_icon( pcWin );
					pcWin->CenterInScreen();
					pcWin->Show();
					pcWin->MakeFocus();
					m_cWindows[pcIFace->GetName()] = pcWin;
				}
			}
			break;
		}

	case ApplyInterfaceChanges:
		{
			LoadInterfaces();
			break;
		}

	case CloseInterfaceWindow:
		{
			String cName;

			pcMessage->FindString( INTERFACE_NAME_KEY, &cName );
			IFaceWinMap_t::iterator i = m_cWindows.find( cName );

			if( i != m_cWindows.end() )
			{
				( *i ).second->Quit();
				m_cWindows.erase( i );
			}
			break;
		}

	default:
		break;
	}
}

bool MainWin::OkToQuit( void )
{
	Application::GetInstance()->PostMessage( M_QUIT );
	return true;
}

void MainWin::LoadInterfaces( void )
{
	m_pcLstIFace->Clear();
	m_pcDdmDefaultIFace->Clear();
	InterfaceList_t cInterfaces = m_cPrefs.GetInterfaces();

	for( InterfaceList_t::iterator i = cInterfaces.begin(); i != cInterfaces.end(  ); i++ )
	{
		ListViewStringRow *pcRow = new ListViewStringRow();

		pcRow->AppendString( ( *i )->GetName() );
		pcRow->AppendString( ( *i )->GetEnabled()? MSG_MAINWIN_IFACE_ENABLED_YES : MSG_MAINWIN_IFACE_ENABLED_NO );
		m_pcLstIFace->InsertRow( pcRow );
		if( ( *i )->IsDefault() )
		{
			m_pcDdmDefaultIFace->SetCurrentString( ( *i )->GetName() );
		}
		m_pcDdmDefaultIFace->AppendItem( ( *i )->GetName() );
	}
}

void MainWin::Load( void )
{
	// Interface tab
	LoadInterfaces();

	// General tab
	m_pcHostname->Set( m_cPrefs.GetHostname().GetHostname(  ).c_str(  ) );
	m_pcDomainName->Set( m_cPrefs.GetHostname().GetDomain(  ).c_str(  ) );

	// DNS tab
	m_pcLstNs->Clear();
	NameserverList_t cNsList;

	m_cPrefs.GetNameserver().GetNameservers( cNsList );
	for( NameserverList_t::iterator i = cNsList.begin(); i != cNsList.end(  ); i++ )
	{
		ListViewEditRow *pcRow = new ListViewEditRow();

		pcRow->SetReadOnly( !m_cPrefs.IsRoot() );
		pcRow->AppendString( ( *i ) );
		m_pcLstNs->InsertRow( pcRow );
	}

	m_pcLstSrc->Clear();
	DomainList_t cSrcList;

	m_cPrefs.GetNameserver().GetDomains( cSrcList );
	for( DomainList_t::iterator i = cSrcList.begin(); i != cSrcList.end(  ); i++ )
	{
		ListViewEditRow *pcRow = new ListViewEditRow();

		pcRow->SetReadOnly( !m_cPrefs.IsRoot() );
		pcRow->AppendString( ( *i ) );
		m_pcLstSrc->InsertRow( pcRow );
	}

	// Hosts tab
	m_pcLstHost->Clear();
	HostEntryList_t cHostList;

	m_cPrefs.GetHosts().GetHostEntries( cHostList );
	for( HostEntryList_t::iterator i = cHostList.begin(); i != cHostList.end(  ); i++ )
	{
		ListViewEditRow *pcRow = new ListViewEditRow();

		pcRow->SetReadOnly( !m_cPrefs.IsRoot() );
		pcRow->AppendString( ( *i ).Address );

		pcRow->AppendString( ( *i ).Aliases );
		m_pcLstHost->InsertRow( pcRow );
	}
}

void MainWin::Save( void )
{
	// Interfaces tab
	InterfaceList_t cInterfaces = m_cPrefs.GetInterfaces();
	String cDefault = m_pcDdmDefaultIFace->GetCurrentString();

	for( InterfaceList_t::iterator i = cInterfaces.begin(); i != cInterfaces.end(  ); i++ )
	{
		( *i )->SetDefault( ( *i )->GetName() == cDefault );
	}

	// General tab
	m_cPrefs.GetHostname().SetHostname( m_pcHostname->GetValue(  ).AsString(  ) );
	m_cPrefs.GetHostname().SetDomain( m_pcDomainName->GetValue(  ).AsString(  ) );

	// DNS tab
	NameserverList_t cNsList;

	m_cPrefs.GetNameserver().GetNameservers( cNsList );
	for( NameserverList_t::iterator i = cNsList.begin(); i != cNsList.end(  ); i++ )
	{
		m_cPrefs.GetNameserver().DeleteNameserver( *i );
	}
	for( uint32 i = 0; i < m_pcLstNs->GetRowCount(); i++ )
	{
		m_cPrefs.GetNameserver().AddNameserver( ( ( ListViewEditRow * ) m_pcLstNs->GetRow( i ) )->GetString( 0 ) );
	}

	DomainList_t cSrcList;

	m_cPrefs.GetNameserver().GetDomains( cSrcList );
	for( DomainList_t::iterator i = cSrcList.begin(); i != cSrcList.end(  ); i++ )
	{
		m_cPrefs.GetNameserver().DeleteDomain( *i );
	}
	for( uint32 i = 0; i < m_pcLstSrc->GetRowCount(); i++ )
	{
		m_cPrefs.GetNameserver().AddDomain( ( ( ListViewEditRow * ) m_pcLstSrc->GetRow( i ) )->GetString( 0 ) );
	}

	// Hosts tab
	HostEntryList_t cHostList;

	m_cPrefs.GetHosts().GetHostEntries( cHostList );
	for( HostEntryList_t::iterator i = cHostList.begin(); i != cHostList.end(  ); i++ )
	{
		m_cPrefs.GetHosts().DeleteHostEntry( ( *i ).Address );
	}
	for( uint32 i = 0; i < m_pcLstHost->GetRowCount(); i++ )
	{
		ListViewEditRow *pcRow = ( ListViewEditRow * ) m_pcLstHost->GetRow( i );

		m_cPrefs.GetHosts().AddHostEntry( pcRow->GetString( 0 ), pcRow->GetString( 1 ) );
	}

	m_cPrefs.ApplyChanges();
}

