#ifndef __F_ALISTBOX_H__
#define __F_ALISTBOX_H__

#include <qwidget.h>

#include <gui/listview.h>

class QListBoxItem : public QObject, public os::ListViewStringRow
{
    Q_OBJECT
public:
    QListBoxItem() { /*m_bSelectable = true;*/ }

    void setSelectable( bool bSelectable ) { SetIsSelectable( bSelectable ); }

private:
    class Private;
    Private* d;
//    bool m_bSelectable;
};

class QListBoxText : public QListBoxItem
{
public:
    QListBoxText( const QString& cString );
};

class QListBox : public QWidget
{
    Q_OBJECT
public:
    enum SelectionMode { Single, Multi, Extended, NoSelection };
public:
    QListBox( QWidget* pcParent ) : QWidget( pcParent, "list_view" ) {}
signals:
    void pressed(QListBoxItem*);
};

class KListBox  : public QListBox
{
    Q_OBJECT
public:
    KListBox( QWidget* pcParent );
    ~KListBox();
    virtual void HandleMessage( os::Message* pcMessage );
    
    uint count() const;

    void clear();

    void insertItem( const QString &text, int index=-1 );
    void insertItem( const QListBoxItem *, int index=-1 );

    virtual void setSelectionMode( SelectionMode );
    void setSelected( int, bool );
    virtual void setCurrentItem( int index );
    bool isSelected( int ) const;

    int index( const QListBoxItem * ) const;
    virtual QSize sizeHint() const;
/*
signals:
    void activated( int );
    */  
private:
    class Private;
    Private* d;
};

#endif // __F_ALISTBOX_H__
