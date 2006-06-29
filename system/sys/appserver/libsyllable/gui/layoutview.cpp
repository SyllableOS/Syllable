
/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 The Syllable Team
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

#include <gui/layoutview.h>

#include <algorithm>
#include <stdio.h>
#include <limits.h>

using namespace os;

class LayoutView::Private
{
	public:
    LayoutNode* m_pcRoot;
    
    Private() {
    	m_pcRoot = NULL;
    }
    ~Private() {
    	delete( m_pcRoot );
    }
};

struct ShareNode {
	LayoutNode* m_pcNext;
	LayoutNode* m_pcPrev;
};

class LayoutNode::Private
{
	public:
    String		    m_cName;
    Rect		    m_cFrame;
    Rect		    m_cBorders;
    View*		    m_pcView;
    LayoutNode*		m_pcParent;
    LayoutView*		m_pcLayoutView;
    std::vector<LayoutNode*> m_cChildList;
    float		    m_vWeight;
    Point		    m_cMinSize;
    Point		    m_cMaxSizeLimit;
    Point		    m_cMaxSizeExtend;
    alignment		m_eHAlign;
    alignment	    m_eVAlign;
    ShareNode	    m_sWidthRing;
    ShareNode	    m_sHeightRing;
    
    Private() {
    	m_eHAlign = ALIGN_CENTER;
		m_eVAlign = ALIGN_CENTER;
		m_pcView = NULL;
		m_pcParent = NULL;
		m_pcLayoutView = NULL;
    }
    
    ~Private() {
    	delete( m_pcView );
    }
};

#if 0
static void AddBorders( Rect * pcRect, const Rect & cBorders )
{
	pcRect->Resize( -cBorders.left, -cBorders.top, cBorders.right, cBorders.bottom );
}
#endif

static void RemBorders( Rect * pcRect, const Rect & cBorders )
{
	pcRect->Resize( cBorders.left, cBorders.top, -cBorders.right, -cBorders.bottom );
}

LayoutView::LayoutView( const Rect & cFrame, const String & cTitle, LayoutNode * pcRoot, uint32 nResizeMask, uint32 nFlags ):View( cFrame, cTitle, nResizeMask, nFlags )
{
	m = new Private;

	SetRoot( pcRoot );
}

LayoutView::~LayoutView()
{
	delete m;
}

/** Get root layout
 * \par	Description:
 *	Get the root layout object.
 * \sa SetRoot()
 * \author Kurt Skauen
 *****************************************************************************/
LayoutNode *LayoutView::GetRoot() const
{
	return ( m->m_pcRoot );
}

/** Set root layout
 * \par	Description:
 *	Set the root layout object.
 * \param	pcRoot Pointer to root layout node.
 * \sa GetRoot()
 * \author Kurt Skauen
 *****************************************************************************/
void LayoutView::SetRoot( LayoutNode * pcRoot )
{
	if( m->m_pcRoot )
	{
		m->m_pcRoot->_RemovedFromView();
	}
	m->m_pcRoot = pcRoot;
	if( m->m_pcRoot )
	{
		m->m_pcRoot->_AddedToView( this );
		FrameSized( Point( 0, 0 ) );
	}
}

/** Recalculate layout
 * \par	Description:
 *	Causes the layout to be re-calculated.
 * \author Kurt Skauen
 *****************************************************************************/
void LayoutView::InvalidateLayout()
{
	if( m->m_pcRoot != NULL )
	{
		m->m_pcRoot->Layout();
	}
}

/** Find node by name
 * \par	Description:
 *	Find a layout node (in the hierarchy specifided with SetRoot) by its name.
 * \param	cName The name of the searched node.
 * \param	cRecursive Set to true if you want to look at the root node's subnodes.
 * \sa SetRoot()
 * \author Kurt Skauen
 *****************************************************************************/
LayoutNode *LayoutView::FindNode( const String & cName, bool bRecursive )
{
	if( m->m_pcRoot == NULL )
	{
		return ( NULL );
	}
	else
	{
		return ( m->m_pcRoot->FindNode( cName, true, true ) );
	}
}

