

#include "qnamespace.h"
#include <qpoint.h>
//#include <qrect.h>
//#include <qsize.h>
//#include <qregion.h>


class QEvent : public Qt		// event base class
{
public:
    enum Type {

	// NOTE: if you get a strange compiler error on the line with "None",
	//       it's probably because you're trying to include X11, which
	//	 has a mess of #defines in it.  Put the messy X11 includes
	//	 *AFTER* the nice clean Qt includes.
	None = 0,				// invalid event


	Timer = 1,				// timer event
	MouseButtonPress = 2,			// mouse button pressed
	MouseButtonRelease = 3,			// mouse button released
	MouseButtonDblClick= 4,			// mouse button double click
	MouseMove = 5,				// mouse move
/*	
	KeyPress = 6,				// key pressed
	KeyRelease = 7,				// key released
	FocusIn = 8,				// keyboard focus received
	FocusOut = 9,				// keyboard focus lost
	Enter = 10,				// mouse enters widget
	Leave = 11,				// mouse leaves widget
	Paint = 12,				// paint widget
	Move = 13,				// move widget
	Resize = 14,				// resize widget
	Create = 15,				// after object creation
	Destroy = 16,				// during object destruction
	Show = 17,				// widget is shown
	Hide = 18,				// widget is hidden
	Close = 19,				// request to close widget
	Quit = 20,				// request to quit application
	Reparent = 21,				// widget has been reparented
	ShowMinimized = 22,		       	// widget is shown minimized
	ShowNormal = 23,	       		// widget is shown normal
	WindowActivate = 24,	       		// window was activated
	WindowDeactivate = 25,	       		// window was deactivated
	ShowToParent = 26,	       		// widget is shown to parent
	HideToParent = 27,	       		// widget is hidden to parent
	ShowMaximized = 28,		       	// widget is shown maximized
	Accel = 30,				// accelerator event
	Wheel = 31,				// wheel event
	*/	
	AccelAvailable = 32,			// accelerator available event
/*	
	CaptionChange = 33,			// caption changed
	IconChange = 34,			// icon changed
	ParentFontChange = 35,			// parent font changed
	ApplicationFontChange = 36,		// application font changed
	ParentPaletteChange = 37,		// parent font changed
	ApplicationPaletteChange = 38,		// application palette changed
	Clipboard = 40,				// internal clipboard event
	Speech = 42,				// reserved for speech input
	SockAct = 50,				// socket activation
	AccelOverride = 51,			// accelerator override event
	DragEnter = 60,				// drag moves into widget
	DragMove = 61,				// drag moves in widget
	DragLeave = 62,				// drag leaves or is cancelled
	Drop = 63,				// actual drop
	DragResponse = 64,			// drag accepted/rejected
	*/	
	ChildInserted = 70,			// new child widget
	ChildRemoved = 71,			// deleted child widget
/*	
	LayoutHint = 72,			// child min/max size changed
	ShowWindowRequest = 73,			// widget's window should be mapped
	ActivateControl = 80,			// ActiveX activation
	DeactivateControl = 81,			// ActiveX deactivation
	*/	
	User = 1000				// first user event id
    };

    QEvent( Type type ) : t(type)/*, posted(FALSE)*/ {}
    virtual ~QEvent() {}
    Type  type() const	{ return t; }
protected:
    Type  t;
//private:
//    bool  posted;
};



class QMouseEvent : public QEvent
{
public:
//    QMouseEvent( Type type, const QPoint &pos, int button, int state );

    QMouseEvent( Type type, const QPoint &pos, const QPoint&globalPos, int nButton, int nButtonState )
//		 int button, int state )
	    : QEvent(type), p(pos), g(globalPos), m_nButtonState(nButtonState), b(nButton) {} //,s((ushort)state) {};

    const QPoint &pos() const	{ return p; }
    const QPoint &globalPos() const { return g; }
    int	   x()		const	{ return p.x(); }
    int	   y()		const	{ return p.y(); }
    int	   globalX()	const	{ return g.x(); }
    int	   globalY()	const	{ return g.y(); }
    ButtonState button() const	{ return (ButtonState) b; }
//    ButtonState state()	const	{ return (ButtonState) s; }
    ButtonState state()	const	{ return (ButtonState) m_nButtonState; }
    ButtonState stateAfter() const { return( (ButtonState)m_nButtonState ); }
protected:
    QPoint p;
    QPoint g;
    int	   m_nButtonState;
    int	   b; // ### Make ushort in 3.0? Here it's an int...
    ushort s; // ### ...and here an ushort. But both are ButtonState!
};

class Q_EXPORT QKeyEvent : public QEvent
{
public:

    QKeyEvent( Type type, int key, int ascii, int state,
		const QString& text=QString::null, bool autorep=FALSE, ushort count=1 )
	: QEvent(type), txt(text), k((ushort)key), s((ushort)state),
	    a((uchar)ascii), accpt(TRUE), autor(autorep), c(count) {}

    int	   key()	const	{ return k; }
      /*
    int	   ascii()	const	{ return a; }
    */    
    ButtonState state()	const	{ return ButtonState(s); }
      /*
    ButtonState stateAfter() const;
    bool   isAccepted() const	{ return accpt; }
    QString text()      const   { return txt; }
    bool   isAutoRepeat() const	{ return autor; }
    int   count() const	{ return int(c); }
    */
    void   accept()		{ accpt = TRUE; }
/*    
    void   ignore()		{ accpt = FALSE; }
    */
protected:
    QString txt;
    ushort k, s;
    uchar  a;
    uint   accpt:1;
    uint   autor:1;
    ushort c;
};


class QCustomEvent : public QEvent
{
public:
//    QCustomEvent( int type );
    QCustomEvent( Type type, void *data )
	: QEvent(type), d(data) {};
    void       *data()	const	{ return d; }
    void	setData( void* data )	{ d = data; }
private:
    void       *d;
};
