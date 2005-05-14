#include <gui/font.h>

#include <gui/treeview.h>

#include <typeinfo>

using namespace os;

/** \internal */
class TreeViewNode::Private
{
	public:
	bool		m_bExpanded;		// Node is expanded
	uint		m_nIndent;			// Indentation depth
	TreeView*	m_pcOwner;
	uint32		m_nLPos;			// Trunk lines
	bool		m_bLastSibling;		// true if this is the last sibling
									// (means that the last trunk segment is of half length)
};


TreeViewNode::TreeViewNode()
{
	m = new Private;
	m->m_nIndent = 0;
	m->m_bExpanded = true;
	m->m_pcOwner = NULL;
	m->m_nLPos = 0;
	m->m_bLastSibling = false;
}

TreeViewNode::~TreeViewNode()
{
	delete m;
}

bool TreeViewNode::HitTest( View* pcView, const Rect& cFrame, int nColumn, Point cPos )
{
	if( nColumn == 0 ) {
		Rect r2 = _ExpanderCrossPos( cFrame );
		//r2 += Point( cFrame.left, cFrame.top );
		if( r2.DoIntersect( cPos ) && m->m_pcOwner ) {
			if( IsExpanded() ) {
				m->m_pcOwner->Collapse( this );
			} else {			
				m->m_pcOwner->Expand( this );
			}
			return false;
		}
	}
	return true;
}

/** Set indentation depth
 *	\par	Description:
 *			Set indentation depth for a node. The hierarchical relations
 *			between nodes is decided by their indentation depths. A node with
 *			a higher indentation depth than the node before it, is considered
 *			a sub node.
 *  \param	nIndent Indentation depth.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeViewNode::SetIndent( uint nIndent )
{
	m->m_nIndent = nIndent;
}

/** Get indentation depth
 *	\par	Description:
 * \sa SetIndent()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
uint TreeViewNode::GetIndent() const
{
	return m->m_nIndent;
}

/** Set expanded state
 *	\par	Description:
 *			Set expanded state of a node. Do not call this method directly, as
 *			it does not update sub nodes. Use TreeView::Expand() instead.
 * \sa TreeView::Expand(), GetExpanded()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/	
void TreeViewNode::SetExpanded( bool bExpanded )
{
	m->m_bExpanded = bExpanded;
}

/** Get expanded state
 *	\par	Description:
 *			Returns true if the node is in expanded state (ie. sub nodes are
 *			displayed).
 * \sa SetIndent()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
bool TreeViewNode::IsExpanded() const
{
	return m->m_bExpanded;
}

/** Set owner
 *	\par	Description:
 *			This method is used by TreeView to tell nodes which TreeView they
 *			belong to. It is called automatically by TreeView::InsertNode(),
 *			so you should never need to use this method directly.
 *			
 * \sa TreeView::InsertNode(), GetOwner()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/	
void TreeViewNode::_SetOwner( TreeView* pcOwner )
{
	m->m_pcOwner = pcOwner;
}

/** Get owner
 *	\par	Description:
 *			Returns a pointer to the TreeView this node belongs to. Returns
 *			NULL if the node is not attached to a TreeView.
 *			
 * \sa _SetOwner()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/	
TreeView* TreeViewNode::GetOwner() const
{
	return m->m_pcOwner;
}

/** Set line positions
 *	\par	Description:
 *			A 'one' in the line positions integer means that a tree trunk
 *			segment should be drawn at the corresponding indentation depth
 *			on the current row.
 * \sa _GetLinePositions()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/	
void TreeViewNode::_SetLinePositions( uint32 nLPos )
{
	m->m_nLPos = nLPos;
}

/** Get line positions
 *	\par	Description:
 * \sa _SetLinePositions()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/	
uint32 TreeViewNode::_GetLinePositions() const
{
	return m->m_nLPos;
}

/** Check if node has more siblings
 *	\par	Description:
 *			Check if there is a sibling following the current node. Used when
 *			rendering the "tree trunk", to know if it should continue to the
 *			next node or if it should be finished at this node.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/	
bool TreeViewNode::IsLastSibling() const
{
	return m->m_bLastSibling;
}

/** Set LastSibling
 *	\par	Description:
 *			Used by TreeView to keep the LastSibling attribute up to date.
 *			Should not be called elsewhere.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/	
void TreeViewNode::_SetLastSibling( bool bIsLast )
{
	m->m_bLastSibling = bIsLast;
}

/** Get expander position
 *	\par	Description:
 *			Returns a rect with dimensions and position for where the expander
 *			image should be drawn.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/	
Rect TreeViewNode::_ExpanderCrossPos( const Rect &cFrame ) const
{
	if( m->m_pcOwner ) {
		Rect cImgFrame( m->m_pcOwner->GetExpanderImageBounds() );
	
		cImgFrame.right += 2;
		cImgFrame.bottom += 2;
	
		cImgFrame += Point( GetIndent() * GetOwner()->GetIndentWidth(),
						(cFrame.Height()/2 - cImgFrame.Height()/2) );
	
		return cImgFrame;
	} else {
		return Rect( 0, 0, 10, 10 );
	}
}

/** Draw expander cross
 *	\par	Description:
 *			The default expander cross drawing routine.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/	
void TreeViewNode::_DrawExpanderCross( View *pcView, const Rect& cRect )
{
	if( !m->m_pcOwner ) return;

	pcView->SetFgColor( Color32_s(0, 0, 0) );

	pcView->SetDrawingMode( DM_COPY );

	if( !IsExpanded() ) {	
        if( !m->m_pcOwner->GetCollapsedImage() ) {
			pcView->DrawLine( Point( cRect.left + cRect.Width()/2, cRect.top+2 ),
					Point( cRect.left + cRect.Width()/2, cRect.bottom-2 ) );
			pcView->DrawLine( Point( cRect.left + 2, cRect.top + cRect.Height()/2 ),
					Point( cRect.right - 2, cRect.top + cRect.Height()/2 ) );
		} else {
            BitmapImage* pcImage = (BitmapImage*)m->m_pcOwner->GetCollapsedImage();
            Point cImgPos( cRect.Width()/2 - pcImage->GetSize().x/2 + cRect.left,
            		cRect.Height()/2 - pcImage->GetSize().y/2 + cRect.top );
            pcImage->Draw( cImgPos, pcView );
		}
	} else {
        if( !m->m_pcOwner->GetExpandedImage() ) {
			pcView->DrawLine( Point( cRect.left + 2, cRect.top + cRect.Height()/2 ),
					Point( cRect.right - 2, cRect.top + cRect.Height()/2 ) );
		} else {
            BitmapImage* pcImage = (BitmapImage*)m->m_pcOwner->GetExpandedImage();
            Point cImgPos( cRect.Width()/2 - pcImage->GetSize().x/2 + cRect.left,
            		cRect.Height()/2 - pcImage->GetSize().y/2 + cRect.top );
            pcImage->Draw( cImgPos, pcView );
		}
	}

	if( m->m_pcOwner->GetDrawExpanderBox() ) {
		pcView->SetDrawingMode( DM_INVERT );
		pcView->DrawFrame( cRect, FRAME_TRANSPARENT | FRAME_THIN );
	}

	pcView->SetDrawingMode( DM_COPY );
}




/** \internal */
class TreeViewStringNode::Private
{
	public:
    std::vector< std::pair<String,float> > m_cStrings;
    Image*	m_pcIcon;
	uint32	m_nTextFlags;
    