Point LayoutView::GetPreferredSize( bool bLargest ) const
{
	if( m->m_pcRoot != NULL )
	{
		return ( m->m_pcRoot->GetPreferredSize( bLargest ) );
	}
	else
	{
		return ( Point( 0.0f, 0.0f ) );
	}
}

void LayoutView::FrameSized( const Point & cDelta )
{
	if( m->m_pcRoot != NULL )
	{
		m->m_pcRoot->SetFrame( GetNormalizedBounds() );
	}
}

void LayoutView::AllAttached()
{
	if( m->m_pcRoot != NULL )
	{
		FrameSized( Point( 0, 0 ) );
	}
}


///// LayoutNode ///////////////////////////////////////////////////////////////


LayoutNode::LayoutNode( const String & cName, float vWeight, LayoutNode * pcParent, View * pcView )
{
	m = new Private;

	m->m_cName = cName;
	m->m_cBorders = Rect( 0.0f, 0.0f, 0.0f, 0.0f );
	m->m_cMinSize = Point( 0.0f, 0.0f );
	m->m_cMaxSizeExtend = Point( 0.0f, 0.0f );
	m->m_cMaxSizeLimit = Point( MAX_SIZE, MAX_SIZE );

	m->m_vWeight = vWeight;
	m->m_pcView = pcView;

	if( pcParent != NULL )
	{
		pcParent->AddChild( this );
	}

	m->m_sWidthRing.m_pcNext = this;
	m->m_sWidthRing.m_pcPrev = this;
	m->m_sHeightRing.m_pcNext = this;
	m->m_sHeightRing.m_pcPrev = this;
}

LayoutNode::~LayoutNode()
{
	std::vector<LayoutNode*> cChildList = m->m_cChildList;
	for( uint i = 0; i < cChildList.size(); ++i )
	{
		delete cChildList[i];
	}
	if( m->m_pcParent != NULL )
	{
		m->m_pcParent->RemoveChild( this );
	}
	delete m;
}

void LayoutNode::AddChild( LayoutNode * pcChild )
{
	for( uint i = 0; i < m->m_cChildList.size(); ++i )
	{
		if( m->m_cChildList[i] == pcChild )
		{
			dbprintf( "Error: LayoutNode::AddChild() node %s already added!\n", pcChild->GetName().c_str() );
			return;
		}
	}
	m->m_cChildList.push_back( pcChild );
	pcChild->_AddedToParent( this );
}

LayoutNode *LayoutNode::AddChild( View * pcChildView, float vWeight )
{
	return ( new LayoutNode( pcChildView->GetName(), vWeight, this, pcChildView ) );
}

void LayoutNode::RemoveChild( LayoutNode * pcChild )
{
	std::vector < LayoutNode * >::iterator i = std::find( m->m_cChildList.begin(), m->m_cChildList.end(  ), pcChild );

	if( i == m->m_cChildList.end() )
	{
		dbprintf( "Error: LayoutNode::RemoveChild() could not find children!\n" );
		return;
	}
	m->m_cChildList.erase( i );
	pcChild->_RemovedFromParent();
}

void LayoutNode::RemoveChild( View * pcChildView )
{
	for( uint i = 0; i < m->m_cChildList.size(); ++i )
	{
		if( m->m_cChildList[i]->m->m_pcView == pcChildView )
		{
			LayoutNode *pcChild = m->m_cChildList[i];

			pcChild->SetView( NULL );
			m->m_cChildList.erase( m->m_cChildList.begin() + i );
			pcChild->_RemovedFromParent();
			delete pcChild;
		}
	}
}


void LayoutNode::SetView( View * pcView )
{
	if( m->m_pcView != NULL )
	{
		if( m->m_pcLayoutView != NULL )
		{
			m->m_pcLayoutView->RemoveChild( m->m_pcView );
		}
	}
	m->m_pcView = pcView;
	if( m->m_pcLayoutView != NULL && m->m_pcView != NULL )
	{
		m->m_pcLayoutView->AddChild( m->m_pcView );
		m->m_pcView->SetFrame( GetAbsFrame() );
	}
}

