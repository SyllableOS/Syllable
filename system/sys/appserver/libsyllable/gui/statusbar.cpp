#include <gui/statusbar.h>

using namespace os;

/** \internal */
class StatusBar::Private
{
public:

	Private()
	{
	}

	~Private()
	{
	}
	
	LayoutNode*	m_pcRoot;
};

/** \internal */
class StatusPanel::Private
{
public:

	Private()
	{
		m_nFlags = 0;
		m_vSize = 0.0;
	}

	~Private()
	{
	}

	uint32 m_nFlags;
	double m_vSize;
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
 * \sa AddPanel, StatusPanel
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
StatusBar::StatusBar( const Rect& cFrame, const String& cName, uint32 nResizeMask, uint32 nFlags )
:LayoutView( cFrame, cName, NULL, nResizeMask, nFlags )
{
	m = new Private;
	m->m_pcRoot = new HLayoutNode( "pcRoot" );
}

StatusBar::~StatusBar()
{
	delete m;
}

/** Add Panel to StatusBar
 * \par 	Description:
 * \param	cName A name that can be used to find this panel.
 * \param	cText The text to display in the panel, the minimum width of the
 *			panel will be set to match the width of this text.
 * \param	vSize Size (optional), normally specified in number of characters
 *			of the width of m. Certain flags may result in other units being
 *			used.
 * \param	nFlags (optionnal) Use F_FIXED to fix the width to the value
 *			passed in vSize.
 * \sa StatusPanel
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
LayoutNode* StatusBar::AddPanel( const String& cName, const String& cText, double vSize, uint32 nFlags )
{
	StatusPanel* p = new StatusPanel( cName, cText, vSize, nFlags );
	return AddPanel( p );
}

LayoutNode* StatusBar::AddPanel( StatusPanel* pcPanel )
{
	SetRoot( NULL );
	LayoutNode* pcLayoutNode = m->m_pcRoot->AddChild( pcPanel, 100.0f );
	pcLayoutNode->SetBorders( Rect( 2, 2, 2, 2 ) );
	SetRoot( m->m_pcRoot );
	return pcLayoutNode;
}


/** Set a Panel's text
 * \par 	Description:
 * \param	cName Identier that was used when the panel was created.
 * \param	cText The text to display in the panel, the minimum width of the
 *			panel will be set to match the width of this text.
 * \sa AddPanel
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
void StatusBar::SetText( const String& cName, const String& cText )
{
	int i;
	View* pcView;
	
	i = 0;
	do {
		pcView = GetChildAt( i++ );
		// TODO: Add a GetChildByName() to View
		if( pcView ) {
			if( pcView->GetName() == cName ) {
				static_cast<StatusPanel *>( pcView )->SetString( cText );
				return;
			}
		}
	} while( pcView );
}

void StatusBar::__SB_reserved1__() {}
void StatusBar::__SB_reserved2__() {}
void StatusBar::__SB_reserved3__() {}
void StatusBar::__SB_reserved4__() {}
void StatusBar::__SB_reserved5__() {}

StatusPanel::StatusPanel( const String& cName, const String& cText, double vSize, uint32 nFlags )
:StringView( Rect(), cName, cText, ALIGN_LEFT )
{
	m = new Private;
	SetSize( vSize );
	SetFlags( nFlags );
	SetRenderBorder( true );
}

StatusPanel::~StatusPanel()
{
	delete m;
}

uint32 StatusPanel::GetFlags() const
{
	return m->m_nFlags;
}

void StatusPanel::SetFlags( uint32 nFlags )
{
	m->m_nFlags = nFlags;
	_UpdateSize();
}

double StatusPanel::GetSize() const
{
	return m->m_vSize;
}

void StatusPanel::SetSize( double vSize )
{
	m->m_vSize = vSize;
	_UpdateSize();
}

void StatusPanel::_UpdateSize()
{
	if( m->m_nFlags == F_FIXED ) {
		if( m->m_vSize == 0 ) {
			m->m_vSize = 5.0;
		}
		SetMinPreferredSize( (int)m->m_vSize, 0 );
		SetMaxPreferredSize( (int)m->m_vSize, 0 );
	}
}