    Private() {
    	m_pcIcon = NULL;
    	m_nTextFlags = DTF_IGNORE_FMT;
    }
    
    ~Private() {
    	if( m_pcIcon ) delete m_pcIcon;
    }
};

TreeViewStringNode::TreeViewStringNode()
{
	m = new Private;
}
TreeViewStringNode::~TreeViewStringNode()
{
	delete m;
}

void TreeViewStringNode::AttachToView( View* pcView, int nColumn )
{
	Point	cSize = pcView->GetTextExtent( m->m_cStrings[nColumn].first );
    m->m_cStrings[nColumn].second = cSize.x + 5.0f;

	if( nColumn == 0 && GetIcon() ) {
		m->m_cStrings[0].second += GetIcon()->GetSize().x + 5;
	}
}

void TreeViewStringNode::SetRect( const Rect& cRect, int nColumn )
{
}

void TreeViewStringNode::AppendString( const String& cString )
{
    m->m_cStrings.push_back( std::make_pair( cString, 0.0f ) );
}

void TreeViewStringNode::SetString( int nIndex, const String& cString )
{
    m->m_cStrings[nIndex].first = cString;
}

const String& TreeViewStringNode::GetString( int nIndex ) const
{
    return( m->m_cStrings[nIndex].first );
}

float TreeViewStringNode::GetWidth( View* pcView, int nIndex )
{
    return( m->m_cStrings[nIndex].second );
}

float TreeViewStringNode::GetHeight( View* pcView )
{
	Point	cSize( 0, 0 );
	uint i;
	
	for( i = 0; i < m->m_cStrings.size(); i++ ) {
		Point s = pcView->GetTextExtent( m->m_cStrings[i].first, m->m_nTextFlags );
		if( s.y > cSize.y ) cSize.y = s.y;
	}
     
    if( GetIcon() ) {
    	if( cSize.y < GetIcon()->GetSize().y )
	    	cSize.y = GetIcon()->GetSize().y;
    }

    return cSize.y;
}

