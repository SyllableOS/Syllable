/*  This file is part of the KDE project
    Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __ATHEOS__
#include "kdialogbase.h"
#include "klocale.h"

#include <qlayout.h>

#include <qpushbutton.h>


KDialogBase::KDialogBase( const QString &caption, int buttonMask, ButtonCode defaultButton,
                          ButtonCode escapeButton, QWidget *parent, const char *name,
                          bool modal, bool separator,
                          const QString &user1,
                          const QString &user2,
                          const QString &/*user3*/ ) // third button left out because
    // it is only used by KCookieWin, which activates the cookie-detail button, which
    // is too large for small displays, anyway
    : QDialog( parent, name, modal )
{
    m_mainLayout = new QVBoxLayout( this );

    m_buttonLayout = new QHBoxLayout( m_mainLayout );

    m_okButton = new QPushButton( user1.isEmpty() ? i18n( "Yes" ) : user1 , this );
    m_cancelButton = new QPushButton( user2.isEmpty() ? i18n( "No" ) : user2, this );

    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget( m_okButton );
    m_buttonLayout->addWidget( m_cancelButton );
    m_buttonLayout->addStretch();

    connect( m_okButton, SIGNAL( clicked() ),
             this, SLOT( slotOk() ) );
    connect( m_cancelButton, SIGNAL( clicked() ),
             this, SLOT( slotCancel() ) );

    m_mainWidget = 0;
}

KDialogBase::~KDialogBase()
{
}

void KDialogBase::setMainWidget( QWidget *widget )
{
    m_mainLayout->insertWidget( 0, widget );
    m_mainWidget = widget;
}

void KDialogBase::enableButtonSeparator( bool state )
{
}

void KDialogBase::slotOk()
{
    done( Yes );
}

void KDialogBase::slotCancel()
{
    done( No );
}

#include "kdialogbase.moc"

#endif // __ATHEOS__
