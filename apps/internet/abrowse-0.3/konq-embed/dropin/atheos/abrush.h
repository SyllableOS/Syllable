


#include "qcolor.h"
#include "qshared.h"


class QBrush : public Qt
{
public:
    QBrush();
    QBrush( const QColor &, BrushStyle=SolidPattern );
    const QColor &color() const;

private:
    QColor m_color;
};
