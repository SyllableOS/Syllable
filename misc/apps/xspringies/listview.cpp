#include "altgui.hpp"

#include <gui/message.hpp>



StringItem_c::StringItem_c( const char* pzString )
{
  if ( NULL != pzString )
  {
    m_pzString = new char[ strlen( pzString ) + 1 ];
    strcpy( m_pzString, pzString );
  }
  else
  {
    m_pzString = NULL;
  }
}

StringItem_c::~StringItem_c()
{
  delete[] m_pzString;
}

void	StringItem_c::Update( Widget_c* pcOwner, Font_c* pcFont )
{
  if ( NULL != m_pzString )
  {
    Point2_c	cSize;

    if ( NULL != pcFont )
    {
      FontHeight_s	sHeight;

      pcFont->GetHeight( &sHeight );

      cSize = Point2_c( pcFont->GetStringWidth( m_pzString ), sHeight.nAscender - sHeight.nDescender + sHeight.nLineGap );
    }
    SetSize( cSize );
  }
}

void	StringItem_c::Draw( Widget_c* pcOwner, Rect_c cFrame, bool bFinal )
{
  if ( NULL != m_pzString )
  {
    pcOwner->SetFgColor( 0, 0, 0, 0 );

    Font_c*	pcFont	=	pcOwner->GetFont();

    if ( NULL != pcFont )
    {
      FontHeight_s	sHeight;
	//		int						nStrWidth	=	pcFont->GetStringWidth( GetString() );

      pcFont->GetHeight( &sHeight );

      pcOwner->DrawString( m_pzString, Point2_c( cFrame.MinX, cFrame.MinY + cFrame.Height() / 2 - (sHeight.nAscender - sHeight.nDescender) / 2 + sHeight.nAscender ) );
    }
  }
}

ListItem_c::ListItem_c()
{
  m_pcNext				=	NULL;
  m_pcFirstChild	=	NULL;
  m_bExpanded			=	false;
}

ListItem_c::~ListItem_c()
{
}