uint32 TreeViewStringNode::GetTextFlags() const
{
	return m->m_nTextFlags;
}

void TreeViewStringNode::SetTextFlags( uint32 nTextFlags ) const
{
	m->m_nTextFlags = nTextFlags;
}

void TreeViewStringNode::Paint( const Rect& cFrame, View* pcView, uint nColumn,
			       bool bSelected, bool bHighlighted, bool bHasFocus )
{
	TreeView *pcOwner = GetOwner();
	uint nIndentWidth = 10;
	bool bExp = false;

	if( pcOwner ) {
		nIndentWidth = pcOwner->GetIndentWidth();
		bExp = pcOwner->HasChildren( this );	// FIXME: slow!
	}

	uint nIndent = GetIndent() * nIndentWidth;

	Rect cItemRect( cFrame );

	Point	cTextSize;
	if( nColumn <= m->m_cStrings.size() ) {
		cTextSize = pcView->GetTextExtent( m->m_cStrings[nColumn].first, m->m_nTextFlags );
	}

	if( nIndent && !nColumn ) {	
		cItemRect.left += nIndent + 15;
		cItemRect.right += nIndent + 15;
	} else {
		nIndent = 0;
	}

	pcView->SetDrawingMode( DM_COPY );

    pcView->SetFgColor( 255, 255, 255 );
    pcView->FillRect( cFrame );
  //  pcView->DrawFrame( cFrame, FRAME_TRANSPARENT | FRAME_THIN );

	float vTextPos = cItemRect.left + 3.0f;

	if( GetIcon() && nColumn == 0 ) {
		vTextPos += GetIcon()->GetSize().x + 5;
	}
	
	if( bSelected || bHighlighted )
	{
		Rect cSelectFrame = cFrame;
		cSelectFrame.left = cItemRect.left;
		if( nColumn == 0 ) {
			cSelectFrame.left += 2;
			cSelectFrame.top += 2;
			cSelectFrame.bottom -= 2;
		}
		if( bSelected )
			pcView->SetFgColor( 186, 199, 227 );
		else
			pcView->SetFgColor( 0, 50, 200 );
		pcView->FillRect( cSelectFrame );
	
		/* Round edges */
		if( nColumn == 0 )
		{
			pcView->DrawLine( os::Point( cSelectFrame.left + 2, cSelectFrame.top - 2 ), 
								os::Point( cSelectFrame.right, cSelectFrame.top - 2 ) );
			pcView->DrawLine( os::Point( cSelectFrame.left, cSelectFrame.top - 1 ), 
								os::Point( cSelectFrame.right, cSelectFrame.top - 1 ) );
			
			pcView->DrawLine( os::Point( cSelectFrame.left - 2, cSelectFrame.top + 2 ), 
								os::Point( cSelectFrame.left - 2, cSelectFrame.bottom - 2 ) );
			pcView->DrawLine( os::Point( cSelectFrame.left - 1, cSelectFrame.top ), 
								os::Point( cSelectFrame.left - 1, cSelectFrame.bottom ) );
								
			pcView->DrawLine( os::Point( cSelectFrame.left + 2, cSelectFrame.bottom + 2 ), 
								os::Point( cSelectFrame.right, cSelectFrame.bottom + 2 ) );
			pcView->DrawLine( os::Point( cSelectFrame.left, cSelectFrame.bottom + 1 ), 
								os::Point( cSelectFrame.right, cSelectFrame.bottom + 1 ) );
		} 
	}
 
    if ( bHighlighted ) {
		pcView->SetFgColor( 255, 255, 255 );
		pcView->SetBgColor( 0, 50, 200 );
    } else if ( bSelected ) {
		pcView->SetFgColor( 0, 0, 0 );
		pcView->SetBgColor( 186, 199, 227 );
    } else {
		pcView->SetBgColor( 255, 255, 255 );
		pcView->SetFgColor( 0, 0, 0 );
    }
/*
    if ( bSelected && nColumn == 0 ) {
		Rect cRect = cItemRect;
		cRect.right  = vTextPos + cTextSize.x + 1;
		pcView->FillRect( cRect, Color32_s( 0, 0, 0, 0 ) );
    }*/

	if( nColumn <= m->m_cStrings.size() ) {
		Rect cTextRect( vTextPos, cItemRect.top, cItemRect.right, cItemRect.bottom );
    	pcView->DrawText( cTextRect, m->m_cStrings[nColumn].first, m->m_nTextFlags );
    }

	if( GetIcon() && nColumn == 0 ) {
		pcView->SetDrawingMode( DM_BLEND );
		GetIcon()->Draw( Point( cItemRect.left + 2, cItemRect.top + (cItemRect.Height() / 2 - GetIcon()->GetSize().y / 2) ), pcView );
	}

	/* Draw Trunk (a.k.a connecting lines) */
	if( nColumn == 0 && pcOwner && pcOwner->GetDrawTrunk() ) {
		pcView->SetFgColor( 0, 0, 0 );

		uint32 bits = _GetLinePositions();
		int i = 0;
		for( ; bits ; bits >>= 1, i++ ) {
			if( bits & 1 ) {
				if( !( bits >> 1 ) && IsLastSibling() ) {
					pcView->DrawLine( Point( cFrame.left + ( i + .5 ) * (float)nIndentWidth, cFrame.top ),
							Point( cFrame.left + ( i + .5 ) * (float)nIndentWidth, cFrame.top + cFrame.Height()/2 ) );
				} else {
					pcView->DrawLine( Point( cFrame.left + ( i + .5 ) * (float)nIndentWidth, cFrame.top ),
							Point( cFrame.left + ( i + .5 ) * (float)nIndentWidth, cFrame.bottom ) );
				}
			}
		}
		
		if( i ) {
			if( bExp ) {
				pcView->DrawLine( Point( cFrame.left + ( i - .5 ) * (float)nIndentWidth, cFrame.top + cFrame.Height()/2 ),
						Point( _ExpanderCrossPos( cFrame ).left-1, cFrame.top + cFrame.Height()/2 ) );
			} else {
				pcView->DrawLine( Point( cFrame.left + ( i - .5 ) * (float)nIndentWidth, cFrame.top + cFrame.Height()/2 ),
						Point( cItemRect.left, cFrame.top + cFrame.Height()/2 ) );
			}
		}
	}

	if( nIndent && bExp ) {    
		Rect cExpCr( _ExpanderCrossPos( cFrame ) );

	    _DrawExpanderCross( pcView, cExpCr + Point( cFrame.left, cFrame.top ) );

		if( IsExpanded() && pcOwner && pcOwner->GetDrawTrunk() ) {
	    	pcView->DrawLine( Point( nIndent + cFrame.left + nIndentWidth/2, cExpCr.bottom + cFrame.top ),
					Point( nIndent + cFrame.left + nIndentWidth/2, cFrame.bottom ) );
		}
	}
	
//	pcView->Flush();
//	pcView->Sync();
}

