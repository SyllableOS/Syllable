

#include <qnamespace.h>
#include <qfont.h>
#include <qpen.h>
#include <qfontmetrics.h>
#include "qpointarray.h"
#include "qbrush.h"
#include "qpen.h"

class QPixmap;
class QWidget;
class QBrush;

class QPainterPrivate;

class QPainter : public Qt
{
public:
    QPainter();
    QPainter( QWidget* );
    QPainter( QPixmap* );
    ~QPainter();

    
    QFontMetrics fontMetrics()	const;

    void	setFont( const QFont & );
    const QFont &font() const;

    bool hasClipping() const;
    void setClipping( bool );
    
    void	setPen( const QColor & );
    void	setPen( PenStyle );
    void	setPen( const QPen & );

    void	setBrush( const QColor & );
    void	setBrush( BrushStyle );
    void	setBrush( const QBrush & );
    
    const QPen &pen() const;
    const QBrush &brush() const;
    void	setRasterOp( RasterOp );
    RasterOp	rasterOp() const;

    void	save();
    void	restore();
    
    void begin( QPixmap* );
    void end();
    QPaintDevice *device() const;
    void	setClipRect( const QRect & );	// set clip rectangle
    void	scale( double sx, double sy );
    
    void translate(int,int);

    void drawText( int /*x*/, int /*y*/, const QString &, int len = -1 );
    void drawText( int /*x*/, int /*y*/, int /*w*/, int /*h*/, int /*flags*/,
		   const QString&, int /*len*/ = -1, QRect */*br*/=0,
		   char **/*internal*/=0 );
    void drawLine( int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/ );
    void drawRect( int /*x*/, int /*y*/, int /*w*/, int /*h*/ );

    void drawEllipse( int x, int y, int w, int h );
    void drawArc( int x, int y, int w, int h, int a, int alen );

    void drawLineSegments( const QPointArray &,
			   int index=0, int nlines=-1 );
    void drawPolyline( const QPointArray &,
		       int index=0, int npoints=-1 );
    void drawPolygon( const QPointArray &, bool winding=false, int index=0, int npoints=-1 );
    
    void drawPixmap (int x, int y, const QPixmap &, int sx, int sy, int sw, int sh);
    void drawPixmap( const QPoint &, const QPixmap & );
    void drawPixmap( const QPoint &, const QPixmap &, const QRect &sr );
    
    void fillRect( int /*x*/, int /*y*/, int /*w*/, int /*h*/, const QColor & );

    void drawTiledPixmap( int /*x*/, int /*y*/, int /*w*/, int /*h*/, const QPixmap &, int sx=0, int sy=0 );
    
private:
    QPainterPrivate* d;
};
