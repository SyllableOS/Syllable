#include "qcolor.h"
#include "qshared.h"


class QPen: public Qt
{
public:
    QPen();
    QPen( const QColor &/*color*/, uint /*width*/=0, PenStyle /*style*/=SolidLine );

    const QColor &color() const;

private:
    QColor m_color;
};
