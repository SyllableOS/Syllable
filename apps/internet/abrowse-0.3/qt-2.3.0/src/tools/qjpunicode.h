/****************************************************************************
** $Id$
**
** Definition of QJpUnicodeConv class
**
** Created : 990225
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the tools module of the Qt GUI Toolkit.
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

// Most of the code here was originally written by Serika Kurusugawa
// a.k.a. Junji Takagi, and is include in Qt with the author's permission,
// and the grateful thanks of the Trolltech team.

/*
 * Copyright (c) 1999 Serika Kurusugawa, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef QJPUNICODE_H
#define QJPUNICODE_H

#ifndef QT_H
#include "qglobal.h"
#endif // QT_H


// "ASCII" is ANSI X.3.4-1986, a.k.a. US-ASCII here.
#define	JU_Default		0x0000

#define	JU_Unicode		0x0001
#define	JU_Unicode_JISX0201	0x0001
#define	JU_Unicode_ASCII	0x0002
#define	JU_JISX0221_JISX0201	0x0003
#define	JU_JISX0221_ASCII	0x0004
#define JU_Sun_JDK117           0x0005
#define JU_Microsoft_CP932      0x0006

#define	JU_NEC_VDC	0x0100		// NEC Vender Defined Char
#define	JU_UDC		0x0200		// User Defined Char
#define	JU_IBM_VDC	0x0400		// IBM Vender Defined Char

class Q_EXPORT QJpUnicodeConv {
public:
    static const QJpUnicodeConv *newConverter(int rule);

    // ### NOTE: member function names will be changed in Qt 3.0; see doc

    virtual uint AsciiToUnicode(uint h, uint l) const;
    /*virtual*/ uint Jisx0201ToUnicode(uint h, uint l) const;
    virtual uint Jisx0201LatinToUnicode(uint h, uint l) const;
    /*virtual*/ uint Jisx0201KanaToUnicode(uint h, uint l) const;
    virtual uint Jisx0208ToUnicode(uint h, uint l) const;
    virtual uint Jisx0212ToUnicode(uint h, uint l) const;

    // ### NOTE: member function names will be changed in Qt 3.0; see doc

    uint AsciiToUnicode(uint ascii) const {
	return AsciiToUnicode((ascii & 0xff00) >> 8, (ascii & 0x00ff));
    }
    uint Jisx0201ToUnicode(uint jis) const {
	return Jisx0201ToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
    }
    uint Jisx0201LatinToUnicode(uint jis) const {
	return Jisx0201LatinToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
    }
    uint Jisx0201KanaToUnicode(uint jis) const {
	return Jisx0201KanaToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
    }
    uint Jisx0208ToUnicode(uint jis) const {
	return Jisx0208ToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
    }
    uint Jisx0212ToUnicode(uint jis) const {
	return Jisx0212ToUnicode((jis & 0xff00) >> 8, (jis & 0x00ff));
    }

    // ### NOTE: member function names will be changed in Qt 3.0; see doc

    virtual uint UnicodeToAscii(uint h, uint l) const;
    /*virtual*/ uint UnicodeToJisx0201(uint h, uint l) const;
    virtual uint UnicodeToJisx0201Latin(uint h, uint l) const;
    /*virtual*/ uint UnicodeToJisx0201Kana(uint h, uint l) const;
    virtual uint UnicodeToJisx0208(uint h, uint l) const;
    virtual uint UnicodeToJisx0212(uint h, uint l) const;

    // ### NOTE: member function names will be changed in Qt 3.0; see doc

    uint UnicodeToAscii(uint unicode) const {
	return UnicodeToAscii((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }
    uint UnicodeToJisx0201(uint unicode) const {
	return UnicodeToJisx0201((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }
    uint UnicodeToJisx0201Latin(uint unicode) const {
	return UnicodeToJisx0201Latin((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }
    uint UnicodeToJisx0201Kana(uint unicode) const {
	return UnicodeToJisx0201Kana((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }
    uint UnicodeToJisx0208(uint unicode) const {
	return UnicodeToJisx0208((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }
    uint UnicodeToJisx0212(uint unicode) const {
	return UnicodeToJisx0212((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }

    // ### NOTE: member function names will be changed in Qt 3.0; see doc

    uint SjisToUnicode(uint h, uint l) const;
    uint UnicodeToSjis(uint h, uint l) const;

    // ### NOTE: member function names will be changed in Qt 3.0; see doc

    uint SjisToUnicode(uint sjis) const {
	return SjisToUnicode((sjis & 0xff00) >> 8, (sjis & 0x00ff));
    }
    uint UnicodeToSjis(uint unicode) const {
	return UnicodeToSjis((unicode & 0xff00) >> 8, (unicode & 0x00ff));
    }

protected:
    QJpUnicodeConv(int r) : rule(r) {}

private:
    int rule;
};

#endif /* QJPUNICODE_H */
