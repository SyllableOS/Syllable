/****************************************************************************
** $Id$
**
** Implementation of QKoi8Codec class
**
** Created : 981015
**
** Copyright (C)1998-2000 Trolltech AS.  All rights reserved.
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

#include "qkoi8codec.h"

#ifndef QT_NO_CODECS

int QKoi8Codec::mibEnum() const
{
    return 2084;
}

static const uchar unkn = '?'; // BLACK SQUARE (94) would be better

/* #!perl

open IN,"<charmaps/KOI8-R";
while (<IN>) {
    $ok = ($mn,$koi8,$uni,$name) = 
	/(\S*)\s*\/x(..)\s*<U(....)> (.*)/;
    $unicode_to_koi8{lc $uni} = $koi8;
    $koi8_to_unicode{lc $koi8} = $uni;
}
@a = sort grep {/25../} keys %unicode_to_koi8;
$max = pop @a; $max =~ s/25//; $max=hex $max; $max++;
print "static const int n_unicode_to_koi8_25 = $max;\n";
print "static const uchar unicode_to_koi8_25[$max] = {\n\t";
for $u (0x2500..(0x2500+$max-1)) {
    $uni = sprintf("%04x",$u);
    if ( $koi8 = $unicode_to_koi8{$uni} ) {
	print "0x$koi8,";
    } else {
	print "unkn,";
    }
    print "\n\t" if $u % 8 == 7;
}
print "};\n\n";
@a = sort grep {/04../} keys %unicode_to_koi8;
$max = pop @a; $max =~ s/04//; $max=hex $max; $max++;
print "static const int n_unicode_to_koi8_04 = $max;\n";
print "static const uchar unicode_to_koi8_04[$max] = {\n\t";
for $u (0x0400..(0x0400+$max-1)) {
    $uni = sprintf("%04x",$u);
    if ( $koi8 = $unicode_to_koi8{$uni} ) {
	print "0x$koi8,";
    } else {
	print "unkn,";
    }
    print "\n\t" if $u % 8 == 7;
}
print "};\n\n";
print "static const ushort koi8_to_unicode[256] = {\n\t";
for $k (0x00..0xff) {
    $koi8 = sprintf("%02x",$k);
    print "0x$koi8_to_unicode{$koi8},";
    print "\n\t" if $k % 8 == 7;
}
print "};\n\n";
__END__
*/

static const int n_unicode_to_koi8_25 = 161;
static const uchar unicode_to_koi8_25[161] = {
	0x80,unkn,0x81,unkn,unkn,unkn,unkn,unkn,
	unkn,unkn,unkn,unkn,0x82,unkn,unkn,unkn,
	0x83,unkn,unkn,unkn,0x84,unkn,unkn,unkn,
	0x85,unkn,unkn,unkn,0x86,unkn,unkn,unkn,
	unkn,unkn,unkn,unkn,0x87,unkn,unkn,unkn,
	unkn,unkn,unkn,unkn,0x88,unkn,unkn,unkn,
	unkn,unkn,unkn,unkn,0x89,unkn,unkn,unkn,
	unkn,unkn,unkn,unkn,0x8A,unkn,unkn,unkn,
	unkn,unkn,unkn,unkn,unkn,unkn,unkn,unkn,
	unkn,unkn,unkn,unkn,unkn,unkn,unkn,unkn,
	0xA0,0xA1,0xA2,0xA4,0xA5,0xA6,0xA7,0xA8,
	0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,
	0xB1,0xB2,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,
	0xBA,0xBB,0xBC,0xBD,0xBE,unkn,unkn,unkn,
	unkn,unkn,unkn,unkn,unkn,unkn,unkn,unkn,
	unkn,unkn,unkn,unkn,unkn,unkn,unkn,unkn,
	0x8B,unkn,unkn,unkn,0x8C,unkn,unkn,unkn,
	0x8D,unkn,unkn,unkn,0x8E,unkn,unkn,unkn,
	0x8F,0x90,0x91,0x92,unkn,unkn,unkn,unkn,
	unkn,unkn,unkn,unkn,unkn,unkn,unkn,unkn,
	0x94,};

