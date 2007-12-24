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

#include <gui/layoutview.h>
#include <gui/stringview.h>
#include <util/message.h>
#include <util/messenger.h>
#include <gui/requesters.h>

#include "IFaceWin.h"
#include "Event.h"
#include "Strings.h"

IFaceWin::IFaceWin( Window * pcParent, Interface * pcIFace, bool bReadOnly ):Window( Rect( 0, 0, 180, 180 ), "IFaceWin", "" )
{
	m_pcIFace = pcIFace;
	m_bReadOnly = bReadOnly;
	m_pcParent = pcParent;

	SetTitle( MSG_IFWIN_TITLE + " " + pcIFace->GetName() );

	LayoutView *pcLayout;
	VLayoutNode *pcRoot;
	HLayoutNode *pcTypeRow;
	StringView *pcLblType;
	HLayoutNode *pcAddressRow;
	StringView *pcLblAddress;
	HLayoutNode *pcNetmaskRow;
	StringView *pcLblNetmask;
	HLayoutNode *pcGatewayRow;
	StringView *pcLblGateway;
	HLayoutNode *pcEnabledRow;
	StringView *pcLblEnabled;
	HLayoutNode *pcBtnRow;
	HLayoutSpacer *pcSpacer;
	Button *pcBtnApply;
	Button *pcBtnClose;

	pcRoot = new VLayoutNode( "pcRoot" );
	pcLayout = new LayoutView( Rect( 0, 0, 180, 180 ), "pcLayout", pcRoot );

	pcTypeRow = new HLayoutNode( "pcTypeRow" );
	pcLblType = new StringView( Rect(), "pcLblType", MSG_IFWIN_TYPE );
	m_pcDdmType = new DropdownMenu( Rect(), "m_pcDdmType" );
	m_pcDdmType->SetReadOnly( true );
	m_pcDdmType->AppendItem( MSG_IFWIN_TYPE_STATIC );
	m_pcDdmType->AppendItem( MSG_IFWIN_TYPE_DHCP );
	m_pcDdmType->SetSelectionMessage( new Message( ChangeType ) );
	m_pcDdmType->SetTarget( this );
	m_pcDdmType->SetTabOrder( 0 );
	pcTypeRow->AddChild( pcLblType );
	pcTypeRow->AddChild( m_pcDdmType );
	pcTypeRow->LimitMaxSize( Point( INT_MAX, m_pcDdmType->GetPreferredSize( false ).y ) );
	pcRoot->AddChild( pcTypeRow );

	pcAddressRow = new HLayoutNode( "pcAddressRow" );
	pcLblAddress = new StringView( Rect(), "pcLblAddress", MSG_IFWIN_ADDRESS );
	m_pcTxtAddress = new TextView( Rect(), "m_pcTxtAddress", "" );
	m_pcTxtAddress->SetMaxLength( 15 );
	m_pcTxtAddress->SetTabOrder( 1 );
	pcAddressRow->AddChild( pcLblAddress );
	pcAddressRow->AddChild( m_pcTxtAddress );
	pcAddressRow->LimitMaxSize( Point( INT_MAX, m_pcTxtAddress->GetPreferredSize( false ).y ) );
	pcRoot->AddChild( pcAddressRow );

	pcNetmaskRow = new HLayoutNode( "pcNetmaskRow" );
	pcLblNetmask = new StringView( Rect(), "pcLblNetmask", MSG_IFWIN_NETMASK );
	m_pcTxtNetmask = new TextView( Rect(), "m_pcTxtNetmask", "" );
	m_pcTxtNetmask->SetMaxLength( 15 );
	m_pcTxtNetmask->SetTabOrder( 2 );
	pcNetmaskRow->AddChild( pcLblNetmask );
	pcNetmaskRow->AddChild( m_pcTxtNetmask );
	pcNetmaskRow->LimitMaxSize( Point( INT_MAX, m_pcTxtNetmask->GetPreferredSize( false ).y ) );
	pcRoot->AddChild( pcNetmaskRow );

	pcGatewayRow = new HLayoutNode( "pcGatewayRow" );
	pcLblGateway = new StringView( Rect(), "pcLblGateway", MSG_IFWIN_GATEWAY );
	m_pcTxtGateway = new TextView( Rect(), "m_pcTxtGateway", "" );
	m_pcTxtGateway->SetMaxLength( 15 );
	m_pcTxtGateway->SetTabOrder( 3 );
	pcGatewayRow->AddChild( pcLblGateway );
	pcGatewayRow->AddChild( m_pcTxtGateway );
	pcGatewayRow->LimitMaxSize( Point( INT_MAX, m_pcTxtGateway->GetPreferredSize( false ).y ) );
	pcRoot->AddChild( pcGatewayRow );

	pcEnabledRow = new HLayoutNode( "pcEnabledRow" );
	pcLblEnabled = new StringView( Rect(), "pcLblEnabled", MSG_IFWIN_ENABLED );
	m_pcCbEnabled = new CheckBox( Rect(), "m_pcCbEnabled", "", new Message( ChangeEnabled ) );
	m_pcCbEnabled->SetTabOrder( 4 );
	pcEnabledRow->AddChild( pcLblEnabled );
	pcEnabledRow->AddChild( m_pcCbEnabled );
	pcEnabledRow->LimitMaxSize( Point( INT_MAX, m_pcCbEnabled->GetPreferredSize( false ).y ) );
	pcRoot->AddChild( pcEnabledRow );

	pcBtnRow = new HLayoutNode( "pcBtnRow" );
	pcSpacer = new HLayoutSpacer( "pcSpacer" );
	pcBtnApply = new Button( Rect(), "pcBtnApply", MSG_IFWIN_APPLY, new Message( Apply ) );
	pcBtnApply->SetTabOrder( 5 );
	pcBtnClose = new Button( Rect(), "pcBtnClose", MSG_IFWIN_CLOSE, new Message( Close ) );
	pcBtnClose->SetTabOrder( 6 );
	pcBtnRow->AddChild( pcSpacer );
	pcBtnRow->AddChild( pcBtnApply );
	pcBtnRow->AddChild( pcBtnClose );
	pcBtnRow->LimitMaxSize( Point( INT_MAX, pcBtnApply->GetPreferredSize( false ).y ) );
	pcRoot->AddChild( pcBtnRow );

	pcRoot->SameWidth( "pcLblEnabled", "pcLblGateway", "pcLblNetmask", "pcLblAddress", "pcLblType", NULL );
	pcRoot->SameWidth( "m_pcTxtGateway", "m_pcTxtNetmask", "m_pcTxtAddress", "m_pcDdmType", "m_pcCbEnabled", NULL );
	pcRoot->SetBorders( Rect( 2, 2, 2, 2 ), "pcLblEnabled", "pcLblGateway", "pcLblNetmask", "pcLblAddress", "pcLblType", "m_pcTxtGateway", "m_pcTxtNetmask", "m_pcTxtAddress", "m_pcDdmType", "m_pcCbEnabled", "pcBtnApply", "pcBtnClose", NULL );
	AddChild( pcLayout );

	Load();

	if( m_bReadOnly )
	{
		m_pcDdmType->SetEnable( false );
		m_pcTxtAddress->SetEnable( false );
		m_pcTxtNetmask->SetEnable( false );
		m_pcTxtGateway->SetEnable( false );
		m_pcCbEnabled->SetEnable( false );
		pcBtnApply->SetEnable( false );
	}
}

