/****************************************************************************
** $Id$
**
** Definition of the QSizePolicy class
**
** Created : 980929
**
** Copyright (C) 1998-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QSIZEPOLICY_H
#define QSIZEPOLICY_H

#ifndef QT_H
#include "qglobal.h"
#endif // QT_H

class Q_EXPORT QSizePolicy
{
private:
    enum { HSize = 6, HMask = 0x3f, VMask = HMask << HSize,
	   MayGrow = 1, ExpMask = 2, MayShrink = 4 };
public:
    enum SizeType { Fixed = 0,
		    Minimum = MayGrow,
		    Maximum = MayShrink,
		    Preferred = MayGrow|MayShrink ,
		    MinimumExpanding = Minimum|ExpMask,
		    Expanding = MinimumExpanding|MayShrink };

    enum ExpandData { NoDirection = 0,
		      Horizontal = 1,
		      Vertical = 2,
		      BothDirections = Horizontal | Vertical };

    QSizePolicy() { data = 0; }

    QSizePolicy( SizeType hor, SizeType ver, bool hfw = FALSE );

    SizeType horData() const { return (SizeType)( data & HMask ); }
    SizeType verData() const { return (SizeType)(( data & VMask ) >> HSize); }

    bool mayShrinkHorizontally() const { return horData() & MayShrink; }
    bool mayShrinkVertically() const { return verData() & MayShrink; }
    bool mayGrowHorizontally() const { return horData() & MayGrow; }
    bool mayGrowVertically() const { return verData() & MayGrow; }

    ExpandData expanding() const
    {
	return (ExpandData)( (int)(verData()&ExpMask ? Vertical : 0)+
			     (int)(horData()&ExpMask ? Horizontal : 0) );
    }

    void setHorData( SizeType d ) { data = (data & ~HMask) | d; }
    void setVerData( SizeType d ) { data = (data & ~(HMask<<HSize)) |
					   (d<<HSize); }
		
    void setHeightForWidth( bool b ) { data = b ? ( data | ( 1 << 2*HSize ) )
					      : ( data & ~( 1 << 2*HSize ) );  }
    bool hasHeightForWidth() const { return data & ( 1 << 2*HSize ); }

    bool operator==( const QSizePolicy& s ) const { return data == s.data; }
    bool operator!=( const QSizePolicy& s ) const { return data != s.data; }

private:
    QSizePolicy( int i ): data( i ) {}

    Q_UINT16 data;
};

#if !(defined(__GNUC__) && __GNUC__ == 2 && __GNUC_MINOR__ == 96)
inline QSizePolicy::QSizePolicy( SizeType hor, SizeType ver, bool hfw )
	: data( hor | (ver<<HSize) | (hfw ? (1<<2*HSize) : 0) ) {}
#endif

#endif
