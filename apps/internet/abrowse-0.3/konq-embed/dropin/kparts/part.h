#ifndef __kparts_part_h__
#define __kparts_part_h__

#include <qwidget.h>
#include <qguardedptr.h>

// ### compatibility
#include <qmap.h>
#include <qvariant.h>
#include <qobject.h>

#include <kurl.h>
#include <kparts/event.h>

// dummy
class KInstance;

#include <kxmlguiclient.h>

namespace KIO
{
    class Job;
}

namespace KParts
{
    class PartManager;

    class Part : public QObject, public KXMLGUIClient
    {
        Q_OBJECT
    public:
        Part( QObject *parent = 0, const char *name = 0 );
        virtual ~Part();

        void setInstance( KInstance *inst ) { m_instance = inst; }
        void setInstance( KInstance *inst, bool ) { m_instance = inst; }
        KInstance *instance() const { return m_instance; }

        virtual QWidget *widget();

        void setManager( PartManager *manager ) { m_manager = manager; }

        PartManager *manager() { return m_manager; }

        virtual Part *hitTest( QWidget *widget, const QPoint &globalPos );

        void setSelectable( bool selectable ) { m_selectable = selectable; }

        bool isSelectable() const { return m_selectable; }

    signals:
        void setWindowCaption( const QString &caption );

        void setStatusBarText( const QString &text );

    protected:
        void setWidget( QWidget *widget );

#ifndef __ATHEOS__
        virtual bool event( QEvent *event );

        virtual void partActivateEvent( PartActivateEvent *event );

        virtual void partSelectEvent( PartSelectEvent *event );

        virtual void guiActivateEvent( GUIActivateEvent *event );
#endif
    private slots:
        void slotWidgetDestroyed();


    private:
        KInstance *m_instance;
        PartManager *m_manager;
        bool m_selectable;
        QGuardedPtr<QWidget> m_widget;
    };

    class ReadOnlyPart : public Part
    {
        Q_OBJECT
    public:
        ReadOnlyPart( QObject *parent = 0, const char *name = 0 );
        virtual ~ReadOnlyPart();

    public slots:
        virtual bool openURL( const KURL &url ) = 0;

        KURL url() const { return m_url; }

        virtual bool closeURL() = 0;

    signals:
        void started( KIO::Job * );
        void completed();
        void canceled( const QString & );

    protected:
#ifndef __ATHEOS__
        virtual void guiActivateEvent( GUIActivateEvent *event );
#endif
        KURL m_url;
    };

};

#endif
