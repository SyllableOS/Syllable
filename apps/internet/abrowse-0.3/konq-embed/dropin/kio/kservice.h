#ifndef __kservice_h__
#define __kservice_h__

#include <ksharedptr.h>
#include <qvaluelist.h>
#include <qstringlist.h>

class KService : public KShared
{
public:
    typedef KSharedPtr<KService> Ptr;
    typedef QValueList<KService> List;

    KService( const QString &name = QString::null,
              const QString &library = QString::null,
              const QStringList &serviceTypes = QStringList() )
        { m_name = name; m_library = library; m_serviceTypes = serviceTypes; }

    QString name() { return m_name; }

    QString library() { return m_library; }

    QStringList serviceTypes() { return m_serviceTypes; }

private:
    QString m_name;
    QString m_library;
    QStringList m_serviceTypes;
};

#endif
