/****************************************************************************
** $Id$
**
** Implementation of QJisCodec class
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

/*! \class QJisCodec qjiscodec.h

  \brief Provides conversion to and from JIS character sets

  The QJisCodec class subclasses QTextCodec to provide support for JIS
  X 0201 Latin, JIS X 0201 Kana, JIS X 0208 and JIS X 0212.

  The environment variable \c UNICODEMAP_JP can be used to fine-tune how
  QJpUnicodeConv, QEucJpCodec, QJisCodec and QSjisCodec do their work.
  The QJpUnicodeConv documentation describes how to use this variable.

  It was largely written by Serika Kurusugawa a.k.a. Junji Takagi, and
  is included in Qt with the author's permission, and the grateful
  thanks of the Trolltech team. Here is the copyright statement for
  that code:

  \mustquote

  Copyright (c) 1999 Serika Kurusugawa, All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met: <ol>
  <li> Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  <li> Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  </ol>

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.
*/

#include "qjiscodec.h"

#ifndef QT_NO_CODECS

static const uchar Esc = 0x1b;
static const uchar So = 0x0e;	// Shift Out
static const uchar Si = 0x0f;	// Shift In

static const uchar ReverseSolidus = 0x5c;
static const uchar YenSign = 0x5c;
static const uchar Tilde = 0x7e;
static const uchar Overline = 0x7e;

#define	IsKana(c)	(((c) >= 0xa1) && ((c) <= 0xdf))
#define	IsJisChar(c)	(((c) >= 0x21) && ((c) <= 0x7e))

#define	QValidChar(u)	((u) ? QChar((ushort)(u)) : QChar::replacement)

enum Iso2022State{ Ascii, MinState = Ascii,
		   JISX0201_Latin, JISX0201_Kana,
		   JISX0208_1978, JISX0208_1983,
		   JISX0212, MaxState = JISX0212,
		   Unknown };

static const char Esc_CHARS[] = "()*+-./";

static const char Esc_Ascii[]		= {Esc, '(', 'B', 0 };
static const char Esc_JISX0201_Latin[]	= {Esc, '(', 'J', 0 };
static const char Esc_JISX0201_Kana[]	= {Esc, '(', 'I', 0 };
static const char Esc_JISX0208_1978[]	= {Esc, '$', '@', 0 };
static const char Esc_JISX0208_1983[]	= {Esc, '$', 'B', 0 };
static const char Esc_JISX0212[]	= {Esc, '$', '(', 'D', 0 };
static const char * const Esc_SEQ[] = { Esc_Ascii,
					Esc_JISX0201_Latin,
					Esc_JISX0201_Kana,
					Esc_JISX0208_1978,
					Esc_JISX0208_1983,
					Esc_JISX0212 };

/*! \internal */
QJisCodec::QJisCodec() : conv(QJpUnicodeConv::newConverter(JU_Default))
{
}

/*! \internal */
int QJisCodec::mibEnum() const
{
    /*
    Name: JIS_Encoding
    MIBenum: 16
    Source: JIS X 0202-1991.  Uses ISO 2022 escape sequences to
	    shift code sets as documented in JIS X 0202-1991.
    Alias: csJISEncoding
    */
    return 16;
}

