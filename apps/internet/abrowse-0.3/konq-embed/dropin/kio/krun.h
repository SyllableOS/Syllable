#ifndef __kio_krun_h__
#define __kio_krun_h__

#include <qobject.h>
#include <kurl.h>
#include <qtimer.h>

namespace KIO
{
    class Job;
};

class KRun : public QObject
{
    Q_OBJECT
public:
    KRun( const KURL &, int, bool, bool );

    static void runURL( const KURL &, const QString & ) {}

protected:
    void scanFile() {}
    virtual void slotScanFinished( KIO::Job * ) {}

    virtual void foundMimeType( const QString & );

    KURL m_strURL;
    bool m_bFinished;
    QTimer m_timer;
    KIO::Job *m_job;

private slots:
    void slotStart();
};

#endif
