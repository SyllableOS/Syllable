
#include <qstring.h>

class QVariant
{
public:
    enum Type {
	Invalid,
	Map,
	List,
	String,
	StringList,
	Font,
	Pixmap,
	Brush,
	Rect,
	Size,
	Color,
	Palette,
	ColorGroup,
	IconSet,
	Point,
	Image,
	Int,
	UInt,
	Bool,
	Double,
	CString,
	PointArray,
	Region,
	Bitmap,
	Cursor,
	SizePolicy
    };

    QVariant();
    QVariant( const QString& cValue );
    QVariant( bool bValue, int dumy );
    QVariant( double vValue );

    Type type() const;
    bool toBool() const;
    
private:
    class Private;
    Private* d;
    
};