/*! \internal */
QCString QJisCodec::fromUnicode(const QString& uc, int& len_in_out) const
{
    int l = QMIN((int)uc.length(),len_in_out);
    QCString result;
    Iso2022State state = Ascii;
    Iso2022State prev = Ascii;
    for (int i=0; i<l; i++) {
	QChar ch = uc[i];
	uint j;
	if ( ch.row() == 0x00 && ch.cell() < 0x80 ) {
	    // Ascii
	    if (state != JISX0201_Latin ||
		ch.cell() == ReverseSolidus || ch.cell() == Tilde) {
		state = Ascii;
	    }
	    j = ch.cell();
	} else if ((j = conv->UnicodeToJisx0201(ch.row(), ch.cell())) != 0) {
	    if (j < 0x80) {
		// JIS X 0201 Latin
		if (state != Ascii ||
		    ch.cell() == YenSign || ch.cell() == Overline) {
		    state = JISX0201_Latin;
		}
	    } else {
		// JIS X 0201 Kana
		state = JISX0201_Kana;
		j &= 0x7f;
	    }
	} else if ((j = conv->UnicodeToJisx0208(ch.row(), ch.cell())) != 0) {
	    // JIS X 0208
	    state = JISX0208_1983;
	} else if ((j = conv->UnicodeToJisx0212(ch.row(), ch.cell())) != 0) {
	    // JIS X 0212
	    state = JISX0212;
	} else {
	    // Invalid
	    state = Unknown;
	    j = '?';
	}
	if (state != prev) {
	    if (state == Unknown) {
		result += Esc_Ascii;
	    } else {
		result += Esc_SEQ[state - MinState];
	    }
	    prev = state;
	}
	if (j < 0x0100) {
	    result += j & 0xff;
	} else {
	    result += (j >> 8) & 0xff;
	    result += j & 0xff;
	}
    }
    if (prev != Ascii) {
	result += Esc_Ascii;
    }
    len_in_out = result.length();
    return result;
}

/*! \internal */
QString QJisCodec::toUnicode(const char* chars, int len) const
{
    QString result;
    Iso2022State state = Ascii, prev = Ascii;
    for (int i=0; i<len; i++) {
	uchar ch = chars[i];
	if ( ch == Esc ) {
	    // Escape sequence
	    state = Unknown;
	    if ( i < len-1 ) {
		uchar c2 = chars[++i];
		if (c2 == '$') {
		    if ( i < len-1 ) {
			uchar c3 = chars[++i];
			if (strchr(Esc_CHARS, c3)) {
			    if ( i < len-1 ) {
				uchar c4 = chars[++i];
				if (c4 == '(') {
				    switch (c4) {
				      case 'D':
					state = JISX0212;	// Esc $ ( D
					break;
				    }
				}
			    }
			} else {
			    switch (c3) {
			      case '@':
				state = JISX0208_1978;	// Esc $ @
				break;
			      case 'B':
				state = JISX0208_1983;	// Esc $ B
				break;
			    }
			}
		    }
		} else {
		    if (strchr(Esc_CHARS, c2)) {
			if ( i < len-1 ) {
			    uchar c3 = chars[++i];
			    if (c2 == '(') {
				switch (c3) {
				  case 'B':
				    state = Ascii;	// Esc ( B
				    break;
				  case 'I':
				    state = JISX0201_Kana;	// Esc ( I
				    break;
				  case 'J':
				    state = JISX0201_Latin;	// Esc ( J
				    break;
				}
			    }
			}
		    }
		}
	    }
	} else if (ch == So) {
	    // Shift out
	    prev = state;
	    state = JISX0201_Kana;
	} else if (ch == Si) {
	    // Shift in
	    if (prev == Ascii || prev == JISX0201_Latin) {
		state = prev;
	    } else {
		state = Ascii;
	    }
	} else {
	    uint u;
	    switch (state) {
	      case Ascii:
		if (ch < 0x80) {
		    result += QChar(ch);
		    break;
		}
		/* fall throught */
	      case JISX0201_Latin:
		u = conv->Jisx0201ToUnicode(ch);
		result += QValidChar(u);
		break;
	      case JISX0201_Kana:
		u = conv->Jisx0201ToUnicode(ch | 0x80);
		result += QValidChar(u);
		break;
	      case JISX0208_1978:
	      case JISX0208_1983:
		if ( i < len-1 ) {
		    uchar c2 = chars[++i];
		    u = conv->Jisx0208ToUnicode(ch & 0x7f, c2 & 0x7f);
		    result += QValidChar(u);
		}
		break;
	      case JISX0212:
		if ( i < len-1 ) {
		    uchar c2 = chars[++i];
		    u = conv->Jisx0212ToUnicode(ch & 0x7f, c2 & 0x7f);
		    result += QValidChar(u);
		}
		break;
	      default:
		result += QChar::replacement;
		break;
	    }
	}
    }
    return result;
}

