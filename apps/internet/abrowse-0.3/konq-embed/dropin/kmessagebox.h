#ifndef __kmessagebox_h__
#define __kmessagebox_h__

#ifndef __ATHEOS__    
#include <qmessagebox.h>

class KMessageBox
{
public:
    // ### evil hack -> No == Cancel
    enum Answer { Ok = 1, Cancel = 4, Yes = 3, No = 4, Continue = 5 };

    static void error( QWidget *parent,
                       const QString &text,
                       const QString &caption = QString::null )
        { information( parent, text, caption ); }

    static void sorry( QWidget *parent,
                       const QString &text,
                       const QString &caption = QString::null )
        { information( parent, text, caption ); }

    static int questionYesNo( QWidget *parent,
                              const QString &text,
                              const QString &caption = QString::null,
                              const QString &buttonYes = QString::null,
                              const QString &buttonNo = QString::null,
                              bool okButton = false );

    static void information( QWidget *parent,
                             const QString &text,
                             const QString &caption = QString::null )
        { questionYesNo( parent, text, caption, QString::null, QString::null, true ); }

    static int warningYesNo( QWidget *parent,
                             const QString &text,
                             const QString &caption = QString::null,
                             const QString &buttonYes = QString::null,
                             const QString &buttonNo = QString::null )
        { return questionYesNo( parent, text, caption, buttonYes, buttonNo ); }
};
#endif // __ATHEOS__    


#endif