bool TreeViewStringNode::IsLessThan( const ListViewRow* pcOther, uint nColumn ) const
{
	if( typeid( *pcOther ) == typeid( *this ) ) {
		return( GetString( nColumn ) < ((TreeViewStringNode*)pcOther)->GetString( nColumn ) );
	} else {
		return true;
	}
}
	
void TreeViewStringNode::SetIcon( Image* pcIcon )
{
	if( m->m_pcIcon ) delete m->m_pcIcon;
	m->m_pcIcon = pcIcon;
}

Image* TreeViewStringNode::GetIcon() const
{
	return m->m_pcIcon;
}






/** \internal */
class TreeView::Private
{
	public:
    Image*		m_pcExpandedImage ;		/** Expander Image used for expanded items */
    Image*		m_pcCollapsedImage;		/** Expander Image used for collapsed items */
    bool		m_bDrawBox;				/** Draw box around expander image */
	bool	    m_bDrawTrunk;			/** Draw lines to each node */
	bool	    m_bTrunkValid;			/** Lines to each node are already set up correctly */
    bool		m_bBoldFont;			/** Use bold font for expanded nodes */
    Message*	m_pcExpandMsg;			/** Message sent when expanding/collapsing node */
    uint		m_nIndentWidth;			/** Width of each indent step in pixels */
    Rect		m_cExpImgBounds;		/** Cache for expander image bounds */

	void CalcExpImgBounds() {
		m_cExpImgBounds.Resize( 0, 0, 0, 0 );

		if( m_pcExpandedImage ) {
			 m_cExpImgBounds.right = m_pcExpandedImage->GetSize().x;
			 m_cExpImgBounds.bottom = m_pcExpandedImage->GetSize().y;
		}

		if( m_pcCollapsedImage ) {
			if( m_pcCollapsedImage->GetSize().x > m_cExpImgBounds.right )
				 m_cExpImgBounds.right = m_pcCollapsedImage->GetSize().x;
			if( m_pcCollapsedImage->GetSize().y > m_cExpImgBounds.bottom )
				 m_cExpImgBounds.bottom = m_pcCollapsedImage->GetSize().y;
		}

		if( !m_cExpImgBounds.right )
			m_cExpImgBounds.Resize( 0, 0, 10, 10 );
	}

    Private()
    : m_cExpImgBounds( 0, 0, 10, 10 )
    {
	    m_pcExpandedImage = NULL;
	    m_pcCollapsedImage = NULL;
		m_pcExpandMsg = NULL;
	    m_bDrawBox = true;
	    m_bBoldFont = true;
	    m_nIndentWidth = 10;
	    m_bDrawTrunk = true;
	    m_bTrunkValid = false;
    }
    
