#include <qpaintdevice.h>
#include <qnamespace.h>
#include <qsize.h>
#include <qrect.h>

class QBitmap;
class QPixmapPrivate;

namespace os {
    class Bitmap;
    class View;
}

class QPixmap : public QPaintDevice, public Qt
{
public:
    QPixmap();
    QPixmap( const QPixmap& cOther );
    QPixmap( int, int );
    QPixmap( os::Bitmap* pcBitmap );
    ~QPixmap();
    QPixmap& operator=(const QPixmap&);

    virtual int	 metric( int id ) const;
    
    QSize size() const;
    QRect rect() const;
    int width() const;
    int height() const;
    void resize( int, int );

    bool isNull() const;

    QBitmap* mask() const;

    void SetIsTransparent( bool bTrans );
    bool IsTransparent() const;
    void ExpandValidRect( const QRect& cRect ) const;
    QRect GetValidRect() const;

    os::Bitmap* GetBitmap() const;
private:
    friend QPainter;
    os::View* GetView() const;
    QPixmapPrivate* d;
};
