
#include <qpushbutton.h>

#include <gui/button.h>
#include <util/message.h>

enum { ID_SELECTED };

class QPushButton::Private
{
public:
    os::Button* m_pcButton;
};




QPushButton::QPushButton( QWidget* pcParent ) : QWidget( pcParent, "push_button_cont" )
{
    d = new Private;
    d->m_pcButton = new os::Button( GetBounds(), "push_button", "Button", new os::Message( ID_SELECTED ), os::CF_FOLLOW_ALL );
    AddChild( d->m_pcButton );
    d->m_pcButton->SetTarget( this );
}

QPushButton::QPushButton( const QString &text, QWidget* pcParent, const char* /*name*/ ) : QWidget( pcParent, "push_button_cont" )
{
    d = new Private;
    d->m_pcButton = new os::Button( GetBounds(), "push_button", text.ascii(), NULL, os::CF_FOLLOW_ALL );
    AddChild( d->m_pcButton );
    d->m_pcButton->SetTarget( this );
}

QPushButton::~QPushButton()
{
    delete d;
}

void QPushButton::AllAttached()
{
    QWidget::AllAttached();
}

void QPushButton::HandleMessage( os::Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_SELECTED:
	    emit clicked();
	    break;
	default:
	    QWidget::HandleMessage( pcMessage );
	    break;
    }
}

void QPushButton::setText( const QString& cString )
{
    d->m_pcButton->SetLabel( cString.ascii() );
}

QSize QPushButton::sizeHint() const
{
    os::Point cSize = d->m_pcButton->GetPreferredSize( false );
    return( QSize( cSize.x + 1, cSize.y + 1 ) );
}

#include "apushbutton.moc"
