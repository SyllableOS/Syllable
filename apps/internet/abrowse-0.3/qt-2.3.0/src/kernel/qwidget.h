/****************************************************************************
** $Id$
**
** Definition of QWidget class
**
** Created : 931029
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/


#ifndef QWIDGET_H
#define QWIDGET_H

#ifdef __ATHEOS__
#include "../../konq-embed/dropin/atheos/awidget.h"
#else

#ifndef QT_H
#include "qwindowdefs.h"
#include "qobject.h"
#include "qpaintdevice.h"
#include "qpalette.h"
#include "qcursor.h"
#include "qfont.h"
#include "qfontmetrics.h"
#include "qfontinfo.h"
#include "qstyle.h"
#include "qsizepolicy.h"
#endif // QT_H

class QLayout;
struct QWExtra;
struct QTLWExtra;
class QFocusData;
class QStyle;
class QWSRegionManager;

class Q_EXPORT QWidget : public QObject, public QPaintDevice
{
    Q_OBJECT
    Q_ENUMS( BackgroundMode PropagationMode FocusPolicy BackgroundOrigin )
    Q_PROPERTY( bool isTopLevel READ isTopLevel )
    Q_PROPERTY( bool isModal READ isModal )
    Q_PROPERTY( bool isPopup READ isPopup )
    Q_PROPERTY( bool isDesktop READ isDesktop )
    Q_PROPERTY( bool enabled READ isEnabled WRITE setEnabled )
    Q_PROPERTY( QRect geometry READ geometry WRITE setGeometry )
    Q_PROPERTY( QRect frameGeometry READ frameGeometry )
    Q_PROPERTY( int x READ x )
    Q_PROPERTY( int y READ y )
    Q_PROPERTY( QPoint pos READ pos )
    Q_PROPERTY( QSize frameSize READ frameSize )
    Q_PROPERTY( QSize size READ size WRITE resize DESIGNABLE false )
    Q_PROPERTY( int width READ width )
    Q_PROPERTY( int height READ height )
    Q_PROPERTY( QRect rect READ rect )
    Q_PROPERTY( QRect childrenRect READ childrenRect )
    Q_PROPERTY( QRegion childrenRegion READ childrenRegion )
    Q_PROPERTY( QSizePolicy sizePolicy READ sizePolicy WRITE setSizePolicy )
    Q_PROPERTY( QSize minimumSize READ minimumSize WRITE setMinimumSize )
    Q_PROPERTY( QSize maximumSize READ maximumSize WRITE setMaximumSize )
    Q_PROPERTY( int minimumWidth READ minimumWidth WRITE setMinimumWidth STORED false )
    Q_PROPERTY( int minimumHeight READ minimumHeight WRITE setMinimumHeight STORED false )
    Q_PROPERTY( int maximumWidth READ maximumWidth WRITE setMaximumWidth STORED false )
    Q_PROPERTY( int maximumHeight READ maximumHeight WRITE setMaximumHeight STORED false )
    Q_PROPERTY( QSize sizeIncrement READ sizeIncrement WRITE setSizeIncrement )
    Q_PROPERTY( QSize baseSize READ baseSize WRITE setBaseSize )
    Q_PROPERTY( BackgroundMode backgroundMode READ backgroundMode WRITE setBackgroundMode DESIGNABLE false )
    Q_PROPERTY( QColor backgroundColor READ backgroundColor WRITE setBackgroundColor DESIGNABLE false )
    Q_PROPERTY( QColor foregroundColor READ foregroundColor )
    Q_PROPERTY( QPixmap backgroundPixmap READ backgroundPixmap WRITE setBackgroundPixmap DESIGNABLE false )
    Q_PROPERTY( QColorGroup colorGroup READ colorGroup )
    Q_PROPERTY( QPalette palette READ palette WRITE setPalette RESET unsetPalette )
    Q_PROPERTY( bool ownPalette READ ownPalette )
    Q_PROPERTY( QFont font READ font WRITE setFont RESET unsetFont )
    Q_PROPERTY( bool ownFont READ ownFont )
#ifndef QT_NO_CURSOR
    Q_PROPERTY( QCursor cursor READ cursor WRITE setCursor RESET unsetCursor )
    Q_PROPERTY( bool ownCursor READ ownCursor )
#endif
    Q_PROPERTY( QString caption READ caption WRITE setCaption )
    Q_PROPERTY( QPixmap icon READ icon WRITE setIcon )
    Q_PROPERTY( QString iconText READ iconText WRITE setIconText )
    Q_PROPERTY( bool mouseTracking READ hasMouseTracking WRITE setMouseTracking )
    Q_PROPERTY( bool isActiveWindow READ isActiveWindow )
    Q_PROPERTY( bool focusEnabled READ isFocusEnabled )
    Q_PROPERTY( FocusPolicy focusPolicy READ focusPolicy WRITE setFocusPolicy )
    Q_PROPERTY( bool focus READ hasFocus )
    Q_PROPERTY( bool updatesEnabled READ isUpdatesEnabled WRITE setUpdatesEnabled DESIGNABLE false )
    Q_PROPERTY( bool visible READ isVisible )
    Q_PROPERTY( QRect visibleRect READ visibleRect )
    Q_PROPERTY( bool hidden READ isHidden )
    Q_PROPERTY( bool minimized READ isMinimized )
    Q_PROPERTY( QSize sizeHint READ sizeHint )
    Q_PROPERTY( QSize minimumSizeHint READ minimumSizeHint )
    Q_PROPERTY( QRect microFocusHint READ microFocusHint )
    Q_PROPERTY( bool acceptDrops READ acceptDrops WRITE setAcceptDrops )
    Q_PROPERTY( bool autoMask READ autoMask WRITE setAutoMask )
    Q_PROPERTY( BackgroundOrigin backgroundOrigin READ backgroundOrigin WRITE setBackgroundOrigin )
    Q_PROPERTY( bool customWhatsThis READ customWhatsThis )

public:
    QWidget( QWidget *parent=0, const char *name=0, WFlags f=0 );
    ~QWidget();

    WId		 winId() const;
    void	 setName( const char *name );
#ifndef QT_NO_STYLE
    // GUI style setting

    QStyle     &style() const;
    void        setStyle( QStyle * );
#endif
    // Widget types and states

    bool	 isTopLevel()	const;
    bool	 isModal()	const;
    bool	 isPopup()	const;
    bool	 isDesktop()	const;

    bool	 isEnabled()	const;
    bool	 isEnabledTo(QWidget*) const;
    bool	 isEnabledToTLW() const;

public slots:
    virtual void setEnabled( bool );
    void setDisabled( bool );

    // Widget coordinates

public:
    QRect	 frameGeometry() const;
    const QRect &geometry()	const;
    int		 x()		const;
    int		 y()		const;
    QPoint	 pos()		const;
    QSize	 frameSize()    const;
    QSize	 size()		const;
    int		 width()	const;
    int		 height()	const;
    QRect	 rect()		const;
    QRect	 childrenRect() const;
    QRegion	 childrenRegion() const;

    QSize	 minimumSize()	 const;
    QSize	 maximumSize()	 const;
    int		 minimumWidth()	 const;
    int		 minimumHeight() const;
    int		 maximumWidth()	 const;
    int		 maximumHeight() const;
    void	 setMinimumSize( const QSize & );
    virtual void setMinimumSize( int minw, int minh );
    void	 setMaximumSize( const QSize & );
    virtual void setMaximumSize( int maxw, int maxh );
    void	 setMinimumWidth( int minw );
    void	 setMinimumHeight( int minh );
    void	 setMaximumWidth( int maxw );
    void	 setMaximumHeight( int maxh );

    QSize	 sizeIncrement() const;
    void	 setSizeIncrement( const QSize & );
    virtual void setSizeIncrement( int w, int h );
    QSize	 baseSize() const;
    void	 setBaseSize( const QSize & );
    void	 setBaseSize( int basew, int baseh );

    void	setFixedSize( const QSize & );
    void	setFixedSize( int w, int h );
    void	setFixedWidth( int w );
    void	setFixedHeight( int h );

    // Widget coordinate mapping

    QPoint	 mapToGlobal( const QPoint & )	 const;
    QPoint	 mapFromGlobal( const QPoint & ) const;
    QPoint	 mapToParent( const QPoint & )	 const;
    QPoint	 mapFromParent( const QPoint & ) const;
    QPoint	 mapTo( QWidget *, const QPoint & ) const;
    QPoint	 mapFrom( QWidget *, const QPoint & ) const;

    QWidget	*topLevelWidget()   const;

    // Widget attribute functions

    enum BackgroundMode { FixedColor, FixedPixmap, NoBackground,
			  PaletteForeground, PaletteButton, PaletteLight,
			  PaletteMidlight, PaletteDark, PaletteMid,
			  PaletteText, PaletteBrightText, PaletteBase,
			  PaletteBackground, PaletteShadow, PaletteHighlight,
			  PaletteHighlightedText, PaletteButtonText,
			  X11ParentRelative };

    BackgroundMode	backgroundMode() const;
    virtual void	setBackgroundMode( BackgroundMode );

    const QColor &	backgroundColor() const;
    const QColor &	foregroundColor() const;
    virtual void	setBackgroundColor( const QColor & );

    const QPixmap *	backgroundPixmap() const;
    virtual void	setBackgroundPixmap( const QPixmap & );

#ifndef QT_NO_PALETTE
    const QColorGroup & colorGroup() const;
    const QPalette &	palette()    const;
    bool		ownPalette() const;
    virtual void	setPalette( const QPalette & );
    void		unsetPalette();
#endif
    QFont		font() const;
    bool		ownFont() const;
    virtual void	setFont( const QFont & );
    void		unsetFont();
    QFontMetrics	fontMetrics() const;
    QFontInfo	 	fontInfo() const;

    enum PropagationMode { NoChildren, AllChildren,
			   SameFont, SamePalette = SameFont };

    PropagationMode	fontPropagation() const; // obsolete, remove 3.0
    virtual void	setFontPropagation( PropagationMode ); // obsolete, remove 3.0

    PropagationMode	palettePropagation() const; // obsolete, remove 3.0
    virtual void	setPalettePropagation( PropagationMode ); // obsolete, remove 3.0
#ifndef QT_NO_CURSOR
    const QCursor      &cursor() const;
    bool		ownCursor() const;
    virtual void	setCursor( const QCursor & );
    virtual void	unsetCursor();
#endif
    QString		caption() const;
    const QPixmap      *icon() const;
    QString		iconText() const;
    bool		hasMouseTracking() const;

    virtual void	setMask( const QBitmap & );
    virtual void	setMask( const QRegion & );
    void		clearMask();

public slots:
    virtual void	setCaption( const QString &);
    virtual void	setIcon( const QPixmap & );
    virtual void	setIconText( const QString &);
    virtual void	setMouseTracking( bool enable );

    // Keyboard input focus functions

    virtual void	setFocus();
    void		clearFocus();

public:
    enum FocusPolicy {
	NoFocus = 0,
	TabFocus = 0x1,
	ClickFocus = 0x2,
	StrongFocus = 0x3,
	WheelFocus = 0x7
    };

    bool		isActiveWindow() const;
    virtual void	setActiveWindow();
    bool		isFocusEnabled() const;

    FocusPolicy		focusPolicy() const;
    virtual void	setFocusPolicy( FocusPolicy );
    bool		hasFocus() const;
    static void		setTabOrder( QWidget *, QWidget * );
    virtual void	setFocusProxy( QWidget * );
    QWidget *		focusProxy() const;

    // Grab functions

    void		grabMouse();
#ifndef QT_NO_CURSOR
    void		grabMouse( const QCursor & );
#endif
    void		releaseMouse();
    void		grabKeyboard();
    void		releaseKeyboard();
    static QWidget *	mouseGrabber();
    static QWidget *	keyboardGrabber();

    // Update/refresh functions

    bool	 	isUpdatesEnabled() const;

#if 0 //def _WS_QWS_
    void		repaintUnclipped( const QRegion &, bool erase = TRUE );
#endif
public slots:
    virtual void	setUpdatesEnabled( bool enable );
    void		update();
    void		update( int x, int y, int w, int h );
    void		update( const QRect& );
    void		repaint();
    void		repaint( bool erase );
    void		repaint( int x, int y, int w, int h, bool erase=TRUE );
    void		repaint( const QRect &, bool erase=TRUE );
    void		repaint( const QRegion &, bool erase=TRUE );

    // Widget management functions

    virtual void	show();
    virtual void	hide();
#ifndef QT_NO_COMPAT
    void		iconify()	{ showMinimized(); }
#endif
    virtual void	showMinimized();
    virtual void	showMaximized();
    void		showFullScreen(); // virtual 3.0
    virtual void	showNormal();
    virtual void	polish();
    void 		constPolish() const;
    bool		close();

    void		raise();
    void		lower();
    void		stackUnder( QWidget* );
    virtual void	move( int x, int y );
    void		move( const QPoint & );
    virtual void	resize( int w, int h );
    void		resize( const QSize & );
    virtual void	setGeometry( int x, int y, int w, int h );
    virtual void	setGeometry( const QRect & );

public:
    virtual bool	close( bool alsoDelete );
    bool		isVisible()	const;
    bool		isVisibleTo(QWidget*) const;
    bool		isVisibleToTLW() const; // obsolete
    QRect		visibleRect() const;
    bool 		isHidden() const;
    bool		isMinimized() const;
    bool		isMaximized() const;

    virtual QSize	sizeHint() const;
    virtual QSize	minimumSizeHint() const;
    virtual QSizePolicy	sizePolicy() const;
    void 		setSizePolicy( QSizePolicy );
    virtual int heightForWidth(int) const;

    virtual void  	adjustSize();
#ifndef QT_NO_LAYOUT
    QLayout *		layout() const { return lay_out; }
#endif
    void		updateGeometry();
    virtual void 	reparent( QWidget *parent, WFlags, const QPoint &,
				  bool showIt=FALSE );
    void		reparent( QWidget *parent, const QPoint &,
				  bool showIt=FALSE );
#ifndef QT_NO_COMPAT
    void		recreate( QWidget *parent, WFlags f, const QPoint & p,
				  bool showIt=FALSE )
    { reparent(parent,f,p,showIt); }
#endif

    void		erase();
    void		erase( int x, int y, int w, int h );
    void		erase( const QRect & );
    void		erase( const QRegion & );
    void		scroll( int dx, int dy );
    void		scroll( int dx, int dy, const QRect& );

    void		drawText( int x, int y, const QString &);
    void		drawText( const QPoint &, const QString &);

    // Misc. functions

    QWidget *		focusWidget() const;
    QRect               microFocusHint() const;

    // drag and drop

    bool		acceptDrops() const;
    virtual void	setAcceptDrops( bool on );

    // transparency and pseudo transparency

    virtual void	setAutoMask(bool);
    bool		autoMask() const;

    enum BackgroundOrigin { WidgetOrigin, ParentOrigin };

    void setBackgroundOrigin( BackgroundOrigin );
    BackgroundOrigin backgroundOrigin() const;


    // whats this help
    virtual bool customWhatsThis() const;

    QWidget *		parentWidget() const;
    bool		testWState( uint n ) const;
    bool		testWFlags( WFlags f ) const;
    static QWidget *	find( WId );
    static QWidgetMapper *wmapper();

#if defined(_WS_QWS_)
    virtual QGfx * graphicsContext(bool clip_children=TRUE) const;
#endif

protected:
    // Event handlers
    bool	 event( QEvent * );
    virtual void mousePressEvent( QMouseEvent * );
    virtual void mouseReleaseEvent( QMouseEvent * );
    virtual void mouseDoubleClickEvent( QMouseEvent * );
    virtual void mouseMoveEvent( QMouseEvent * );
    virtual void wheelEvent( QWheelEvent * );
    virtual void keyPressEvent( QKeyEvent * );
    virtual void keyReleaseEvent( QKeyEvent * );
    virtual void focusInEvent( QFocusEvent * );
    virtual void focusOutEvent( QFocusEvent * );
    virtual void enterEvent( QEvent * );
    virtual void leaveEvent( QEvent * );
    virtual void paintEvent( QPaintEvent * );
    virtual void moveEvent( QMoveEvent * );
    virtual void resizeEvent( QResizeEvent * );
    virtual void closeEvent( QCloseEvent * );

#ifndef QT_NO_DRAGANDDROP
    virtual void dragEnterEvent( QDragEnterEvent * );
    virtual void dragMoveEvent( QDragMoveEvent * );
    virtual void dragLeaveEvent( QDragLeaveEvent * );
    virtual void dropEvent( QDropEvent * );
#endif

    virtual void showEvent( QShowEvent * );
    virtual void hideEvent( QHideEvent * );
    virtual void customEvent( QCustomEvent * );

#if defined(_WS_MAC_)
    virtual bool macEvent( MSG * );		// Macintosh event
    QWidget * mytop;                            // This widget's top-level widg
    int back_type;                              // Type of background
    QPixmap * bg_pix;
    virtual void fixport();
    virtual void propagateUpdates(int x,int y,int x2,int y2);
#elif defined(_WS_WIN_)
    virtual bool winEvent( MSG * );		// Windows event
#elif defined(_WS_X11_)
    virtual bool x11Event( XEvent * );		// X11 event
#elif defined(_WS_QWS_)
    virtual bool qwsEvent( QWSEvent * );
    virtual unsigned char * scanLine(int) const;
    virtual int bytesPerLine() const;
#endif

    virtual void updateMask();

    // Misc. protected functions

protected:
#ifndef QT_NO_STYLE
    virtual void styleChange( QStyle& );
#endif
    virtual void enabledChange( bool );
    virtual void backgroundColorChange( const QColor & );
    virtual void backgroundPixmapChange( const QPixmap & );
#ifndef QT_NO_PALETTE
    virtual void paletteChange( const QPalette & );
#endif
    virtual void fontChange( const QFont & );

    int		 metric( int )	const;

    virtual void create( WId = 0, bool initializeWindow = TRUE,
			 bool destroyOldWindow = TRUE );
    virtual void destroy( bool destroyWindow = TRUE,
			  bool destroySubWindows = TRUE );
    uint	 getWState() const;
    virtual void setWState( uint );
    void	 clearWState( uint n );
    WFlags	 getWFlags() const;
    virtual void setWFlags( WFlags );
    void	 clearWFlags( WFlags n );

    virtual void setFRect( const QRect & );
    virtual void setCRect( const QRect & );

    virtual bool focusNextPrevChild( bool next );

    QWExtra	*extraData();
    QTLWExtra	*topData();
    QFocusData	*focusData();

    virtual void setKeyCompression(bool);
    virtual void setMicroFocusHint(int x, int y, int w, int h, bool text=TRUE);

private slots:
    void	 focusProxyDestroyed();

private:
    void	 setFontSys();
#ifndef QT_NO_LAYOUT
    void 	 setLayout( QLayout *l );
#endif
    void	 setWinId( WId );
    void	 showWindow();
    void	 hideWindow();
    void	 sendShowEventsToChildren( bool spontaneous );
    void	 sendHideEventsToChildren( bool spontaneous );
    void	 createTLExtra();
    void	 createExtra();
    void	 deleteExtra();
    void	 createSysExtra();
    void	 deleteSysExtra();
    void	 createTLSysExtra();
    void	 deleteTLSysExtra();
    void	 deactivateWidgetCleanup();
    void	 internalSetGeometry( int, int, int, int, bool );
    void	 reparentFocusWidgets( QWidget * );
    QFocusData	*focusData( bool create );
    void         setBackgroundFromMode();
    void         setBackgroundColorDirect( const QColor & );
    void   	 setBackgroundPixmapDirect( const QPixmap & );
    void         setBackgroundModeDirect( BackgroundMode );
    void         setBackgroundEmpty();
#if defined(_WS_X11_)
    void         setBackgroundX11Relative();
#endif

    WId		 winid;
    uint	 widget_state;
    uint	 widget_flags;
    uint	 propagate_font : 2; // obsolete
    uint	 propagate_palette : 2; // obsolete
    uint	 focus_policy : 4;
    uint 	 own_font :1;
    uint 	 own_palette :1;
    uint 	 sizehint_forced :1;
    uint 	 is_closing :1;
    uint 	 in_show : 1;
    QPoint	 fpos;
    QRect	 crect;
    QColor	 bg_col;
#ifndef QT_NO_PALETTE
    QPalette	 pal;
#endif
    QFont	 fnt;
#ifndef QT_NO_LAYOUT
    QLayout 	*lay_out;
#endif
    QWExtra	*extra;
#if defined(_WS_QWS_)
    QRegion	 req_region;			// Requested region
    mutable QRegion	 paintable_region;	// Paintable region
    mutable bool         paintable_region_dirty;// needs to be recalculated
    mutable QRegion      alloc_region;          // Allocated region
    mutable bool         alloc_region_dirty;    // needs to be recalculated
    mutable int          overlapping_children;  // Handle overlapping children

    int		 alloc_region_index;
    int		 alloc_region_revision;

    void updateOverlappingChildren() const;
    void setChildrenAllocatedDirty();
    bool isAllocatedRegionDirty() const;
    QRegion requestedRegion() const;
    QRegion allocatedRegion() const;
    QRegion paintableRegion() const;

    // used to accumulate dirty region when children moved/resized.
    QRegion dirtyChildren;
    bool isSettingGeometry;
    friend class QWSManager;
#endif

    static void	 createMapper();
    static void	 destroyMapper();
    static QWidgetList	 *wList();
    static QWidgetList	 *tlwList();
    static QWidgetMapper *mapper;
    friend class QApplication;
    friend class QBaseApplication;
    friend class QPainter;
    friend class QFontMetrics;
    friend class QFontInfo;
    friend class QETWidget;
#if 1 //was #ifdef TOTAL_LOSER_COMPILER but they all seem to be
    friend class QLayout;
#else
    friend void QLayout::setWidgetLayout( QWidget *, QLayout * );
#endif
private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QWidget( const QWidget & );
    QWidget &operator=( const QWidget & );
#endif

public: // obsolete functions to dissappear or to become inline in 3.0
#ifndef QT_NO_PALETTE
    void setPalette( const QPalette &, bool iReallyMeanIt );
#endif
    void setFont( const QFont &, bool iReallyMeanIt );
};


inline bool QWidget::testWState( uint f ) const
{ return (widget_state & f) != 0; }

inline bool QWidget::testWFlags( WFlags f ) const
{ return (widget_flags & f) != 0; }


inline WId QWidget::winId() const
{ return winid; }

inline bool QWidget::isTopLevel() const
{ return testWFlags(WType_TopLevel); }

inline bool QWidget::isModal() const
{ return testWFlags(WType_Modal); }

inline bool QWidget::isPopup() const
{ return testWFlags(WType_Popup); }

inline bool QWidget::isDesktop() const
{ return testWFlags(WType_Desktop); }

inline bool QWidget::isEnabled() const
{ return !testWState(WState_Disabled); }

inline const QRect &QWidget::geometry() const
{ return crect; }

inline int QWidget::x() const
{ return fpos.x(); }

inline int QWidget::y() const
{ return fpos.y(); }

inline QPoint QWidget::pos() const
{ return fpos; }

inline QSize QWidget::size() const
{ return crect.size(); }

inline int QWidget::width() const
{ return crect.width(); }

inline int QWidget::height() const
{ return crect.height(); }

inline QRect QWidget::rect() const
{ return QRect(0,0,crect.width(),crect.height()); }

inline int QWidget::minimumWidth() const
{ return minimumSize().width(); }

inline int QWidget::minimumHeight() const
{ return minimumSize().height(); }

inline int QWidget::maximumWidth() const
{ return maximumSize().width(); }

inline int QWidget::maximumHeight() const
{ return maximumSize().height(); }

inline void QWidget::setMinimumSize( const QSize &s )
{ setMinimumSize(s.width(),s.height()); }

inline void QWidget::setMaximumSize( const QSize &s )
{ setMaximumSize(s.width(),s.height()); }

inline void QWidget::setSizeIncrement( const QSize &s )
{ setSizeIncrement(s.width(),s.height()); }

inline void QWidget::setBaseSize( const QSize &s )
{ setBaseSize(s.width(),s.height()); }

inline const QColor &QWidget::backgroundColor() const
{ return bg_col; }

#ifndef QT_NO_PALETTE
inline const QPalette &QWidget::palette() const
{ return pal; }
#endif

inline QFont QWidget::font() const
{ return fnt; }

inline QFontMetrics QWidget::fontMetrics() const
{ return QFontMetrics(font()); }

inline QFontInfo QWidget::fontInfo() const
{ return QFontInfo(font()); }

inline bool QWidget::hasMouseTracking() const
{ return testWState(WState_MouseTracking); }

inline bool  QWidget::isFocusEnabled() const
{ return (FocusPolicy)focus_policy != NoFocus; }

inline QWidget::FocusPolicy QWidget::focusPolicy() const
{ return (FocusPolicy)focus_policy; }

inline bool QWidget::isUpdatesEnabled() const
{ return !testWState(WState_BlockUpdates); }

inline void QWidget::update( const QRect &r )
{ update( r.x(), r.y(), r.width(), r.height() ); }

inline void QWidget::repaint()
{ repaint( 0, 0, crect.width(), crect.height(), TRUE ); }

inline void QWidget::repaint( bool erase )
{ repaint( 0, 0, crect.width(), crect.height(), erase ); }

inline void QWidget::repaint( const QRect &r, bool erase )
{ repaint( r.x(), r.y(), r.width(), r.height(), erase ); }

inline void QWidget::erase()
{ erase( 0, 0, crect.width(), crect.height() ); }

inline void QWidget::erase( const QRect &r )
{ erase( r.x(), r.y(), r.width(), r.height() ); }

inline bool QWidget::close()
{ return close( FALSE ); }

inline bool QWidget::isVisible() const
{ return testWState(WState_Visible); }

inline bool QWidget::isHidden() const
{ return testWState(WState_ForceHide); }

inline void QWidget::move( const QPoint &p )
{ move( p.x(), p.y() ); }

inline void QWidget::resize( const QSize &s )
{ resize( s.width(), s.height()); }

inline void QWidget::setGeometry( const QRect &r )
{ setGeometry( r.left(), r.top(), r.width(), r.height() ); }

inline void QWidget::drawText( const QPoint &p, const QString &s )
{ drawText( p.x(), p.y(), s ); }

inline QWidget *QWidget::parentWidget() const
{ return (QWidget *)QObject::parent(); }

inline QWidgetMapper *QWidget::wmapper()
{ return mapper; }

inline uint QWidget::getWState() const
{ return widget_state; }

inline void QWidget::setWState( uint f )
{ widget_state |= f; }

inline void QWidget::clearWState( uint f )
{ widget_state &= ~f; }

inline Qt::WFlags QWidget::getWFlags() const
{ return widget_flags; }

inline void QWidget::setWFlags( WFlags f )
{ widget_flags |= f; }

inline void QWidget::clearWFlags( WFlags f )
{ widget_flags &= ~f; }

inline void QWidget::constPolish() const
{
    if ( !testWState(WState_Polished) ) {
	QWidget* that = (QWidget*) this;
	that->polish();
        that->setWState(WState_Polished); // be on the safe side...
    }
}
#ifndef QT_NO_CURSOR
inline bool QWidget::ownCursor() const
{
    return testWState( WState_OwnCursor );
}
#endif
inline bool QWidget::ownFont() const
{
    return own_font;
}
#ifndef QT_NO_PALETTE
inline bool QWidget::ownPalette() const
{
    return own_palette;
}
#endif

// Extra QWidget data
//  - to minimize memory usage for members that are seldom used.
//  - top-level widgets have extra extra data to reduce cost further

class QFocusData;
class QWSManager;
#if defined(_WS_WIN_)
class QOleDropTarget;
#endif

struct QTLWExtra {
    QString  caption;				// widget caption
    QString  iconText;				// widget icon text
    QPixmap *icon;				// widget icon
    QFocusData *focusData;			// focus data (for TLW)
    QSize    fsize;				// rect of frame
    short    incw, inch;			// size increments
    uint     iconic: 1;				// iconified [cur. win32 only]
    uint     fullscreen : 1;			// full-screen mode
    uint     showMode: 2;			// 0 normal, 1 minimized, 2 maximized, 3 reset
    short    basew, baseh;			// base sizes
#if defined(_WS_X11_)
    WId	     parentWinId;			// parent window Id (valid after reparenting)
    uint     embedded : 1;			// window is embedded in another Qt application
    uint     reserved: 2;			// reserved
    uint     dnd : 1; 				// DND properties installed
    uint     uspos : 1;                         // User defined position
    uint     ussize : 1;                        // User defined size
    void    *xic;				// XIM Input Context
#endif
#if defined(_WS_QWS_) && !defined ( QT_NO_QWS_MANAGER )
    QRegion decor_allocated_region;		// decoration allocated region
    QWSManager *qwsManager;
#endif
#if defined(_WS_WIN_)
    HICON    winIcon;				// internal Windows icon
#endif
    QRect    normalGeometry;			// used by showMin/maximized/FullScreen
};


#define QWIDGETSIZE_MAX 32767

// dear user: you can see this struct, but it is internal. do not touch.

struct QWExtra {
    Q_INT16  minw, minh;			// minimum size
    Q_INT16  maxw, maxh;			// maximum size
    QPixmap *bg_pix;				// background pixmap
    QWidget *focus_proxy;
#ifndef QT_NO_CURSOR
    QCursor *curs;
#endif
    QTLWExtra *topextra;			// only useful for TLWs
#if defined(_WS_WIN_)
    QOleDropTarget *dropTarget;			// drop target
#endif
#if defined(_WS_X11_)
    WId	     xDndProxy;				// XDND forwarding to embedded windows
#endif
#if defined(_WS_QWS_)
    QRegion mask;				// widget mask
#endif
    char     bg_mode;				// background mode
#ifndef QT_NO_STYLE
    QStyle* style;
#endif
    QRect micro_focus_hint;                     // micro focus hint
    QSizePolicy size_policy;
    void * posted_events;			// in qapplication
};

#endif // __ATHEOS__

#endif // QWIDGET_H