bool	ListItem_c::IsExpanded( void ) const
{
  return( m_bExpanded );
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

ListView_c::ListView_c( const Rect_c& cFrame, const char* pzName, ListViewType_e eType, uint32 nResizeMask, uint32 nFlags )
	: Widget_c( cFrame, pzName, nResizeMask, nFlags )
{
  m_pcFirstItem	=	NULL;
  m_nItemCount	=	0;

  m_eListType = eType;

  m_pcSelectionMsg	=	NULL;
  m_pcInvocationMsg	=	NULL;

  m_nLeftBorder	=	3;
  m_nTopBorder	=	2;
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

ListView_c::~ListView_c()
{
}

void	ListView_c::AttachedToWindow( void )
{
  ListItem_c*	pcItem;

  Font_c*	pcFont = GetFont();

  for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext ) {
    pcItem->Update( this, pcFont );
  }
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

void	ListView_c::Paint( const Rect_c& cUpdateRect )
{
  ListItem_c*	pcItem;
  int					y = m_nTopBorder;

  Rect_c	cBounds = GetBounds();

  SetFgColor( GetDefColor( PEN_WINCLIENT ) );
  FillRect( cBounds );

  for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext )
  {
    Point2_c cSize = pcItem->GetSize();

    pcItem->Draw( this, Rect_c( m_nLeftBorder, y, cBounds.MaxX, y + cSize.Y - 1 ), true );

    y += cSize.Y;

    if ( y > cBounds.MaxY ) {
      break;
    }
  }
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

bool	ListView_c::AddItem( ListItem_c* pcItem )
{
  if ( NULL == m_pcFirstItem ) {
    m_pcFirstItem = pcItem;
  } else {
    ListItem_c*	pcPrev;


    for ( pcPrev = m_pcFirstItem ; NULL != pcPrev->m_pcNext ; pcPrev = pcPrev->m_pcNext ) {
    }
    pcPrev->m_pcNext = pcItem;
  }
  pcItem->m_pcNext = NULL;
  m_nItemCount++;
  return( true );
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

bool	ListView_c::AddItem( ListItem_c* pcItem, int nIndex )
{
  if ( 0 == nIndex )
  {
    pcItem->m_pcNext = m_pcFirstItem;
    m_pcFirstItem = pcItem;
    m_nItemCount++;
    return( true );
  }
  else
  {
    ListItem_c* pcPrev = GetItemAt( nIndex - 1 );

    if ( NULL != pcPrev )
    {
      pcItem->m_pcNext = pcPrev->m_pcNext;
      pcPrev->m_pcNext = pcItem;
      m_nItemCount++;
      return( true );
    }
  }
  return( false );
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

int	ListView_c::GetItemCount( void ) const
{
  return( m_nItemCount );
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

ListItem_c*	ListView_c::GetItemAt( int nIndex ) const
{
  ListItem_c*	pcItem;

  if ( nIndex >= 0 )
  {
    for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext )
    {
      if ( 0 == nIndex ) {
	return( pcItem );
      }
      nIndex--;
    }
  }
  return( NULL );
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

ListItem_c*	ListView_c::GetItemAt( Point2_c	cPos ) const
{
  ListItem_c*	pcItem;
  int					y = 0;

  for ( pcItem = m_pcFirstItem ; NULL != pcItem ; pcItem = pcItem->m_pcNext )
  {
    Point2_c cSize = pcItem->GetSize();

    if (  cPos.Y < y + cSize.Y && cPos.Y >= y ) {
      return( pcItem );
    }
    y += cSize.Y;
  }
  return( NULL );
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

bool	ListView_c::RemoveItem( ListItem_c* pcItem )
{
  ListItem_c** ppcPrev;

  for ( ppcPrev = &m_pcFirstItem ; NULL != *ppcPrev ; ppcPrev = &(*ppcPrev)->m_pcNext )
  {
    if ( *ppcPrev == pcItem ) {
      *ppcPrev = pcItem->m_pcNext;
      m_nItemCount--;
      return( true );
    }
  }
  return( false );
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

ListItem_c*	ListView_c::RemoveItem( int nIndex )
{
  ListItem_c*	pcItem = NULL;

  if ( 0 == nIndex )
  {
    pcItem = m_pcFirstItem;

    if ( NULL != pcItem ) {
      m_pcFirstItem = m_pcFirstItem->m_pcNext;
      m_nItemCount--;
    }
  }
  else
  {
    ListItem_c*	pcPrev = GetItemAt( nIndex - 1 );

    if ( NULL != pcPrev )
    {
      pcItem = pcPrev->m_pcNext;

      if ( NULL != pcItem ) {
	pcPrev->m_pcNext = pcItem->m_pcNext;
	m_nItemCount--;
      }
    }
  }
  return( pcItem );
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

bool	ListView_c::RemoveItems( int nIndex, int nCount )
{
  for ( int i = 0 ; i < nCount ; ++i ) {
    if ( RemoveItem( nIndex ) == false ) {
      return( false );
    }
  }
  return( true );
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

void	ListView_c::SetSelectionMessage( Message_c* pcMsg )
{
  delete m_pcSelectionMsg;

  m_pcSelectionMsg = pcMsg;
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

void	ListView_c::SetInvocationMessage( Message_c* pcMsg )
{
  delete	m_pcInvocationMsg;
  m_pcInvocationMsg = pcMsg;
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

Message_c*	ListView_c::GetSelectionMessage( void ) const
{
  return( m_pcSelectionMsg );
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

int	ListView_c::GetSelectionCode( void ) const
{
  if ( NULL != m_pcSelectionMsg ) {
    return( m_pcSelectionMsg->GetCode() );
  } else {
    return( 0 );
  }
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

Message_c*	ListView_c::GetInvocationMessage( void ) const
{
  return( m_pcInvocationMsg );
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

int	ListView_c::GetInvocationCode( void ) const
{
  if ( NULL != m_pcInvocationMsg ) {
    return( m_pcInvocationMsg->GetCode() );
  } else {
    return( 0 );
  }
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

void	ListView_c::SetListType( ListViewType_e eType )
{
  m_eListType = eType;
}

//--- ListView_c::	----------------------------------------------------------
//
//	SYNOPSIS:
//
//	PURPOSE
//
//	INPUT
//
//	OUTPUT
//
//	NOTES
//
//	SEE ALSO
//
//----------------------------------------------------------------------------

ListViewType_e	ListView_c::GetListType( void ) const
{
  return( m_eListType );
}
