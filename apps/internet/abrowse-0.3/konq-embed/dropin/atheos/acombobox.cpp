#include <stdio.h>

#include <qcombobox.h>
#include <gui/dropdownmenu.h>
#include <util/message.h>

enum { ID_SELECTED };

class KComboBox::Private
{
public:
    os::DropdownMenu* m_pcMenu;
};


KComboBox::KComboBox( bool, QWidget* pcParent ) : QWidget( pcParent, "combo_box" )
{
    d = new Private;
    d->m_pcMenu = new os::DropdownMenu( GetBounds(), "combo_box", os::CF_FOLLOW_ALL );
    d->m_pcMenu->SetReadOnly( true );
    d->m_pcMenu->SetSelectionMessage( new os::Message( ID_SELECTED ) );
    AddChild( d->m_pcMenu );
    d->m_pcMenu->SetTarget( this );
}

int KComboBox::count() const
{
    return( d->m_pcMenu->GetItemCount() );
}

void KComboBox::clear()
{
    d->m_pcMenu->Clear();
}

void KComboBox::insertItem( const QString &text, int index=-1 )
{
    std::string cBuffer;

    if ( text.isEmpty() == false ) {
	cBuffer = text.ascii();
    }
    if ( index == -1 ) {
	d->m_pcMenu->AppendItem( text.ascii() );
    } else {
	d->m_pcMenu->InsertItem( index, cBuffer.c_str() );
    }
}

void KComboBox::setCurrentItem( int index )
{
    d->m_pcMenu->SetSelection( index, false );
}

QSize KComboBox::sizeHint() const
{
    os::Point cSize = d->m_pcMenu->GetPreferredSize( false );
    return( QSize( cSize.x, cSize.y ) );
}


void KComboBox::HandleMessage( os::Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_SELECTED:
	{
	    bool bFinal = false;
	    pcMessage->FindBool( "final", &bFinal );
	    if ( bFinal ) {
		emit activated( d->m_pcMenu->GetSelection() );
	    }
	    break;
	}
	default:
	    QWidget::HandleMessage( pcMessage );
	    break;
    }
}

#include "acombobox.moc"