IFaceWin::~IFaceWin()
{

}

void IFaceWin::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case Apply:
		ApplyChanges();
		break;

	case ChangeType:
		ChangeIFaceType( m_pcDdmType->GetCurrentString() == MSG_IFWIN_TYPE_STATIC ? Static : DHCP );
		break;

	case Close:
		SendCloseMessage();
		break;
	}
}

bool IFaceWin::OkToQuit( void )
{
	SendCloseMessage();
	return false;
}

void IFaceWin::SendCloseMessage( void )
{
	Messenger cMsnger( m_pcParent );
	Message *pcMsg = new Message( CloseInterfaceWindow );

	pcMsg->AddString( INTERFACE_NAME_KEY, m_pcIFace->GetName() );
	cMsnger.SendMessage( pcMsg );
}

void IFaceWin::Load( void )
{
	m_pcCbEnabled->SetValue( m_pcIFace->GetEnabled() );
	switch ( m_pcIFace->GetType() )
	{
	case Static:
		{
			m_pcDdmType->SetCurrentString( MSG_IFWIN_TYPE_STATIC );
			m_pcTxtAddress->Set( IPAddressToString( m_pcIFace->GetAddress() ).c_str(  ) );
			m_pcTxtNetmask->Set( IPAddressToString( m_pcIFace->GetNetmask() ).c_str(  ) );
			m_pcTxtGateway->Set( IPAddressToString( m_pcIFace->GetGateway() ).c_str(  ) );
			break;
		}

	case DHCP:
		{
			m_pcDdmType->SetCurrentString( MSG_IFWIN_TYPE_DHCP );
			m_pcTxtAddress->SetEnable( false );
			m_pcTxtNetmask->SetEnable( false );
			m_pcTxtGateway->SetEnable( false );
			break;
		}
	}
}

