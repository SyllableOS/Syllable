#ifndef __ATHEOS__

#include "kmessagebox.h"

int KMessageBox::questionYesNo( QWidget *parent,
                                const QString &text,
                                const QString &caption,
                                const QString &buttonYes,
                                const QString &buttonNo,
                                bool okButton )
{
    QMessageBox mb( caption, text, QMessageBox::NoIcon,
                    okButton ? QMessageBox::Ok : QMessageBox::Yes,
                    okButton ? QMessageBox::NoButton : QMessageBox::No,
                    QMessageBox::NoButton,
                    parent );

    if ( !buttonYes.isEmpty() )
        mb.setButtonText( QMessageBox::Yes, buttonYes );

    if ( !buttonNo.isEmpty() )
        mb.setButtonText( QMessageBox::No, buttonNo );

    if ( mb.exec() == QMessageBox::Yes )
        return KMessageBox::Yes;
    else
        return KMessageBox::No;
}
#endif