View* LayoutNode::GetView() const
{
	return m->m_pcView;
}

void LayoutNode::Layout()
{
	Rect cFrame( m->m_cFrame );

	cFrame.Resize( m->m_cBorders.left, m->m_cBorders.top, -m->m_cBorders.right, -m->m_cBorders.bottom );

	for( uint i = 0; i < m->m_cChildList.size(); ++i )
	{
		m->m_cChildList[i]->SetFrame( cFrame );
	}

}

void LayoutNode::SetBorders( const Rect & cBorder )
{
	m->m_cBorders = cBorder;
	Layout();
}

Rect LayoutNode::GetBorders() const
{
	return ( m->m_cBorders );
}

float LayoutNode::GetWeight() const
{
	return ( m->m_vWeight );
}

void LayoutNode::SetWeight( float vWeight )
{
	m->m_vWeight = vWeight;
}

void LayoutNode::SetHAlignment( alignment eAlignment )
{
	m->m_eHAlign = eAlignment;
	Layout();
}

void LayoutNode::SetVAlignment( alignment eAlignment )
{
	m->m_eVAlign = eAlignment;
	Layout();
}

alignment LayoutNode::GetHAlignment() const
{
	return ( m->m_eHAlign );
}

alignment LayoutNode::GetVAlignment() const
{
	return ( m->m_eVAlign );
}

void LayoutNode::SetFrame( const Rect & cFrame )
{
	m->m_cFrame = cFrame;
	if( m->m_pcView != NULL )
	{
		Rect cViewFrame = GetAbsFrame();

		cViewFrame.Resize( m->m_cBorders.left, m->m_cBorders.top, -m->m_cBorders.right, -m->m_cBorders.bottom );
		cViewFrame.Floor();
		
		
		m->m_pcView->SetFrame( cViewFrame );
	}
	Layout();
}

Rect LayoutNode::GetFrame() const
{
	return ( m->m_cFrame );
}

Rect LayoutNode::GetBounds() const
{
	return ( GetFrame().Bounds(  ) );
}

Rect LayoutNode::GetAbsFrame() const
{
	if( m->m_pcParent != NULL )
	{
		return ( m->m_cFrame + m->m_pcParent->GetAbsFrame().LeftTop(  ) );
	}
	else
	{
		return ( m->m_cFrame );
	}
}

void LayoutNode::AdjustPrefSize( Point * pcMinSize, Point * pcMaxSize )
{
	if( pcMinSize->x < m->m_cMinSize.x )
		pcMinSize->x = m->m_cMinSize.x;
	if( pcMinSize->y < m->m_cMinSize.y )
		pcMinSize->y = m->m_cMinSize.y;

	if( pcMaxSize->x < m->m_cMaxSizeExtend.x )
		pcMaxSize->x = m->m_cMaxSizeExtend.x;
	if( pcMaxSize->y < m->m_cMaxSizeExtend.y )
		pcMaxSize->y = m->m_cMaxSizeExtend.y;

	if( pcMaxSize->x > m->m_cMaxSizeLimit.x )
		pcMaxSize->x = m->m_cMaxSizeLimit.x;
	if( pcMaxSize->y > m->m_cMaxSizeLimit.y )
		pcMaxSize->y = m->m_cMaxSizeLimit.y;

	if( pcMaxSize->x < pcMinSize->x )
		pcMaxSize->x = pcMinSize->x;
	if( pcMaxSize->y < pcMinSize->y )
		pcMaxSize->y = pcMinSize->y;
}

Point LayoutNode::CalculatePreferredSize( bool bLargest )
{
	Point cSize( 0.0f, 0.0f );

	if( m->m_pcView != NULL )
	{
		cSize = m->m_pcView->GetPreferredSize( bLargest );
		cSize.x = ceil( cSize.x );
		cSize.y = ceil( cSize.y );
	}

	return ( cSize + Point( m->m_cBorders.left + m->m_cBorders.right, m->m_cBorders.top + m->m_cBorders.bottom ) );
}

