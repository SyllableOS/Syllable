
#include <qlistbox.h>
#include <gui/listview.h>
#include <util/message.h>

enum { ID_SELECTED };

class KListBox::Private
{
public:
    os::ListView* m_pcMenu;
    bool	  m_bIgnoreSelection;
};


QListBoxText::QListBoxText( const QString& cString )
{
    AppendString( cString.utf8().data() );
}

KListBox::KListBox( QWidget* pcParent ) : QListBox( pcParent )
{
    d = new Private;
    d->m_pcMenu = new os::ListView( GetBounds(), "list_box",
				    os::ListView::F_RENDER_BORDER | os::ListView::F_NO_HEADER | os::ListView::F_NO_AUTO_SORT | os::ListView::F_MULTI_SELECT,
				    os::CF_FOLLOW_ALL );
    d->m_pcMenu->InsertColumn( "", 150 );
    d->m_pcMenu->SetSelChangeMsg( new os::Message( ID_SELECTED ) );
    d->m_bIgnoreSelection = false;
    AddChild( d->m_pcMenu );
    d->m_pcMenu->SetTarget( this );
}

KListBox::~KListBox()
{
    delete d;
}

uint KListBox::count() const
{
    return( d->m_pcMenu->GetRowCount() );
}

void KListBox::clear()
{
    d->m_pcMenu->Clear();
}

void KListBox::insertItem( const QString &text, int index )
{
    std::string cBuffer;

    if ( text.isEmpty() == false ) {
	cBuffer = text.ascii();
    }
    d->m_pcMenu->InsertRow( index, new QListBoxText( text ) );
}

void KListBox::insertItem( const QListBoxItem* pcItem, int index )
{
    d->m_pcMenu->InsertRow( index, const_cast<QListBoxItem*>(pcItem) );
}

void KListBox::setSelectionMode( SelectionMode eMode )
{
    d->m_pcMenu->SetMultiSelect( eMode != Single );
}

void KListBox::setSelected( int nIndex, bool bSelected )
{
    if ( d->m_bIgnoreSelection ) {
	return;
    }
    d->m_pcMenu->Select( nIndex, false, bSelected );
}

void KListBox::setCurrentItem( int index )
{
    d->m_pcMenu->SetCurrentRow( index );
}

bool KListBox::isSelected( int nIndex ) const
{
    return( d->m_pcMenu->IsSelected( nIndex ) );
}

int KListBox::index( const QListBoxItem* pcItem ) const
{
    int nCount = d->m_pcMenu->GetRowCount();
    for ( int i = 0 ; i < nCount ; ++i ) {
	if ( static_cast<QListBoxItem*>(d->m_pcMenu->GetRow( i )) == pcItem ) {
	    return( i );
	}
    }
    return( -1 );
}

QSize KListBox::sizeHint() const
{
//    os::Point cSize = d->m_pcMenu->GetPreferredSize( false );
    return( QSize( 100/*cSize.x*/, 100/*cSize.y*/ ) );
}

void KListBox::HandleMessage( os::Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {	
	case ID_SELECTED:
	{
	    int nCount = d->m_pcMenu->GetRowCount();
	    d->m_bIgnoreSelection = true;
	    for ( int i = 0 ; i < nCount ; ++i ) {
//		emit activated( i /*d->m_pcMenu->GetFirstSelected()*/ );
		emit pressed( static_cast<QListBoxItem*>(d->m_pcMenu->GetRow(i)) );
	    }
	    d->m_bIgnoreSelection = false;
	    break;
	}
	default:
	    QWidget::HandleMessage( pcMessage );
	    break;
    }
}

#include "alistbox.moc"
