#include "altgui.hpp"

#include <gui/message.hpp>


FileRequester_c::FileRequester_c( FileReqMode_e eMode , Messenger_c* pcTarget, bool bMultiSelect, Message_c* pcMsg, bool bModal, bool bHideWhenDone )
{
  m_eMode					=	eMode;
  m_bHideWhenDone	=	bHideWhenDone;
  m_pcTarget			=	pcTarget;
  m_pcMessage			=	pcMsg;
  m_pcWindow			=	NULL;
  m_bModal				=	bModal;
  m_bMultiSelect	=	m_bMultiSelect;
}

FileRequester_c::~FileRequester_c()
{
}

FileReqMode_e	FileRequester_c::GetMode( void ) const
{
}

void	FileRequester_c::SetMode( FileReqMode_e eMode )
{
}

void	FileRequester_c::Refresh( void )
{
}

void	FileRequester_c::SelectionChanged()
{
}

void	FileRequester_c::SetButtonLabel( ReqButton_e eWhich, const char* pzLabel )
{
}

void	FileRequester_c::SetPath( const char* pzPath )
{
}

void	FileRequester_c::SetHideWhenDone( bool bHide )
{
  m_bHideWhenDone = bHide;
}

bool	FileRequester_c::GetHideWhenDone( void ) const
{
  return( m_bHideWhenDone );
}

void	FileRequester_c::SetMessage( Message_c* pcMsg )
{
  delete m_pcMessage;
  m_pcMessage = pcMsg;
}

void	FileRequester_c::SetTarget( Messenger_c* pcTarget )
{
  delete m_pcTarget;
  m_pcTarget = pcTarget;
}

Messenger_c*	FileRequester_c::GetTarget( void ) const
{
  return( m_pcTarget );
}

void	FileRequester_c::Show( void )
{
  Widget_c*	pcFrame = new Widget_c( Rect_c( 0, 0, 200, 100 ), "", CF_FOLLOW_ALL );

  ListView_c*	pcListView = new ListView_c( Rect_c( 20, 30, 180, 70 ), "list_view", SINGLE_SELECTION_LIST, CF_FOLLOW_ALL );

  pcListView->AddItem( new StringItem_c( "file1" ) );
  pcListView->AddItem( new StringItem_c( "file2" ) );
  pcListView->AddItem( new StringItem_c( "file3" ) );
  pcListView->AddItem( new StringItem_c( "file4" ) );
  pcListView->AddItem( new StringItem_c( "file5" ) );

  pcFrame->AddChild( pcListView );

  m_pcWindow = new Window_c( g_pcApp->GetDefaultScreen(), Rect_c( 30, 30, 230, 130 ), "FILE_REQ", "Load file:", 0  );

  m_pcWindow->AddChild( pcFrame );
}

void	FileRequester_c::Hide( void )
{
  if ( NULL != m_pcWindow )
  {
    m_pcWindow->Quit();
    m_pcWindow = NULL;
  }
}

bool	FileRequester_c::IsShowing( void ) const
{
  return( NULL != m_pcWindow );
}

void	FileRequester_c::WasHidden( void )	// Called when the requester is automatically hidden.
{
}

Window_c*	FileRequester_c::GetWindow( void ) const
{
  return( m_pcWindow );
}
