#ifndef __kdialog_h__
#define __kdialog_h__

#include <qdialog.h>

class KDialog : public QDialog
{
    Q_OBJECT
public:
    KDialog( QWidget *parent, const char *name, bool modal = false, WFlags f = 0 );
    virtual ~KDialog();

    static int marginHint() { return 12; }
    static int spacingHint() { return 6; }
};

#endif
