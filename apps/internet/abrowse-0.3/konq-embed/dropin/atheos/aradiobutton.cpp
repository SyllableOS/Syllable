

#include <qradiobutton.h>

#include <gui/radiobutton.h>
#include <util/message.h>

enum { ID_SELECTED };

class QRadioButton::Private
{
public:
    os::RadioButton* m_pcRadioButton;    
};

QRadioButton::QRadioButton( QWidget* pcParent ) : QWidget( pcParent, "radiobutton" )
{
    d = new Private;

    d->m_pcRadioButton = new os::RadioButton( GetBounds(), "radiobutton", "", new os::Message( ID_SELECTED ), os::CF_FOLLOW_ALL );

    AddChild( d->m_pcRadioButton );
    d->m_pcRadioButton->SetTarget( this );
    
}

QRadioButton::~QRadioButton()
{
    delete d;
}

bool QRadioButton::isChecked() const
{
    return( d->m_pcRadioButton->GetValue() );
}

void QRadioButton::setChecked( bool check )
{
    d->m_pcRadioButton->SetValue( check, false );
}

QSize QRadioButton::sizeHint() const
{
    os::Point cSize = d->m_pcRadioButton->GetPreferredSize( false );
    return( QSize( int(cSize.x), int(cSize.y) ) ); // Sees like KHTML doesn't set the size exactly enough
}

void QRadioButton::HandleMessage( os::Message* pcMessage )
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

#include "aradiobutton.moc"