Point LayoutNode::GetPreferredSize( bool bLargest )
{
	if( m->m_sWidthRing.m_pcNext == this && m->m_sHeightRing.m_pcNext == this )
	{
		return ( CalculatePreferredSize( bLargest ) );
	}

	Point cMinSize = CalculatePreferredSize( false );
	Point cMaxSize;

	if( m->m_sWidthRing.m_pcNext == this || m->m_sHeightRing.m_pcNext == this )
	{
		cMaxSize = CalculatePreferredSize( true );
	}
	else
	{
		cMaxSize = cMinSize;
	}
	Point cSize = cMinSize;

	if( bLargest )
	{
		if( m->m_sWidthRing.m_pcNext == this )
		{
			cSize.x = cMaxSize.x;
		}
		if( m->m_sHeightRing.m_pcNext == this )
		{
			cSize.y = cMaxSize.y;
		}
	}
	for( LayoutNode * pcNode = m->m_sWidthRing.m_pcNext; pcNode != this; pcNode = pcNode->m->m_sWidthRing.m_pcNext )
	{
		Point cSSize = pcNode->CalculatePreferredSize( false );

		if( cSSize.x > cSize.x )
		{
			cSize.x = cSSize.x;
		}
	}
	for( LayoutNode * pcNode = m->m_sHeightRing.m_pcNext; pcNode != this; pcNode = pcNode->m->m_sHeightRing.m_pcNext )
	{
		Point cSSize = pcNode->CalculatePreferredSize( false );

		if( cSSize.y > cSize.y )
		{
			cSize.y = cSSize.y;
		}
	}

	return ( cSize );
}

String LayoutNode::GetName() const
{
	return ( m->m_cName );
}

const std::vector < LayoutNode * >&LayoutNode::GetChildList() const
{
	return ( m->m_cChildList );
}

LayoutNode *LayoutNode::GetParent() const
{
	return ( m->m_pcParent );
}

LayoutView *LayoutNode::GetLayoutView() const
{
	return ( m->m_pcLayoutView );
}

LayoutNode *LayoutNode::FindNode( const String & cName, bool bRecursive, bool bIncludeSelf )
{
	if( bIncludeSelf && cName == m->m_cName )
	{
		return ( this );
	}
	for( uint i = 0; i < m->m_cChildList.size(); ++i )
	{
		if( bRecursive )
		{
			LayoutNode *pcChild = m->m_cChildList[i]->FindNode( cName, true );

			if( pcChild != NULL )
			{
				return ( pcChild );
			}
		}
		if( m->m_cChildList[i]->m->m_cName == cName )
		{
			return ( m->m_cChildList[i] );
		}
	}
	return ( NULL );
}

#define FOR_EACH_NAME( name1, func ) { \
    va_list pArgs; va_start( pArgs, name1 );	\
    for( const char* name = name1 ; name != NULL ; name = va_arg(pArgs,const char*) ) {	\
	LayoutNode* pcNode = FindNode( name, true, true );				\
	if ( pcNode != NULL ) {								\
	    pcNode->func;								\
	} else {									\
	    dbprintf( "Warning: LayoutNode::%s() could not find node '%s'\n", __FUNCTION__, name );	\
	}										\
    }											\
    va_end( pArgs );									\
}


void LayoutNode::SetBorders( const Rect & cBorders, const char *pzName1, ... )
{
	FOR_EACH_NAME( pzName1, SetBorders( cBorders ) );
}

void LayoutNode::SetWeights( float vWeight, const char *pzFirstName, ... )
{
	FOR_EACH_NAME( pzFirstName, SetWeight( vWeight ) );
}

void LayoutNode::SetHAlignments( alignment eAlign, const char *pzFirstName, ... )
{
	FOR_EACH_NAME( pzFirstName, SetHAlignment( eAlign ) );
}