static const int n_unicode_to_koi8_04 = 82;
static const uchar unicode_to_koi8_04[82] = {
	unkn,0xB3,unkn,unkn,unkn,unkn,unkn,unkn,
	unkn,unkn,unkn,unkn,unkn,unkn,unkn,unkn,
	0xE1,0xE2,0xF7,0xE7,0xE4,0xE5,0xF6,0xFA,
	0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,
	0xF2,0xF3,0xF4,0xF5,0xE6,0xE8,0xE3,0xFE,
	0xFB,0xFD,0xFF,0xF9,0xF8,0xFC,0xE0,0xF1,
	0xC1,0xC2,0xD7,0xC7,0xC4,0xC5,0xD6,0xDA,
	0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,
	0xD2,0xD3,0xD4,0xD5,0xC6,0xC8,0xC3,0xDE,
	0xDB,0xDD,0xDF,0xD9,0xD8,0xDC,0xC0,0xD1,
	unkn,0xA3,};

static const ushort koi8_to_unicode[256] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
	0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
	0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F, 
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
	0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, 
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
	0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, 
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
	0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
	0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
	0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
	0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F, 
	0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524, 
	0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590, 
	0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2022, 0x221A, 0x2248, 
	0x2264, 0x2265, 0x00A0, 0x2321, 0x00B0, 0x00B2, 0x00B7, 0x00F7, 
	0x2550, 0x2551, 0x2552, 0x0451, 0x2553, 0x2554, 0x2555, 0x2556, 
	0x2557, 0x2558, 0x2559, 0x255A, 0x255B, 0x255C, 0x255D, 0x255E, 
	0x255F, 0x2560, 0x2561, 0x0401, 0x2562, 0x2563, 0x2564, 0x2565, 
	0x2566, 0x2567, 0x2568, 0x2569, 0x256A, 0x256B, 0x256C, 0x00A9, 
	0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 
	0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 
	0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 
	0x044C, 0x044B, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A, 
	0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 
	0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 
	0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 
	0x042C, 0x042B, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x042A, 
	};


QCString QKoi8Codec::fromUnicode(const QString& uc, int& len_in_out) const
{
    int l = QMIN((int)uc.length(),len_in_out);
    int rlen = l+1;
    QCString rstr(rlen);
    uchar* cursor = (uchar*)rstr.data();
    for (int i=0; i<l; i++) {
	const QChar ch = uc[i];
	if ( ch.row() ) {
	    if ( ch.row() == 0x25 ) {
		if ( ch.cell() < n_unicode_to_koi8_25 )
		    *cursor++ = unicode_to_koi8_25[ch.cell()];
		else
		    *cursor++ = unkn;
	    } else if ( ch.row() == 0x04 ) {
		if ( ch.cell() < n_unicode_to_koi8_04 )
		    *cursor++ = unicode_to_koi8_04[ch.cell()];
		else
		    *cursor++ = unkn;
	    } else if ( ch.row() == 0x22 ) {
		if ( ch.cell() == 0x1A )
		    *cursor++ = 0x96;
		else if ( ch.cell() == 0x48 )
		    *cursor++ = 0x97;
		else if ( ch.cell() == 0x64 )
		    *cursor++ = 0x98;
		else if ( ch.cell() == 0x65 )
		    *cursor++ = 0x99;
		else
		    *cursor++ = unkn;
	    } else if ( ch.row() == 0x23 ) {
		if ( ch.cell() == 0x20 )
		    *cursor++ = 0x93;
		else if ( ch.cell() == 0x21 )
		    *cursor++ = 0x9B;
		else
		    *cursor++ = unkn;
	    } else if ( ch.row() == 0x20 ) {
		if ( ch.cell() == 0x22 )
		    *cursor++ = 0x95;
		else
		    *cursor++ = unkn;
	    } else {
		*cursor++ = unkn;
	    }
	} else {
	    if ( ch.cell() < 128 ) {
		*cursor++ = ch.cell();
	    } else {
		*cursor++ = unkn;
	    }
	}
    }
    *cursor = '\0';
    // len_in_out = cursor - result;
    return rstr;
}

QString QKoi8Codec::toUnicode(const char* chars, int len) const
{
    QString result;
    for (int i=0; i<len; i++) {
	uchar ch = chars[i];
	result += QChar(koi8_to_unicode[ch]);
    }
    return result;
}

const char* QKoi8Codec::name() const
{
    return "KOI8-R";
}

int QKoi8Codec::heuristicNameMatch(const char* hint) const
{
    if ( qstrnicmp(hint,"koi8",4)==0 )
	return 2;
    return QTextCodec::heuristicNameMatch(hint);
}

int QKoi8Codec::heuristicContentMatch(const char* chars, int len) const
{
    int score = 0;
    for (int i=0; i<len; i++) {
	uchar ch = chars[i];
	// No nulls allowed.
	if ( !ch )
	    return -1;
    }
    return score;
}

#endif
