
#include <qmultilineedit.h>

#include <gui/textview.h>
#include <util/message.h>

enum { ID_SELECTED };

class QMultiLineEdit::Private
{
public:
    os::TextView* m_pcTextView;
};

QMultiLineEdit::QMultiLineEdit( QWidget* pcParent ) : QWidget( pcParent, "text_view_cont" )
{
    d = new Private;
    d->m_pcTextView = new os::TextView( GetBounds(), "text_view", "", os::CF_FOLLOW_ALL );
    d->m_pcTextView->SetMultiLine( true );
    d->m_pcTextView->SetMessage( new os::Message( ID_SELECTED ) );

    AddChild( d->m_pcTextView );
    d->m_pcTextView->SetTarget( this );
}

QMultiLineEdit::~QMultiLineEdit()
{
    delete d;
}

void QMultiLineEdit::selectAll()
{
    d->m_pcTextView->SelectAll();
}

bool QMultiLineEdit::hasMarkedText() const
{
    return( d->m_pcTextView->GetSelection() );
}

void QMultiLineEdit::getCursorPosition( int *line, int *col ) const
{
    d->m_pcTextView->GetCursor( col, line );
}

void QMultiLineEdit::setCursorPosition( int line, int col, bool mark )
{
    d->m_pcTextView->SetCursor( col, line, mark );
}

void QMultiLineEdit::setReadOnly( bool bFlag )
{
    d->m_pcTextView->SetReadOnly( bFlag );
}

bool QMultiLineEdit::isReadOnly() const
{
    return( d->m_pcTextView->GetReadOnly() );
}

QSize QMultiLineEdit::sizeHint() const
{
    os::Point cSize = d->m_pcTextView->GetPreferredSize( false );
    return( QSize( cSize.x + 1, cSize.y + 1 ) );
}

void QMultiLineEdit::SetMinPreferredSize( int nWidthChars, int nHeightChars )
{
    d->m_pcTextView->SetMaxPreferredSize( nWidthChars, nHeightChars );
    d->m_pcTextView->SetMinPreferredSize( nWidthChars, nHeightChars );
}

void QMultiLineEdit::setText( const QString& cText )
{
    d->m_pcTextView->Set( cText.utf8().data() );
}

QString QMultiLineEdit::text() const
{
    QString txt;
    for( int i = 0 ; i < numLines() ; ++i ) {
	txt += textLine(i) + QString::fromLatin1("\n");
    }
    return( txt );
}

QString QMultiLineEdit::textLine( int line ) const
{
    return( QString::fromUtf8( d->m_pcTextView->GetBuffer()[line].c_str() ) );
}

int QMultiLineEdit::numLines() const
{
    return( d->m_pcTextView->GetBuffer().size() );
}

void QMultiLineEdit::HandleMessage( os::Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_SELECTED:
	{
	    uint32 nEvents = 0;
	    
	    pcMessage->FindInt( "events", &nEvents );
	    
	    if ( nEvents & os::TextView::EI_CONTENT_CHANGED ) {
		emit textChanged();
	    }
	    break;
	}
	default:
	    QWidget::HandleMessage( pcMessage );
	    break;
    }
}

#include "amultilineedit.moc"