void LayoutNode::SetVAlignments( alignment eAlign, const char *pzFirstName, ... )
{
	FOR_EACH_NAME( pzFirstName, SetVAlignment( eAlign ) );
}


void LayoutNode::ExtendMinSize( const Point & cMinSize )
{
	m->m_cMinSize = cMinSize;
}

void LayoutNode::LimitMaxSize( const Point & cMaxSize )
{
	m->m_cMaxSizeLimit = cMaxSize;
}

void LayoutNode::ExtendMaxSize( const Point & cMaxSize )
{
	m->m_cMaxSizeExtend = cMaxSize;
}

void LayoutNode::AddToWidthRing( LayoutNode * pcRing )
{
	m->m_sWidthRing.m_pcNext = pcRing;
	m->m_sWidthRing.m_pcPrev = pcRing->m->m_sWidthRing.m_pcPrev;
	pcRing->m->m_sWidthRing.m_pcPrev->m->m_sWidthRing.m_pcNext = this;
	pcRing->m->m_sWidthRing.m_pcPrev = this;
}

void LayoutNode::AddToHeightRing( LayoutNode * pcRing )
{
	m->m_sHeightRing.m_pcNext = pcRing;
	m->m_sHeightRing.m_pcPrev = pcRing->m->m_sHeightRing.m_pcPrev;
	pcRing->m->m_sHeightRing.m_pcPrev->m->m_sHeightRing.m_pcNext = this;
	pcRing->m->m_sHeightRing.m_pcPrev = this;
}

void LayoutNode::SameWidth( const char *pzName1, ... )
{
	if( pzName1 == NULL )
	{
		/* Reset */
		m->m_sWidthRing.m_pcNext = this;
		m->m_sWidthRing.m_pcPrev = this;
		return;
	}

	va_list pArgs;

	va_start( pArgs, pzName1 );
	LayoutNode *pcFirstNode = FindNode( pzName1, true, true );

	if( pcFirstNode == NULL )
	{
		dbprintf( "LayoutNode::SameWidth() failed to find node '%s'\n", pzName1 );
		return;
	}

	for( const char *pzName = va_arg( pArgs, const char * ); pzName != NULL; pzName = va_arg( pArgs, const char * ) )
	{
		LayoutNode *pcNode = FindNode( pzName, true, true );

		if( pcNode == NULL )
		{
			dbprintf( "LayoutNode::SameWidth() failed to find node '%s'\n", pzName );
			continue;
		}
		pcNode->AddToWidthRing( pcFirstNode );
	}
	va_end( pArgs );
}

void LayoutNode::SameHeight( const char *pzName1, ... )
{
	if( pzName1 == NULL )
	{
		/* Reset */
		m->m_sHeightRing.m_pcNext = this;
		m->m_sHeightRing.m_pcPrev = this;
		return;
	}

	va_list pArgs;

	va_start( pArgs, pzName1 );
	LayoutNode *pcFirstNode = FindNode( pzName1, true, true );

	if( pcFirstNode == NULL )
	{
		dbprintf( "LayoutNode::SameHeight() failed to find node '%s'\n", pzName1 );
		return;
	}

	for( const char *pzName = va_arg( pArgs, const char * ); pzName != NULL; pzName = va_arg( pArgs, const char * ) )
	{
		LayoutNode *pcNode = FindNode( pzName, true, true );

		if( pcNode == NULL )
		{
			dbprintf( "LayoutNode::SameHeight() failed to find node '%s'\n", pzName );
			continue;
		}
		pcNode->AddToHeightRing( pcFirstNode );
	}
	va_end( pArgs );
}

void LayoutNode::_AddedToParent( LayoutNode * pcParent )
{
	m->m_pcParent = pcParent;
	m->m_pcLayoutView = pcParent->m->m_pcLayoutView;
	if( m->m_pcView != NULL && m->m_pcLayoutView != NULL )
	{
		m->m_pcLayoutView->AddChild( m->m_pcView );
	}
	for( uint i = 0; i < m->m_cChildList.size(); ++i )
	{
		m->m_cChildList[i]->_AddedToParent( this );
	}
}