    ~Private() {
    	if( m_pcExpandedImage ) delete m_pcExpandedImage;
    	if( m_pcCollapsedImage ) delete m_pcCollapsedImage;
		if( m_pcExpandMsg ) delete m_pcExpandMsg;
    }
};

TreeView::TreeView( const Rect& cFrame, const String& cName, uint32 nModeFlags,
                    uint32 nResizeMask, uint32 nFlags )
        : ListView( cFrame, cName, nModeFlags, nResizeMask, nFlags ) {
    m = new Private;
}
    
TreeView::~TreeView()
{
	delete m;
}

/** Insert a node at the end
 *	\par	Description:
 *			Insert a node at the end of the tree.
 *  \param	nPos Zero-based index to insert position.
 *  \param	pcNode Pointer to node to insert.
 *  \param	bUpdate Set to true to refresh TreeView.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeView::InsertNode( TreeViewNode* pcNode, bool bUpdate )
{
	pcNode->_SetOwner( this );
    m->m_bTrunkValid = false;
	ListView::InsertRow( 1 << 15, pcNode, bUpdate );
}

/** Insert a node at a certain position
 *	\par	Description:
 *			Insert a node at position nPos.
 *  \param	nPos Zero-based index to insert position.
 *  \param	pcNode Pointer to node to insert.
 *  \param	bUpdate Set to true to refresh TreeView.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeView::InsertNode( int nPos, TreeViewNode* pcNode, bool bUpdate )
{
	pcNode->_SetOwner( this );
    m->m_bTrunkValid = false;
	ListView::InsertRow( nPos, pcNode, bUpdate );
}

/** Expand a node
 *	\par	Description:
 *			Expands a node so that subnodes are made visible.
 *  \param	pcNode Pointer to node to expand.
 *
 *  \sa Collapse
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeView::Expand( TreeViewNode* pcNode )
{
	uint			count = GetRowCount();
	uint			i;
	bool			begin = false;

	ClearSelection();

	pcNode->SetExpanded( true );
	
	for(i = 0; i < count; i++)
	{
		TreeViewNode*		lvr = (TreeViewNode*)GetRow( i );
		
		if( begin ) {
			if( lvr->GetIndent() > pcNode->GetIndent() ) {
				lvr->SetIsVisible( true );
				lvr->SetExpanded( true );	// BUG: should respect previous expanded state instead
			} else {
				break;
			}
		}
		
		if( lvr == pcNode ) begin = true;		
	}
	
	RefreshLayout();
	
	if( m->m_pcExpandMsg ) {
		Message* pcMsg = new Message( *m->m_pcExpandMsg );
		pcMsg->AddPointer( "node", pcNode );
		pcMsg->AddBool( "expanded", true );
		Invoke( pcMsg );
	}
}

/** Collapse a node
 *	\par	Description:
 *			Contracts a node so that subnodes are made invisible.
 *  \param	pcNode Pointer to node to collapse.
 *
 *  \sa Expand
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeView::Collapse( TreeViewNode* pcNode )
{
	uint			count = GetRowCount();
	uint			i;
	bool			begin = false;
	
	ClearSelection();

	pcNode->SetExpanded( false );
	
	for(i = 0; i < count; i++)
	{
		TreeViewNode*		lvr = (TreeViewNode*)GetRow( i );
		
		if( begin ) {
			if( lvr->GetIndent() > pcNode->GetIndent() ) {
				lvr->SetIsVisible( false );
			} else {
				break;
			}
		}
		
		if( lvr == pcNode ) begin = true;		
	}

	RefreshLayout();

	if( m->m_pcExpandMsg ) {
		Message* pcMsg = new Message( *m->m_pcExpandMsg );
		pcMsg->AddPointer( "node", pcNode );
		pcMsg->AddBool( "expanded", false );
		Invoke( pcMsg );
	}
}

/** Find out if a node has sub nodes
 *	\par	Description:
 *			Returns true if a node has at least one child node.
 *  \param	pcNode Pointer to node.
 *
 *  \sa GetChild
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
bool TreeView::HasChildren( TreeViewNode* pcNode )
{
	uint			count = GetRowCount();
	uint			i;
	bool			begin = false;
	
	for(i = 0; i < count; i++)
	{
		TreeViewNode*		lvr = (TreeViewNode*)GetRow( i );
		
		if( begin ) {
			if( lvr->GetIndent() > pcNode->GetIndent() ) {
				return true;
			} else {
				return false;
			}
		}
		
		if( lvr == pcNode ) begin = true;		
	}

	return false;
}

void TreeView::_ConnectLines()	// Scan through the tree and connect the treetrunk to each node
{
	uint			count = GetRowCount();
	uint			i;
	uint32			bits = 0;
	uint			depth = 33;

	for( i = count; i; i-- )
	{
		TreeViewNode*		lvr = (TreeViewNode*)GetRow( i - 1 );

		depth = lvr->GetIndent();

		if( ( bits >> ( depth - 1 ) ) & 1 ) {
			lvr->_SetLastSibling( false );
		} else {
			lvr->_SetLastSibling( true );
		}

		bits &= ~( ~0 << depth );
		if( depth > 1 ) bits |= 1 << ( depth - 1 );

		lvr->_SetLinePositions( bits );
	}
}

/** Set expander image when expanded
 *	\par	Description:
 *			Used to set the expander image (in expanded state). Normally you
 *			should not use this function, but instead use the system default
 *			image.
 *  \param	pcImage Pointer to image to use for expander image.
 *
 *  \sa GetExpandedImage(), SetCollapsedImage()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeView::SetExpandedImage(Image* pcImage)
{
	if( m->m_pcExpandedImage ) delete m->m_pcExpandedImage;
    m->m_pcExpandedImage = pcImage;
	m->CalcExpImgBounds();
}

/** Set expander image when collapsed
 *	\par	Description:
 *			Used to set the expander image (in collapsed state). Normally you
 *			should not use this function, but instead use the system default
 *			image.
 *  \param	pcImage Pointer to image to use for expander image.
 *
 *  \sa GetCollapsedImage(), SetExpandedImage()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeView::SetCollapsedImage(Image* pcImage)
{
	if( m->m_pcCollapsedImage ) delete m->m_pcCollapsedImage;
    m->m_pcCollapsedImage = pcImage;
	m->CalcExpImgBounds();
}

/** Get expander image when expanded
 *	\par	Description:
 *			Returns the image used for expander icons in expanded state.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
Image* TreeView::GetExpandedImage() const
{
    return m->m_pcExpandedImage;
}

/** Get expander image when expanded
 *	\par	Description:
 *			Returns the image used for expander icons in collapsed state.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
Image* TreeView::GetCollapsedImage() const
{
    return m->m_pcCollapsedImage;
}

/** Get indentation width
 *	\par	Description:
 *			Returns the size of indentations. Default is 10 pixels.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
uint TreeView::GetIndentWidth() const
{
    return m->m_nIndentWidth;
}

/** Set indentation width
 *	\par	Description:
 *			Set the size of indentations. Default is 10 pixels.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeView::SetIndentWidth( uint nIndentWidth )
{
    m->m_nIndentWidth = nIndentWidth;
}

/** Draw box around expander image
 *	\par	Description:
 *			Returns true if a box is drawn around the expander image.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
bool TreeView::GetDrawExpanderBox() const
{
    return m->m_bDrawBox;
}

/** Draw box around expander image
 *	\par	Description:
 *			Set to true to have a box drawn around the expander image.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeView::SetDrawExpanderBox(const bool bDraw)
{
    m->m_bDrawBox = bDraw;
}

/** Draw tree trunk
 *	\par	Description:
 *			Returns true if lines are drawn to each node (aka. tree trunk).
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
bool TreeView::GetDrawTrunk() const
{
	return m->m_bDrawTrunk;
}

/** Draw tree trunk
 *	\par	Description:
 *			Set to true to have lines drawn to each node (aka. tree trunk).
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeView::SetDrawTrunk( bool bDraw )
{
	m->m_bDrawTrunk = bDraw;
}

/** Get parent node
 *	\par	Description:
 *			Returns a pointer to the parent node of pcNode, or the parent of
 *			the currently selected node, if pcNode is NULL.
 *  \param	pcNode Pointer to node.
 *
 *  \sa GetChild(), GetNext(), GetPrev(), ListView::GetRow()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
TreeViewNode* TreeView::GetParent(TreeViewNode* pcNode)
{
	uint			count = GetRowCount();
	bool			begin = false;

	if( !pcNode ) {
		int nLastSelected = GetLastSelected();
		if( nLastSelected != -1 ) {
			pcNode = (TreeViewNode*)GetRow( nLastSelected );
			count = nLastSelected;
			begin = true;
		} else return NULL;
	}

	uint			i, j = pcNode->GetIndent();

	for( i = count; i; i-- )
	{
		TreeViewNode*		node = (TreeViewNode*)GetRow( i - 1 );

		if( begin && node->GetIndent() < j )
			return node;

		if( node == pcNode )
			begin = true;
	}

	return NULL;
}

/** Get child node
 *	\par	Description:
 *			Returns a pointer to the child node (sub node) of pcNode, or the
 *			child node of the currently selected node, if pcNode is NULL.
 *  \param	pcNode Pointer to node.
 *
 *  \sa GetParent(), GetNext(), GetPrev(), ListView::GetRow()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
TreeViewNode* TreeView::GetChild(TreeViewNode* pcNode)
{
	uint			i = 0;

	if( !pcNode ) {
		int nLastSelected = GetLastSelected();
		if( nLastSelected != -1 ) {
			pcNode = (TreeViewNode*)GetRow( nLastSelected );
			i = nLastSelected;
		} else return NULL;
	}

	TreeViewNode*		node = (TreeViewNode*)GetRow( i + 1 );

	if( node ) {
		if( node->GetIndent() > pcNode->GetIndent() )
			return node;
	}
	
	return NULL;
}

/** Get previous sibling
 *	\par	Description:
 *			Returns a pointer to the previous sibling of pcNode, or the
 *			sibling of the currently selected node, if pcNode is NULL.
 *  \param	pcNode Pointer to node.
 *
 *  \sa GetChild(), GetNext(), GetParent(), ListView::GetRow()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
TreeViewNode* TreeView::GetPrev(TreeViewNode* pcNode)
{
	uint			count = GetRowCount();
	bool			begin = false;

	if( !pcNode ) {
		int nLastSelected = GetLastSelected();
		if( nLastSelected != -1 ) {
			pcNode = (TreeViewNode*)GetRow( nLastSelected );
			count = nLastSelected;
			begin = true;
		} else return NULL;
	}

	uint			i, j = pcNode->GetIndent();

	for( i = count; i; i-- )
	{
		TreeViewNode*		node = (TreeViewNode*)GetRow( i - 1 );

		if( begin ) {
			if( node->GetIndent() < j )
				return NULL;
			if( node->GetIndent() == j )
				return node;
		}

		if( node == pcNode )
			begin = true;
	}

	return NULL;
}

/** Get next sibling
 *	\par	Description:
 *			Returns a pointer to the next sibling of pcNode, or the
 *			sibling of the currently selected node, if pcNode is NULL.
 *  \param	pcNode Pointer to node.
 *
 *  \sa GetChild(), GetPrev(), GetParent(), ListView::GetRow()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
TreeViewNode* TreeView::GetNext(TreeViewNode* pcNode)
{
	uint			i = 0;

	if( !pcNode ) {
		int nLastSelected = GetLastSelected();
		if( nLastSelected != -1 ) {
			pcNode = (TreeViewNode*)GetRow( nLastSelected );
			i = nLastSelected;
		} else return NULL;
	}

	uint			count = GetRowCount();
	uint			j = pcNode->GetIndent();
	bool			begin = false;

	for( ; i < count; i++ )
	{
		TreeViewNode*		node = (TreeViewNode*)GetRow( i );

		if( begin ) {
			if( node->GetIndent() < j )
				return NULL;
			if( node->GetIndent() == j )
				return node;
		}

		if( node == pcNode )
			begin = true;
	}
	
	return NULL;
}

/** Set expand/collapse message
 *	\par	Description:
 *			Set a message to be sent when a node is expanded or collapsed.
 *			When the message is sent, a boolean named "expanded" is added,
 *			containing the new expanded state of the node (true = expanded,
 *			false = collapsed). A pointer named "node" points to the node that
 *			has been expanded/collapsed.
 *  \param	pcMsg Pointer to message to send.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeView::SetExpandedMessage( Message *pcMsg )
{
	if( m->m_pcExpandMsg ) delete m->m_pcExpandMsg;
	m->m_pcExpandMsg = pcMsg;
}

/** Set expand/collapse message
 *	\par	Description:
 *			Get expanded/collapsed message.
 * \sa SetExpandedMessage()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
Message* TreeView::GetExpandedMessage() const
{
	return m->m_pcExpandMsg;
}

/** Get subnodes
 *	\par	Description:
 *			Fills a std::vector with the sub nodes (children) of a supplied
 *			node that is attached to this TreeView, or the currently selected
 *			node if pcNode is NULL.
 *	\param	cvecChildren std::vector to be filled with pointers to nodes.
 *	\param	pcNode Pointer to node to examine or NULL for selected node.
 * \sa GetSiblings()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeView::GetChildren( std::vector<TreeViewNode*>& cvecChildren, TreeViewNode* pcNode )
{
	uint						i = 0;

	if( !pcNode ) {
		int nLastSelected = GetLastSelected();
		if( nLastSelected != -1 ) {
			pcNode = (TreeViewNode*)GetRow( nLastSelected );
			i = nLastSelected;
		} else return;
	}

	uint						count = GetRowCount();
	uint						j = pcNode->GetIndent();
	bool						begin = false;

	for( ; i < count; i++ )
	{
		TreeViewNode*		node = (TreeViewNode*)GetRow( i );

		if( begin ) {
			if( node->GetIndent() == j+1 ) {
				cvecChildren.push_back( node );
			} else if( node->GetIndent() <= j ) {
				return ;
			}
		}

		if( node == pcNode )
			begin = true;
	}
}

/** Get sibling nodes
 *	\par	Description:
 *			Fills a std::vector with the siblings of a supplied
 *			node that is attached to this TreeView, or the currently selected
 *			node if pcNode is NULL.
 *	\param	cvecSibling std::vector to be filled with pointers to nodes.
 *	\param	pcNode Pointer to node to examine or NULL for selected node.
 * \sa GetChildren()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void TreeView::GetSiblings( std::vector<TreeViewNode*>& cvecSiblings, TreeViewNode* pcNode )
{
	TreeViewNode* pcParent = GetParent( pcNode );
	if( pcParent ) {
		GetChildren( cvecSiblings, pcParent );
	}
}

/** Get max size of expander image
 *	\par	Description:
 *			Returns a Rect with the maximum dimensions for the expander icon.
 *			Normally there is no reason to call this method directly, it is
 *			used by the TreeViewNodes to calculate what their sizes will be.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
const Rect& TreeView::GetExpanderImageBounds( ) const
{
	return m->m_cExpImgBounds;
}

void TreeView::SortRows( std::vector<ListViewRow*>* pcRows, int nColumn )
{
	std::vector<ListViewRow*>::const_iterator		cSource;
	std::vector<ListViewRow*>						cDest;
	std::vector<ListViewRow*>::iterator				cIterator;
	std::vector<ListViewRow*>::iterator				cBefore;
	std::vector<int>								cPath;
	int												nIndent;
	int												nBeforeIndex = 0;

	/* NOTE: This routine can probably be optimised a lot. For starters, cDest could be */
	/* a std::list instead of a vector. */

	cPath.push_back( 0 );
	cBefore = cDest.begin();
	cSource = pcRows->begin();
	nIndent = ((TreeViewNode*)*cSource)->GetIndent();

	for( ; cSource != pcRows->end() ; cSource++ ) {
		int nNewIndent = ((TreeViewNode*)*cSource)->GetIndent();

		if( nNewIndent < nIndent ) {
			cPath.pop_back();
		}

		if( nNewIndent > nIndent ) {
			cPath.push_back( nBeforeIndex );
		}

		cBefore = cIterator = cPath.back() + cDest.begin();

		for( ; cIterator != cDest.end() ; cIterator++ ) {
			int cmpindent = ((TreeViewNode*)*cIterator)->GetIndent();
			if( cmpindent < nNewIndent ) {
				cBefore = cIterator + 1;
				break;
			}
			if( cmpindent == nNewIndent ) {
				if( (*cSource)->IsLessThan( (*cIterator), nColumn ) ) {
					break;
				}
			}
			cBefore = cIterator + 1;
		}

		nBeforeIndex = (cBefore-cDest.begin());

		cDest.insert( cBefore, (*cSource) );
	
		nIndent = nNewIndent;
	}

	*pcRows = cDest;

    m->m_bTrunkValid = false;
}