/*! \internal */
const char* QJisCodec::name() const
{
    return "JIS7";
}

/*! \internal */
int QJisCodec::heuristicNameMatch(const char* hint) const
{
    int score = 0;
    bool ja = FALSE;
    if (qstrnicmp(hint, "ja_JP", 5) == 0 || qstrnicmp(hint, "japan", 5) == 0) {
	score += 3;
	ja = TRUE;
    } else if (qstrnicmp(hint, "ja", 2) == 0) {
	score += 2;
	ja = TRUE;
    }
    const char *p;
    if (ja) {
	p = strchr(hint, '.');
	if (p == 0) {
	    return score - 2;
	}
	p++;
    } else {
	p = hint;
    }
    if (p) {
	if ((qstricmp(p, "JIS") == 0) ||
	    (qstricmp(p, "JIS7") == 0) ||
	    (simpleHeuristicNameMatch("ISO-2022-JP", p) > 0)) {
	    return score + 4;
	}
    }
    return QTextCodec::heuristicNameMatch(hint);
}

/*! \internal */
int QJisCodec::heuristicContentMatch(const char* chars, int len) const
{
    int score = 0;
    Iso2022State state = Ascii, prev = Ascii;
    for (int i=0; i<len; i++) {
	uchar ch = chars[i];
	// No nulls allowed.
	if ( !ch )
	    return -1;
	if ( ch == Esc ) {
	    // Escape sequence
	    state = Unknown;
	    if ( i < len-1 ) {
		uchar c2 = chars[++i];
		if (c2 == '$') {
		    if ( i < len-1 ) {
			uchar c3 = chars[++i];
			if (strchr(Esc_CHARS, c3)) {
			    if ( i < len-1 ) {
				uchar c4 = chars[++i];
				if (c4 == '(') {
				    switch (c4) {
				      case 'D':
					state = JISX0212;	// Esc $ ( D
					score++;
					break;
				    }
				}
			    }
			    score++;
			} else {
			    switch (c3) {
			      case '@':
				state = JISX0208_1978;	// Esc $ @
				score++;
				break;
			      case 'B':
				state = JISX0208_1983;	// Esc $ B
				score++;
				break;
			    }
			}
		    }
		    score++;
		} else {
		    if (strchr(Esc_CHARS, c2)) {
			if ( i < len-1 ) {
			    uchar c3 = chars[++i];
			    if (c2 == '(') {
				switch (c3) {
				  case 'B':
				    state = Ascii;	// Esc ( B
				    score++;
				    break;
				  case 'I':
				    state = JISX0201_Kana;	// Esc ( I
				    score++;
				    break;
				  case 'J':
				    state = JISX0201_Latin;	// Esc ( J
				    score++;
				    break;
				}
			    }
			}
			score++;
		    }
		}
	    }
	    if ( state == Unknown ) {
		return -1;
	    }
	    score++;
	} else if (ch == So) {
	    // Shift out
	    prev = state;
	    state = JISX0201_Kana;
	    score++;
	} else if (ch == Si) {
	    // Shift in
	    if (prev == Ascii || prev == JISX0201_Latin) {
		state = prev;
	    } else {
		state = Ascii;
	    }
	    score++;
	} else {
	    switch (state) {
	      case Ascii:
	      case JISX0201_Latin:
		if ( ch < 32 && ch != '\t' && ch != '\n' && ch != '\r' ) {
		    // Suspicious
		    if ( score )
		      score--;
		} else {
		    // Inconclusive
		}
		break;
	      case JISX0201_Kana:
		if ( !IsKana(ch | 0x80) ) {
		    return -1;
		}
		score++;
		break;
	      case JISX0208_1978:
	      case JISX0208_1983:
	      case JISX0212:
		if ( !IsJisChar(ch) ) {
		    // Invalid
		    return -1;
		}
		if ( i < len-1 ) {
		    uchar c2 = chars[++i];
		    if ( !IsJisChar(c2) ) {
			// Invalid
			return -1;
		    }
		    score++;
		}
		score++;
		break;
	      default:
		return -1;
	    }
	}
    }
    return score;
}