void LayoutNode::_AddedToView( LayoutView * pcView )
{
	m->m_pcLayoutView = pcView;

	if( m->m_pcView != NULL )
	{
		m->m_pcLayoutView->AddChild( m->m_pcView );
	}
	for( uint i = 0; i < m->m_cChildList.size(); ++i )
	{
		m->m_cChildList[i]->_AddedToView( pcView );
	}
}

void LayoutNode::_RemovedFromParent()
{
	if( m->m_pcView != NULL && m->m_pcLayoutView != NULL )
	{
		m->m_pcLayoutView->RemoveChild( m->m_pcView );
	}
	m->m_pcParent = NULL;
	m->m_pcLayoutView = NULL;
	for( uint i = 0; i < m->m_cChildList.size(); ++i )
	{
		m->m_cChildList[i]->_RemovedFromParent();
	}
}

void LayoutNode::_RemovedFromView()
{
	if( m->m_pcView != NULL && m->m_pcLayoutView != NULL )
	{
		m->m_pcLayoutView->RemoveChild( m->m_pcView );
	}
	m->m_pcLayoutView = NULL;
	for( uint i = 0; i < m->m_cChildList.size(); ++i )
	{
		m->m_cChildList[i]->_RemovedFromView();
	}
}

void LayoutNode::__LYN_reserved1__() {}
void LayoutNode::__LYN_reserved2__() {}
void LayoutNode::__LYN_reserved3__() {}
void LayoutNode::__LYN_reserved4__() {}
void LayoutNode::__LYN_reserved5__() {}

static float SpaceOut( uint nCount, float vTotalSize, float vTotalMinSize, float vTotalWeight, float *avMinSizes, float *avMaxSizes, float *avWeights, float *avFinalSizes )
{
	float vExtraSpace = vTotalSize - vTotalMinSize;

	bool *abDone = ( bool * )alloca( sizeof( bool ) * nCount );

	memset( abDone, 0, sizeof( bool ) * nCount );

	for( ;; )
	{
		uint i;

		for( i = 0; i < nCount; ++i )
		{
			if( abDone[i] )
			{
				continue;
			}
			float vWeight = avWeights[i] / vTotalWeight;

			avFinalSizes[i] = avMinSizes[i] + vExtraSpace * vWeight;
			if( avFinalSizes[i] >= avMaxSizes[i] )
			{
				vExtraSpace -= avMaxSizes[i] - avMinSizes[i];
				vTotalWeight -= avWeights[i];
				avFinalSizes[i] = avMaxSizes[i];
				abDone[i] = true;
				break;
			}
		}
		if( i == nCount )
		{
			break;
		}
	}
	for( uint i = 0; i < nCount; ++i )
	{
		vTotalSize -= avFinalSizes[i];
	}
	return ( vTotalSize );
}

///// HLayoutNode //////////////////////////////////////////////////////////////

HLayoutNode::HLayoutNode( const String & cName, float vWeight, LayoutNode * pcParent, View * pcView ):LayoutNode( cName, vWeight, pcParent, pcView )
{
}

Point HLayoutNode::CalculatePreferredSize( bool bLargest )
{
	const std::vector < LayoutNode * >cList = GetChildList();
	Rect cBorders( GetBorders() );
	Point cSize( 0.0f, 0.0f );

	for( uint i = 0; i < cList.size(); ++i )
	{
		Point cChildSize = cList[i]->GetPreferredSize( bLargest );

		cSize.x += cChildSize.x;
		if( cChildSize.y > cSize.y )
		{
			cSize.y = cChildSize.y;
		}
	}
	cSize += Point( cBorders.left + cBorders.right, cBorders.top + cBorders.bottom );
	return ( cSize );
}

