#include <stdio.h>
#include <gui/font.h>
#include "diskmanager.h"

using namespace os;




DiskManagerView::DiskManagerView( const std::string& cName ) : LayoutView( Rect(), cName, NULL, CF_FOLLOW_NONE )
{
    VLayoutNode* pcRoot = new VLayoutNode( "root1" );

    pcRoot->AddChild( new PartitionView( "disk_1", "/dev/disk/bios/hda/" ) );
    pcRoot->AddChild( new PartitionView( "disk_2", "/dev/disk/bios/hdb/" ) );
    
    SetRoot( pcRoot );
}

DiskManagerView::~DiskManagerView()
{
}








///////////////////////////////////////////////////////////////////////////////
///  PartitionView ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

PartitionView::PartitionView( const std::string& cName, const std::string& cDevicePath ) :
	View( Rect(), cName, CF_FOLLOW_NONE, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE ), m_cDevicePath( cDevicePath )
{
    m_nDiskSize = 3000;
    m_nSelected = 1;
    m_cPartitionList.push_back( PartitionEntry( 0, 999 ) );
    m_cPartitionList.push_back( PartitionEntry( 1000, 1999 ) );
    m_cPartitionList.push_back( PartitionEntry( 2000, 2999 ) );
}


Point PartitionView::GetPreferredSize( bool bLargest ) const
{
    return( (bLargest) ? Point( 100000.0f, 20.0f ) : Point( 50.0f, 20.0f ) );
}

void PartitionView::Paint( const Rect& cUpdateRect )
{
    Rect cBounds = GetNormalizedBounds();
    cBounds.Floor();

    m_cPartitionFrames.resize( m_cPartitionList.size() );

    Rect cPartFrame = cBounds;
    cPartFrame.Resize( 2.0f, 2.0f, -2.0f, -2.0f );
    for ( uint i = 0 ; i < m_cPartitionList.size() ; ++i ) {
	m_cPartitionFrames[i] = cPartFrame;
	m_cPartitionFrames[i].left  = 2 + (double(m_cPartitionList[i].m_nStart) / double(m_nDiskSize)) * (cBounds.Width() - 3.0f );
	m_cPartitionFrames[i].right = 2 + (double(m_cPartitionList[i].m_nEnd) / double(m_nDiskSize)) * (cBounds.Width() - 3.0f );
	m_cPartitionFrames[i].Floor();
	if ( i == 0 ) {
	    if ( m_cPartitionFrames[i].left < 2.0f ) {
		m_cPartitionFrames[i].left = 2.0f;
	    }
	} else {
	    if ( m_cPartitionFrames[i].left <= m_cPartitionFrames[i-1].right ) {
		m_cPartitionFrames[i].left = m_cPartitionFrames[i-1].right + 1.0f;
	    }
	}
    }
    DrawFrame( cBounds, FRAME_RECESSED );

    FontHeight_s sFontHeight;
    GetFontHeight( &sFontHeight );

    bool bIsActive = HasFocus();
    for ( uint i = 0 ; i < m_cPartitionFrames.size() ; ++i ) {
	Rect cFrame = m_cPartitionFrames[i];
	DrawFrame( cFrame, FRAME_RAISED | FRAME_TRANSPARENT );
	cFrame.Resize( 2.0f, 2.0f, -2.0f, -2.0f );
	if ( bIsActive && i == uint(m_nSelected) ) {
	    FillRect( cFrame, Color32_s( 0,0,255 ) );
	    SetBgColor( 0,0,255 );
	} else {
	    SetBgColor( get_default_color( COL_NORMAL ) );
	}
	SetFgColor( 0, 0, 0 );
	char zString[1024];
	sprintf( zString, "%.1fKB", double(m_cPartitionList[i].m_nEnd - m_cPartitionList[i].m_nStart + 1) / 1024.0 );
	float vStrWidth = GetStringWidth( zString );
	MovePenTo( cFrame.left + (cFrame.Width()+1.0f) * 0.5f - vStrWidth * 0.5f,
		   cFrame.top + (cFrame.Height()+1.0f) * 0.5f -
		   float(sFontHeight.nAscender - sFontHeight.nDescender) * 0.5f +
		   sFontHeight.nAscender + float(sFontHeight.nLineGap) * 0.5f );
	DrawString( zString );
    }
}

void PartitionView::Activated( bool bIsActive )
{
    Invalidate();
    Flush();
}

void PartitionView::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
}

void PartitionView::MouseDown( const Point& cPosition, uint32 nButton )
{
    if ( nButton == 1 ) {
	MakeFocus();
	m_nSelected = -1;
	for ( uint i = 0 ; i < m_cPartitionFrames.size() ; ++i ) {
	    if ( m_cPartitionFrames[i].DoIntersect( cPosition ) ) {
		m_nSelected = i;
		break;
	    }
	}
	Invalidate();
	Flush();
    }
}

void PartitionView::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
}