class QJisDecoder : public QTextDecoder {
    uchar buf[4];
    int nbuf;
    Iso2022State state, prev;
    bool esc;
    const QJpUnicodeConv * const conv;
public:
    QJisDecoder(const QJpUnicodeConv *c) : nbuf(0), state(Ascii), prev(Ascii), esc(FALSE), conv(c)
    {
    }

    QString toUnicode(const char* chars, int len)
    {
	QString result;
	for (int i=0; i<len; i++) {
	    uchar ch = chars[i];
	    if (esc) {
		// Escape sequence
		state = Unknown;
		switch (nbuf) {
		  case 0:
		    if (ch == '$' || strchr(Esc_CHARS, ch)) {
			buf[nbuf++] = ch;
		    } else {
			nbuf = 0;
			esc = FALSE;
		    }
		    break;
		  case 1:
		    if (buf[0] == '$') {
			if (strchr(Esc_CHARS, ch)) {
			    buf[nbuf++] = ch;
			} else {
			    switch (ch) {
			      case '@':
				state = JISX0208_1978;	// Esc $ @
				break;
			      case 'B':
				state = JISX0208_1983;	// Esc $ B
				break;
			    }
			    nbuf = 0;
			    esc = FALSE;
			}
		    } else {
			if (buf[0] == '(') {
			    switch (ch) {
			      case 'B':
				state = Ascii;	// Esc ( B
				break;
			      case 'I':
				state = JISX0201_Kana;	// Esc ( I
				break;
			      case 'J':
				state = JISX0201_Latin;	// Esc ( J
				break;
			    }
			}
			nbuf = 0;
			esc = FALSE;
		    }
		    break;
		  case 2:
		    if (buf[1] == '(') {
			switch (ch) {
			  case 'D':
			    state = JISX0212;	// Esc $ ( D
			    break;
			}
		    }
		    nbuf = 0;
		    esc = FALSE;
		    break;
		}
	    } else {
		if (ch == Esc) {
		    // Escape sequence
		    nbuf = 0;
		    esc = TRUE;
		} else if (ch == So) {
		    // Shift out
		    prev = state;
		    state = JISX0201_Kana;
		    nbuf = 0;
		} else if (ch == Si) {
		    // Shift in
		    if (prev == Ascii || prev == JISX0201_Latin) {
			state = prev;
		    } else {
			state = Ascii;
		    }
		    nbuf = 0;
		} else {
		    uint u;
		    switch (nbuf) {
		      case 0:
			switch (state) {
			  case Ascii:
			    if (ch < 0x80) {
				result += QChar(ch);
				break;
			    }
			    /* fall throught */
			  case JISX0201_Latin:
			    u = conv->Jisx0201ToUnicode(ch);
			    result += QValidChar(u);
			    break;
			  case JISX0201_Kana:
			    u = conv->Jisx0201ToUnicode(ch | 0x80);
			    result += QValidChar(u);
			    break;
			  case JISX0208_1978:
			  case JISX0208_1983:
			  case JISX0212:
			    buf[nbuf++] = ch;
			    break;
			  default:
			    result += QChar::replacement;
			    break;
			}
			break;
		      case 1:
			switch (state) {
			  case JISX0208_1978:
			  case JISX0208_1983:
			    u = conv->Jisx0208ToUnicode(buf[0] & 0x7f, ch & 0x7f);
			    result += QValidChar(u);
			    break;
			  case JISX0212:
			    u = conv->Jisx0212ToUnicode(buf[0] & 0x7f, ch & 0x7f);
			    result += QValidChar(u);
			    break;
			  default:
			    result += QChar::replacement;
			    break;
			}
			nbuf = 0;
			break;
		    }
		}
	    }
	}
	return result;
    }
};

/*! \internal */
QTextDecoder* QJisCodec::makeDecoder() const
{
    return new QJisDecoder(conv);
}

#endif