void HLayoutNode::Layout()
{
	Rect cBounds = GetBounds();
	const std::vector < LayoutNode * >cChildList = GetChildList();

	RemBorders( &cBounds, GetBorders() );

	float *avMinWidths = ( float * )alloca( sizeof( float ) * cChildList.size() );
	float *avMaxWidths = ( float * )alloca( sizeof( float ) * cChildList.size() );
	float *avMaxHeights = ( float * )alloca( sizeof( float ) * cChildList.size() );
	float *avWeights = ( float * )alloca( sizeof( float ) * cChildList.size() );

	float *avFinalHeights = ( float * )alloca( sizeof( float ) * cChildList.size() );
	float vMinWidth = 0.0f;
	float vTotalWeight = 0.0f;

	for( uint i = 0; i < cChildList.size(); ++i )
	{
		Point cMinSize = cChildList[i]->GetPreferredSize( false );
		Point cMaxSize = cChildList[i]->GetPreferredSize( true );

		cChildList[i]->AdjustPrefSize( &cMinSize, &cMaxSize );
		avWeights[i] = cChildList[i]->GetWeight();

		avMaxHeights[i] = cMaxSize.y;
		avMaxWidths[i] = cMaxSize.x;
		avMinWidths[i] = cMinSize.x;

		vMinWidth += cMinSize.x;
		vTotalWeight += avWeights[i];
	}
//    if ( vMinWidth > cBounds.Width() + 1.0f ) {
//      printf( "Error: HLayoutNode::Layout() Width=%.2f, Required width=%.2f\n", cBounds.Width() + 1.0f, vMinWidth  );
//    }
	float vUnusedWidth = SpaceOut( cChildList.size(), cBounds.Width(  ) + 1.0f, vMinWidth, vTotalWeight,
		avMinWidths, avMaxWidths, avWeights, avFinalHeights );

	vUnusedWidth /= float ( cChildList.size() );
	float x = cBounds.left + vUnusedWidth * 0.5f;

	for( uint i = 0; i < cChildList.size(); ++i )
	{
		Rect cFrame( 0.0f, 0.0f, avFinalHeights[i] - 1.0f, cBounds.bottom - cBounds.top );

		if( cFrame.Height() + 1.0f > avMaxHeights[i] )
		{
			cFrame.bottom = avMaxHeights[i] - 1.0f;
		}
		cFrame += Point( x, cBounds.top + ( cBounds.Height() - cFrame.Height(  ) ) * 0.5f );
		x += cFrame.Width() + 1.0f + vUnusedWidth;
		cFrame.Floor();
		cChildList[i]->SetFrame( cFrame );
	}

}

void HLayoutNode::__LHLN_reserved1__() {}
void HLayoutNode::__LHLN_reserved2__() {}
void HLayoutNode::__LHLN_reserved3__() {}

///// VLayoutNode //////////////////////////////////////////////////////////////

VLayoutNode::VLayoutNode( const String & cName, float vWeight, LayoutNode * pcParent, View * pcView ):LayoutNode( cName, vWeight, pcParent, pcView )
{
}

Point VLayoutNode::CalculatePreferredSize( bool bLargest )
{
	const std::vector < LayoutNode * >cList = GetChildList();
	Rect cBorders( GetBorders() );
	Point cSize( 0.0f, 0.0f );

	for( uint i = 0; i < cList.size(); ++i )
	{
		Point cChildSize = cList[i]->GetPreferredSize( bLargest );

		cSize.y += cChildSize.y;
		if( cChildSize.x > cSize.x )
		{
			cSize.x = cChildSize.x;
		}
	}
	cSize += Point( cBorders.left + cBorders.right, cBorders.top + cBorders.bottom );
	return ( cSize );
}

