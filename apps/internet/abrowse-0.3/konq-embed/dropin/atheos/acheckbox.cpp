
#include <qcheckbox.h>

#include <gui/checkbox.h>
#include <util/message.h>

enum { ID_SELECTED };

class QCheckBox::Private
{
public:
    os::CheckBox* m_pcCheckBox;
};

QCheckBox::QCheckBox( QWidget* pcParent ) : QWidget( pcParent, "checkbox" )
{
    d = new Private;
    d->m_pcCheckBox = new os::CheckBox( GetBounds(), "radiobutton", "", new os::Message( ID_SELECTED ), os::CF_FOLLOW_ALL );

    AddChild( d->m_pcCheckBox );
    d->m_pcCheckBox->SetTarget( this );
}

bool QCheckBox::isChecked() const
{
    return( d->m_pcCheckBox->GetValue() );
}

void QCheckBox::setChecked( bool check )
{
    d->m_pcCheckBox->SetValue( check, false );
}

QSize QCheckBox::sizeHint() const
{
    os::Point cSize = d->m_pcCheckBox->GetPreferredSize( false );
    return( QSize( int(cSize.x), int(cSize.y) ) );
}


void QCheckBox::HandleMessage( os::Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_SELECTED:
	    emit clicked();
	    emit stateChanged( (d->m_pcCheckBox->GetValue()) ? 2 : 0 );
	    break;
	default:
	    QWidget::HandleMessage( pcMessage );
	    break;
    }
}

#include "acheckbox.moc"
