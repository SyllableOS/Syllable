#ifndef __kxmlguiclient_h__
#define __kxmlguiclient_h__

#include <qstring.h>
#include <qlist.h>
#include <qdom.h>

#include <kaction.h>

class KXMLGUIClient
{
public:
    KXMLGUIClient() {}
    virtual ~KXMLGUIClient() {}

    void setXMLFile( const QString & ) {}

#ifndef __ATHEOS__
    KActionCollection *actionCollection() { return &m_collection; }
  
    void plugActionList( const QString &, const QList<KAction> & ) {}
#endif
    void unplugActionList( const QString & ) {}

    void setInstance( KInstance * ) {}

    void setXML( const QString & ) {}

    void setDOMDocument( const QDomDocument & ) {}

    QDomDocument domDocument() { return QDomDocument(); }

private:
#ifndef __ATHEOS__
    KActionCollection m_collection;
#endif
};

#endif