void VLayoutNode::Layout()
{
	Rect cBounds = GetBounds();
	const std::vector < LayoutNode * >cChildList = GetChildList();

	RemBorders( &cBounds, GetBorders() );

	float *avMaxWidths = ( float * )alloca( sizeof( float ) * cChildList.size() );
	float *avMinHeights = ( float * )alloca( sizeof( float ) * cChildList.size() );
	float *avMaxHeights = ( float * )alloca( sizeof( float ) * cChildList.size() );
	float *avWeights = ( float * )alloca( sizeof( float ) * cChildList.size() );
	float *avFinalHeights = ( float * )alloca( sizeof( float ) * cChildList.size() );
	float vTotalWeight = 0.0f;
	float vMinHeight = 0.0f;
	float vMaxHeight = 0.0f;

	for( uint i = 0; i < cChildList.size(); ++i )
	{
		Point cMinSize = cChildList[i]->GetPreferredSize( false );
		Point cMaxSize = cChildList[i]->GetPreferredSize( true );

		cChildList[i]->AdjustPrefSize( &cMinSize, &cMaxSize );

		avWeights[i] = cChildList[i]->GetWeight();
		avMaxWidths[i] = cMaxSize.x;
		avMinHeights[i] = cMinSize.y;
		avMaxHeights[i] = cMaxSize.y;

		vMinHeight += cMinSize.y;
		vMaxHeight += cMaxSize.y;
		vTotalWeight += avWeights[i];
	}
//    if ( vMinHeight > cBounds.Height() + 1.0f ) {
//      printf( "Error: HLayoutNode::Layout() Width=%.2f, Required width=%.2f\n", cBounds.Height() + 1.0f, vMinHeight  );
//    }
	float vUnusedHeight = SpaceOut( cChildList.size(), cBounds.Height(  ) + 1.0f, vMinHeight, vTotalWeight,
		avMinHeights, avMaxHeights, avWeights, avFinalHeights );

	vUnusedHeight /= float ( cChildList.size() );
	float y = cBounds.top + vUnusedHeight * 0.5f;

	for( uint i = 0; i < cChildList.size(); ++i )
	{
		Rect cFrame( 0.0f, 0.0f, cBounds.right - cBounds.left, avFinalHeights[i] - 1.0f );

		if( cFrame.Width() + 1.0f > avMaxWidths[i] )
		{
			cFrame.right = avMaxWidths[i] - 1.0f;
		}
		float x;

		switch ( GetHAlignment() )
		{
		case ALIGN_LEFT:
			x = cBounds.left;
			break;
		case ALIGN_RIGHT:
			x = cBounds.right - cFrame.Width() - 1.0f;
			break;
		default:
			dbprintf( "Error: VLayoutNode::Layout() node '%s' has invalid h-alignment %d\n", GetName().c_str(  ), GetHAlignment(  ) );
		case ALIGN_CENTER:
			x = cBounds.left + ( cBounds.Width() - cFrame.Width(  ) ) * 0.5f;
			break;
		}
		cFrame += Point( x, y );
		y += cFrame.Height() + 1.0f + vUnusedHeight;
		cFrame.Floor();
		cChildList[i]->SetFrame( cFrame );
	}
}

void VLayoutNode::__LVLN_reserved1__() {}
void VLayoutNode::__LVLN_reserved2__() {}
void VLayoutNode::__LVLN_reserved3__() {}

LayoutSpacer::LayoutSpacer( const String & cName, float vWeight, LayoutNode * pcParent, const Point & cMinSize, const Point & cMaxSize ):LayoutNode( cName, vWeight, pcParent )
{
	m_cMinSize = cMinSize;
	m_cMaxSize = cMaxSize;
}

void LayoutSpacer::SetMinSize( const Point & cSize )
{
	m_cMinSize = cSize;
}

void LayoutSpacer::SetMaxSize( const Point & cSize )
{
	m_cMaxSize = cSize;
}

Point LayoutSpacer::GetMinSize() const
{
	return( m_cMinSize );
}

Point LayoutSpacer::GetMaxSize() const
{
	return( m_cMaxSize );
}

Point LayoutSpacer::CalculatePreferredSize( bool bLargest )
{
	return ( ( bLargest ) ? m_cMaxSize : m_cMinSize );
}


void LayoutView::__LYV_reserved1__()
{
}

void LayoutView::__LYV_reserved2__()
{
}

void LayoutView::__LYV_reserved3__()
{
}

void LayoutView::__LYV_reserved4__()
{
}

void LayoutView::__LYV_reserved5__()
{
}




















