
#include <qlineedit.h>

#include <gui/textview.h>
#include <util/message.h>

enum { ID_SELECTED };

class QLineEdit::Private
{
public:
    os::TextView* m_pcTextView;
};

QLineEdit::QLineEdit( QWidget* pcParent ) : QWidget( pcParent, "text_view_cont" )
{
    d = new Private;
    d->m_pcTextView = new os::TextView( GetBounds(), "text_view", "", os::CF_FOLLOW_ALL );
    d->m_pcTextView->SetMessage( new os::Message( ID_SELECTED ) );

    AddChild( d->m_pcTextView );
    d->m_pcTextView->SetTarget( this );
}

QLineEdit::~QLineEdit()
{
    delete d;
}

bool QLineEdit::frame() const
{
    return( true );
}

void QLineEdit::selectAll()
{
    d->m_pcTextView->SelectAll();
}

void QLineEdit::setMaxLength( int nLength )
{
    d->m_pcTextView->SetMaxLength( nLength );
}

void QLineEdit::setReadOnly( bool bFlag )
{
    d->m_pcTextView->SetReadOnly( bFlag );
}

void QLineEdit::setText( const QString& cText )
{
    d->m_pcTextView->Set( cText.utf8().data() );
}

bool QLineEdit::isReadOnly() const
{
    return( d->m_pcTextView->GetReadOnly() );
}

void QLineEdit::setEchoMode( EchoMode eMode )
{
    d->m_pcTextView->SetPasswordMode( eMode != Normal );
}

QSize QLineEdit::sizeHint() const
{
    os::Point cSize = d->m_pcTextView->GetPreferredSize( false );
    return( QSize( cSize.x + 1, cSize.y + 1 ) );
}

void QLineEdit::SetMinPreferredSize( int nWidthChars )
{
    d->m_pcTextView->SetMinPreferredSize( nWidthChars, 1 );
    d->m_pcTextView->SetMaxPreferredSize( nWidthChars, 1 );
}

void QLineEdit::AllAttached()
{
    QWidget::AllAttached();
}

void QLineEdit::HandleMessage( os::Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_SELECTED:
	{
	    uint32 nEvents = 0;
	    
	    pcMessage->FindInt( "events", &nEvents );
	    if ( nEvents & os::TextView::EI_CONTENT_CHANGED ) {
		emit textChanged( QString::fromUtf8( d->m_pcTextView->GetBuffer()[0].c_str() ) );
	    }
	    if ( nEvents & os::TextView::EI_ENTER_PRESSED ) {
		emit returnPressed();
	    }
	    break;
	}
	default:
	    QWidget::HandleMessage( pcMessage );
	    break;
    }
}

#include "alineedit.moc"
