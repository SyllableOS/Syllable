#ifndef __kfiledialog_h__
#define __kfiledialog_h__

#ifndef __ATHEOS__    
#include <qfiledialog.h>

class KFileDialog
{
public:

    static QString getOpenFileName( const QString &dir,
                                    const QString &filter,
                                    QWidget *parent, const QString &caption )
        {
            return QFileDialog::getOpenFileName( dir, filter, parent, 0, caption );
        }

};
#endif // __ATHEOS__    

#endif
