#ifndef __F_AWIDGET_H__
#define __F_AWIDGET_H__

#include <gui/view.h>
#include <qobject.h>
#include <qpoint.h>
#include <qsize.h>
#include <qpalette.h>
#include <qpaintdevice.h>
#include <qnamespace.h>

class QWidgetPrivate;

class QPaintEvent;
class QFocusEvent;
class QStyle;

class QWidget : public QObject, public os::View, public QPaintDevice
{
    Q_OBJECT
public:
    enum BackgroundMode { NoBackground };

    enum FocusPolicy {
	NoFocus = 0,
	TabFocus = 0x1,
	ClickFocus = 0x2,
	StrongFocus = 0x3,
	WheelFocus = 0x7
    };

    
    QWidget( QWidget* pcParent, const char* pzName );
    virtual ~QWidget();

    virtual void focusInEvent( QFocusEvent * );
    virtual void focusOutEvent( QFocusEvent * );
    
    virtual void setEnabled( bool bEnabled );
    virtual void setAutoMask(bool);
    
    void clearFocus();
    
    void show();
    void hide();

    virtual void setFocusProxy( QWidget * );
    virtual bool focusNextPrevChild( bool next );
    QWidget* focusWidget() const;
    
    virtual int	 metric( int id ) const;
    
    QPoint mapToGlobal( const QPoint & ) const;
    QPoint mapFromGlobal( const QPoint& );

    virtual QSize sizeHint() const;

    void		repaint();

    virtual void setFont( const QFont & );
    QFont		font() const;

    void 		constPolish() const;

    QStyle     &style() const;
    
    virtual void resize( int /*w*/, int /*h*/ );
    
    int x();
    int y();

    QPoint	 pos()		const;
    void		move( const QPoint & );
    QSize	 size()		const;
    void		resize( const QSize & );
    
    int width();
    int height();


    void erase();

//    const QPalette& palette() const;
    
    QWidget* topLevelWidget();
    void setFocus();
    bool isTopLevel() const;
    bool isEnabled() const;


    void setMouseTracking( bool );

    void setBackgroundMode( BackgroundMode );

    void setCursor( const QCursor& );
    
    virtual void setFocusPolicy( FocusPolicy );
    virtual void setAcceptDrops( bool /*on*/ );

    virtual os::Point GetPreferredSize( bool bLargest ) const;
protected:
//    virtual void paintEvent( QPaintEvent * );
    
private:
    QWidgetPrivate* d;

//    QPalette m_cPalette;
};


#endif // __F_AWIDGET_H__
