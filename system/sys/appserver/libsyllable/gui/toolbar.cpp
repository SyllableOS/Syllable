#include <gui/toolbar.h>
#include <gui/imagebutton.h>
#include <gui/popupmenu.h>
#include <gui/separator.h>

using namespace os;

/** \internal */
class ToolBar::Private
{
public:

	Private()
	{
	}

	~Private()
	{
	}
	
	LayoutNode*	m_pcRoot;
	LayoutNode* m_pcSpacer;
};

/** Constructor
 * \par 	Description:
 * \param	cFrame The frame rectangle in the parents coordinate system.
 * \param	pzName The logical name of the view. This parameter is newer
 *	   	rendered anywhere, but is passed to the Handler::Handler()
 *		constructor to identify the view.
 * \param	nResizeMask Flags defining how the view's frame rectangle is
 *		affected if the parent view is resized.
 * \param	nFlags Various flags to control the view's behaviour.
 * \sa 
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
ToolBar::ToolBar( const Rect& cFrame, const String& cName, uint32 nResizeMask, uint32 nFlags )
:LayoutView( cFrame, cName, NULL, nResizeMask, nFlags )
{
	m = new Private;
	m->m_pcRoot = new HLayoutNode( "pcRoot" );
	m->m_pcSpacer = new HLayoutSpacer( "__tb_spacer" );
	m->m_pcRoot->AddChild( m->m_pcSpacer );
	SetRoot( m->m_pcRoot );
}

ToolBar::~ToolBar()
{
	delete m;
}

/** Add button to ToolBar
 * \par 	Description:
 * \param	cName A name that can be used to find this object.
 * \param	cText The text to display in the button.
 * \param	pcIcon Icon to show above the text (or null for no icon).
 * \param	pcMsg Message to be sent upon activation (clicking the button).
 * \sa AddChild(), AddSeparator(), AddPopupMenu()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
LayoutNode* ToolBar::AddButton( const String& cName, const String& cText, Image* pcIcon, Message* pcMsg )
{
	ImageButton* pcButton = new ImageButton( Rect(), cName, cText, pcMsg, pcIcon, ImageButton::IB_TEXT_BOTTOM, true, true, true );
	return AddChild( pcButton, TB_FIXED_WIDTH );
}

/** Add PopupMenu to ToolBar
 * \par 	Description:
 * \param	cName A name that can be used to find this object.
 * \param	cText The text to display in the button.
 * \param	pcIcon Icon to show above the text (or null for no icon).
 * \param	pcMenu Menu to display.
 * \param	pcMsg Message to be sent upon activation (clicking the button).
 * \sa AddButton(), AddSeparator(), AddChild()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
LayoutNode* ToolBar::AddPopupMenu( const String& cName, const String& cText, Image* pcIcon, Menu* pcMenu )
{
	PopupMenu* pcButton = new PopupMenu( Rect(), cName, cText, pcMenu, pcIcon );
	return AddChild( pcButton, TB_FIXED_WIDTH );
}

/** Add Separator to ToolBar
 * \par 	Description:
 * \param	cName A name that can be used to find this object.
 * \sa AddButton(), AddChild(), AddPopupMenu()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
LayoutNode* ToolBar::AddSeparator( const String& cName )
{
	Separator* pcSeparator = new Separator( Rect(), cName );
	return AddChild( pcSeparator, TB_FIXED_MINIMUM );
}

/** Add View to ToolBar
 * \par 	Description:
 * AddChild() can be used to add any kind of View object to the ToolBar.
 * \param	pcView View to add.
 * \param	eMode TB_FIXED_WIDTH or TB_FREE_WIDTH
 * \param	vWeight Weight
 * \sa AddButton(), AddSeparator(), AddPopupMenu()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
LayoutNode* ToolBar::AddChild( View* pcView, ToolBarObjectMode eMode, float vWeight )
{
	SetRoot( NULL );

	LayoutNode* pcLayoutNode = m->m_pcRoot->AddChild( pcView, ( eMode == TB_FIXED_WIDTH ? 0.0f : eMode == TB_FIXED_MINIMUM ? 0.1f : vWeight ) );
	pcLayoutNode->SetBorders( Rect( 2, 2, 2, 2 ) );

	std::vector<LayoutNode*> nodes = m->m_pcRoot->GetChildList();
	std::vector<LayoutNode*>::iterator i;
	Point minsize( 0.0f, 0.0f );
	for( i = nodes.begin(); i != nodes.end(); i++ ) {
		LayoutNode *n = (*i);
		if( n->GetWeight() == 0.0f ) {
			View *v = n->GetView();
			if( v ) {
				Point p = v->GetPreferredSize( false );
				minsize.x = std::max( minsize.x, p.x );
				minsize.y = std::max( minsize.y, p.y );
			}
		}
	}

	for( i = nodes.begin(); i != nodes.end(); i++ ) {
		LayoutNode *n = (*i);
		if( n->GetWeight() == 0.0f ) {
			n->ExtendMinSize( minsize );
		}
		if( n->GetWeight() == 0.1f ) {
			View *v = n->GetView();
			if( v ) {
				Point p = v->GetPreferredSize( false );
				p.y = COORD_MAX;
				n->LimitMaxSize( p );
			}
		}
	}

	m->m_pcRoot->RemoveChild( m->m_pcSpacer );
	m->m_pcRoot->AddChild( m->m_pcSpacer );
	SetRoot( m->m_pcRoot );
	return pcLayoutNode;
}

void ToolBar::__TB_reserved1__() {}
void ToolBar::__TB_reserved2__() {}
void ToolBar::__TB_reserved3__() {}
void ToolBar::__TB_reserved4__() {}
void ToolBar::__TB_reserved5__() {}