void TreeView::Paint( const Rect& cUpdateRect )
{
	if( !m->m_bTrunkValid && m->m_bDrawTrunk ) {
		_ConnectLines();
		m->m_bTrunkValid = true;
	}

	ListView::Paint( cUpdateRect );
}

/** Remove a node
 *	\par	Description:
 *			Does exactly the same thing as RemoveRow().
 * \sa ListView::RemoveRow()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
TreeViewNode* TreeView::RemoveNode( int nIndex, bool bUpdate )
{
	m->m_bTrunkValid = false;
	return (TreeViewNode*)RemoveRow( nIndex, bUpdate );
}

ListViewRow* TreeView::RemoveRow( int nIndex, bool bUpdate )
{
	m->m_bTrunkValid = false;
	return ListView::RemoveRow( nIndex, bUpdate );
}

void TreeView::Clear()
{
	m->m_bTrunkValid = false;
	ListView::Clear();
}

/* Disabled - not valid in a TreeView */

void TreeView::InsertRow( int nPos, ListViewRow* pcRow, bool bUpdate )
{
}

void TreeView::InsertRow( ListViewRow* pcRow, bool bUpdate )
{
}

void TreeView::__TVW_reserved1__()
{
}

void TreeView::__TVW_reserved2__()
{
}

void TreeView::__TVW_reserved3__()
{
}

void TreeView::__TVW_reserved4__()
{
}

void TreeView::__TVW_reserved5__()
{
}

void TreeViewNode::__TVN_reserved1__()
{
}

void TreeViewNode::__TVN_reserved2__()
{
}

void TreeViewStringNode::__TVS_reserved1__()
{
}

void TreeViewStringNode::__TVS_reserved2__()
{
}
