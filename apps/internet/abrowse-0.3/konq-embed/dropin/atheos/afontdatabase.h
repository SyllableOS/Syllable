#include <qstring.h>
#include <qvaluelist.h>

class QFont;
class QFontDatabaseInternal;

class QFontDatabase
{
public:
    QFontDatabase();

    QString styleString( const QFont &);

    bool  isSmoothlyScalable( const QString &/*family*/,
			      const QString &style = QString::null,
			      const QString &charSet = QString::null ) const;

    QValueList<int> smoothSizes( const QString &/*family*/,
				 const QString &style,
				 const QString &charSet = QString::null );
private:
    QFontDatabaseInternal* d;
    QString m_style;
    QValueList<int> m_sizes;
};
