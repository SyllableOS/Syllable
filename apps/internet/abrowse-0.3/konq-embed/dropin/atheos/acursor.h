
#include <qpoint.h>

enum QCursorShape {
    ArrowCursor, UpArrowCursor, CrossCursor, WaitCursor, IbeamCursor,
    SizeVerCursor, SizeHorCursor, SizeBDiagCursor, SizeFDiagCursor,
    SizeAllCursor, BlankCursor, SplitVCursor, SplitHCursor, PointingHandCursor,
    ForbiddenCursor, LastCursor = ForbiddenCursor, BitmapCursor=24 };


class QCursor
{
public:
    QCursor();
    QCursor( QCursorShape );

    static QPoint pos();
};