void IFaceWin::ChangeIFaceType( IFaceType_t nType )
{
	switch ( nType )
	{
	case Static:
		m_pcTxtAddress->SetEnable( true );
		m_pcTxtNetmask->SetEnable( true );
		m_pcTxtGateway->SetEnable( true );
		break;

	case DHCP:
		m_pcTxtAddress->SetEnable( false );
		m_pcTxtNetmask->SetEnable( false );
		m_pcTxtGateway->SetEnable( false );
		break;
	}
}

void IFaceWin::ApplyChanges( void )
{
	IFaceType_t nType = m_pcDdmType->GetCurrentString() == MSG_IFWIN_TYPE_STATIC ? Static : DHCP;
	bool bDefault = m_pcIFace->IsDefault();
	bool bError = false;

	switch ( nType )
	{
	case Static:
		{
			IPAddress_t nIP, nNM, nGW;
			String cError;

			if( !ParseIPAddress( m_pcTxtAddress->GetValue().AsString(  ), &nIP ) )
			{
				bError = true;
				cError = MSG_IFWIN_ERR_IP_ADDRESS;
			}
			else if( !ParseIPAddress( m_pcTxtNetmask->GetValue().AsString(  ), &nNM ) )
			{
				bError = true;
				cError = MSG_IFWIN_ERR_NETMASK;
			}
			else if( !ParseIPAddress( m_pcTxtGateway->GetValue().AsString(  ), &nGW ) )
			{
				bError = true;
				cError = MSG_IFWIN_ERR_GATEWAY;
			}

			if( bError )
			{
				Alert *pcAlert = new Alert( MSG_IFWIN_ERR_TITLE, cError, Alert::ALERT_WARNING, 0, MSG_IFWIN_ERR_CLOSE.c_str(), NULL );

				pcAlert->Go( new Invoker( new Message() ) );
			}
			else
			{
				m_pcIFace->SetAddress( nIP );
				m_pcIFace->SetNetmask( nNM );
				m_pcIFace->SetGateway( nGW );
			}
			break;
		}

	case DHCP:
		break;
	}

	if( !bError )
	{
		m_pcIFace->SetType( nType );
		m_pcIFace->SetEnabled( m_pcCbEnabled->GetValue().AsBool(  ) );
		m_pcIFace->SetDefault( bDefault );
		Messenger cMsnger( m_pcParent );

		cMsnger.SendMessage( new Message( ApplyInterfaceChanges ) );
	}
}

