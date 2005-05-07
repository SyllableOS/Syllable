#include <gui/separator.h>

using namespace os;

/** \internal */
class Separator::Private
{
      public:

	Private()
	{
	}

	~Private()
	{
	}

	orientation m_eOrientation;
};

/** Constructor
 * \par 	Description:
 * \param	cFrame The frame rectangle in the parents coordinate system.
 * \param	pzName The logical name of the view. This parameter is newer
 *	   	rendered anywhere, but is passed to the Handler::Handler()
 *		constructor to identify the view.
 * \param	eOrientation VERTICAL or HORIZONTAL.
 * \param	nResizeMask Flags defining how the views frame rectangle is
 *		affected if the parent view is resized.
 * \param	nFlags Various flags to control the views behaviour.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
Separator::Separator( Rect cFrame, const String& cName, orientation eOrientation, uint32 nResizeMask, uint32 nFlags )
:View( cFrame, cName, nResizeMask, nFlags )
{
	m = new Private;
	m->m_eOrientation = eOrientation;
}

Separator::~Separator()
{
	delete m;
}

/** View callback, Renders the Separator
 * \par 	Description:
 *		This method is called by the OS when the Separator needs to
 *		redraw itself.
 * \param	cUpdateRect The part of the Separator that needs refreshing.
 * \sa os::Image, os::View
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void Separator::Paint( const Rect & cUpdateRect )
{
	Rect cBounds( GetBounds() );

	SetDrawingMode( DM_COPY );

	SetEraseColor( get_default_color( COL_NORMAL ) );
	EraseRect( cUpdateRect );

	if( m->m_eOrientation == VERTICAL ) {
		float middle = cBounds.left + cBounds.Width() / 2;
		SetFgColor( get_default_color( COL_SHADOW ) );
		MovePenTo( middle, cBounds.top );
		DrawLine( Point( middle - 1, cBounds.top ) );
		DrawLine( Point( middle - 1, cBounds.bottom ) );
		SetFgColor( get_default_color( COL_SHINE ) );
		MovePenTo( middle, cBounds.bottom );
		DrawLine( Point( middle + 1, cBounds.bottom ) );
		DrawLine( Point( middle + 1, cBounds.top ) );
	} else {
		float middle = cBounds.top + cBounds.Height() / 2;
		SetFgColor( get_default_color( COL_SHADOW ) );
		MovePenTo( cBounds.left, middle );
		DrawLine( Point( cBounds.left, middle - 1 ) );
		DrawLine( Point( cBounds.right, middle - 1 ) );
		SetFgColor( get_default_color( COL_SHINE ) );
		MovePenTo( cBounds.right, middle );
		DrawLine( Point( cBounds.right, middle + 1 ) );
		DrawLine( Point( cBounds.left, middle + 1 ) );
	}
}

/** Returns size for automatic layout
 * \par 	Description:
 *		This method is used by LayoutNode classes to calculate the
 *		size of the object.
 * \param	bLargest Tells whether the smallest (normal) size is requested,
 *		or the largest possible size.
 * \retval	Returns (3, 3) if bLargest is false, otherwise
 *		it returns Point( COORD_MAX, COORD_MAX ).
 * \sa os::View, os::LayoutNode, os::LayoutView
 * \author Henrik Isaksson (henrik@boing.nu)
 *****************************************************************************/
Point Separator::GetPreferredSize( bool bLargest ) const
{
	if( bLargest ) {
		return Point( COORD_MAX, COORD_MAX );
	} else {
		return Point( 3, 3 );
	}
}

void Separator::SetOrientation( orientation eOrientation )
{
	m->m_eOrientation = eOrientation;
}

orientation Separator::GetOrientation() const
{
	return 	m->m_eOrientation;
}

void Separator::__SV_reserved1__() {}
void Separator::__SV_reserved2__() {}
void Separator::__SV_reserved3__() {}
void Separator::__SV_reserved4__() {}
void Separator::__SV_reserved5__() {}


