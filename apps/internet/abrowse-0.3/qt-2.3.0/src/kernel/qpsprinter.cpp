/**********************************************************************
** $Id$
**
** Implementation of QPSPrinter class
**
** Created : 941003
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
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
/*!  
  \mustquote

  True type font embedding for Qt's post script driver:
  
 Copyright 1995, Trinity College Computing Center.
 Written by David Chappell.

 Permission to use, copy, modify, and distribute this software and its
 documentation for any purpose and without fee is hereby granted, provided
 that the above copyright notice appear in all copies and that both that
 copyright notice and this permission notice appear in supporting
 documentation.  This software is provided "as is" without express or
 implied warranty.

 TrueType font support.  These functions allow PPR to generate
 PostScript fonts from Microsoft compatible TrueType font files.

 The functions in this file do most of the work to convert a
 TrueType font to a type 3 PostScript font.

 Most of the material in this file is derived from a program called
 "ttf2ps" which L. S. Ng posted to the usenet news group
 "comp.sources.postscript".  The author did not provide a copyright
 notice or indicate any restrictions on use.

 Last revised 11 July 1995.                                                 
*/       

#include "qpsprinter_p.h"

#ifndef QT_NO_PRINTER

#undef Q_PRINTER_USE_TYPE42

#include "qpainter.h"
#include "qapplication.h"
#include "qpaintdevicemetrics.h"
#include "qimage.h"
#include "qdatetime.h"

#include "qstring.h"
#include "qdict.h"
#include "qregexp.h"

#include "qfile.h"
#include "qbuffer.h"
#include "qintdict.h"
#include "qtextcodec.h"
#include "qjpunicode.h"

#include <ctype.h>
#if defined(_OS_WIN32_)
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef _WS_X11_
#include "qt_x11.h"
#ifdef None
#undef None
#endif
#ifdef GrayScale
#undef GrayScale
#endif
#endif

static bool qt_gen_epsf = TRUE;

void qt_generate_epsf( bool b )
{
    qt_gen_epsf = b;
}

// Note: this is comment-stripped and word-wrapped later.
// Note: stripHeader() constrains the postscript used in this prolog.
// only function/variable declarations are currently allowed, and
// stripHeader knows a bit about the names of some functions.

//
// #### improve build process and qembed the result, saving RAM and ROM
//

static const char * const ps_header[] = {
"/d  /def load def",
"/D  {bind d} bind d",
"/d2 {dup dup} D",
"/B  {0 d2} D",
"/W  {255 d2} D",
"/ED {exch d} D",
"/D0 {0 ED} D",
"/LT {lineto} D",
"/MT {moveto} D",
"/ND /.notdef d",
"/S  {stroke} D",
"/F  {setfont} D",
"/SW {setlinewidth} D",
"/CP {closepath} D",
"/RL {rlineto} D",
"/NP {newpath} D",
"/CM {currentmatrix} D",
"/SM {setmatrix} D",
"/TR {translate} D",
"/SC {aload pop setrgbcolor} D",
"",
"/BSt 0 d",				// brush style
"/LWi 1 d",				// line width
"/PSt 1 d",				// pen style
"/Cx  0 d",				// current x position
"/Cy  0 d",				// current y position
"/WFi false d",			// winding fill
"/OMo false d",			// opaque mode (not transparent)
"",
"/BCol  [ 1 1 1 ] d",			// brush color
"/PCol  [ 0 0 0 ] d",			// pen color
"/BkCol [ 1 1 1 ] d",			// background color
"",
"/nS 0 d",				// number of saved painter states
"",
"/LArr[",					// Pen styles:
"    []		     []",			//   solid line
"    [ 10 3 ]	     [ 3 10 ]",			//   dash line
"    [ 3 3 ]	     [ 3 3 ]",			//   dot line
"    [ 5 3 3 3 ]	     [ 3 5 3 3 ]",	//   dash dot line
"    [ 5 3 3 3 3 3 ]  [ 3 5 3 3 3 3 ]",		//   dash dot dot line
"] d",
"",
"",//
"",// Returns the line pattern (from pen style PSt).
"",//
"",// Argument:
"",//   bool pattern
"",//	true : draw pattern
"",//	false: fill pattern
"",//
"",
"/GPS {",
"  PSt 1 ge PSt 5 le and",			// valid pen pattern?
"    { { LArr PSt 1 sub 2 mul get }",		// draw pattern
"      { LArr PSt 2 mul 1 sub get } ifelse",    // opaque pattern
"    }",
"    { [] } ifelse",				// out of range => solid line
"} D",
"",
"/QS {",				// stroke command
"    PSt 0 ne",				// != NO_PEN
"    { LWi SW",				// set line width
"      gsave",
"      PCol SC",			// set pen color
"      true GPS 0 setdash S",		// draw line pattern
"      OMo PSt 1 ne and",		// opaque mode and not solid line?
"      { grestore BkCol SC",
"	false GPS dup 0 get setdash S",	// fill in opaque pattern
"      }",
"      { grestore } ifelse",
"    } if",
"} D",
"",
"/BDArr[",				// Brush dense patterns:
"    0.06 0.12 0.37 0.50 0.63 0.88 0.94",
"] d",
"",

"", // read 28 bits and leave them on tos
"/r28 {",
"  ", // skip past whitespace and read one character
"  { currentfile read pop ",
"    dup 32 gt { exit } if",
"    pop",
"  } loop",
"  ", // read three more
"  3 {",
"    currentfile read pop",
"  } repeat",
"  ", // make an accumulator
"  0",
"  ", // for each character, shift the accumulator and add in the character
"  4 {",
"    7 bitshift exch",
"    dup 128 gt { 84 sub } if 42 sub 127 and",
"    add",
"  } repeat",
"} D",
"",
"", // read some bits and leave them on tos
"/rA 0 d ", // accumulator
"/rL 0 d ", // bits left
"",
"", // takes number of bits, leaves number
"/rB {",
"  rL 0 eq {",
"    ", // if we have nothing, let's get something
"    /rA r28 d",
"    /rL 28 d",
"  } if",
"  dup rL gt {",
"    ", // if we don't have enough, take what we have and get more
"    rA exch rL sub rL exch",
"    /rA 0 d /rL 0 d",
"    rB exch bitshift add",
"  } {",
"    ", // else take some of what we have
"    dup rA 16#fffffff 3 -1 roll bitshift not and exch",
"    ", // ... and update rL and rA
"    dup rL exch sub /rL ED",
"    neg rA exch bitshift /rA ED",
"  } ifelse",
"} D",
"",
"", // uncompresses from currentfile until the string on the stack is full;
"", // leaves the string there.  assumes that nothing could conceivably go
"", // wrong.
"/rC {",
"  /rL 0 d",
"  0",
"  {", // string pos
"    dup 2 index length ge { exit } if",
"    1 rB",
"    1 eq {", // compressed
"      3 rB", // string pos bits
"      dup 4 ge {",
"        dup rB", // string pos bits extra
"        1 index 5 ge {",
"          1 index 6 ge {",
"            1 index 7 ge {",
"              64 add",
"            } if",
"            32 add",
"          } if",
"          16 add",
"        } if",
"        4 add",
"	exch pop",
"      } if",
"      1 add 3 mul",
"      ", // string pos length
"      exch 10 rB 1 add 3 mul",
"      ", // string length pos dist
"      {",
"	dup 3 index lt {",
"	  dup",
"	} {",
"	  2 index",
"	} ifelse", // string length pos dist length-this-time
"	4 index 3 index 3 index sub 2 index getinterval",
"	5 index 4 index 3 -1 roll putinterval",
"	dup 4 -1 roll add 3 1 roll",
"	4 -1 roll exch sub ",
"	dup 0 eq { exit } if",
"	3 1 roll",
"      } loop", // string pos dist length
"      pop pop",
"    } {", // uncompressed
"      3 rB 1 add 3 mul",
"      {",
"	2 copy 8 rB put 1 add",
"      } repeat",
"    } ifelse",
"  } loop",
"  pop",
"} D",
"",
"/sl D0", // slow implementation, but strippable by stripHeader
"/QCIgray D0 /QCIcolor D0 /QCIindex D0",
"/QCI {", // as colorimage but without the last two arguments
"  /colorimage where {",
"    pop",
"    false 3 colorimage",
"  }{",  // the hard way, based on PD code by John Walker <kelvin@autodesk.com>
"    exec /QCIcolor ED",
"    /QCIgray QCIcolor length 3 idiv string d",
"    0 1 QCIcolor length 3 idiv 1 sub",
"    { /QCIindex ED",
"      /x QCIindex 3 mul d",
"      QCIgray QCIindex",
"      QCIcolor x       get 0.30 mul",
"      QCIcolor x 1 add get 0.59 mul",
"      QCIcolor x 2 add get 0.11 mul",
"      add add cvi",
"      put",
"    } for",
"    QCIgray image",
"  } ifelse",
"} D",
"",
"/defM matrix d",
"",
"/BF {",				// brush fill
"   gsave",
"   BSt 1 eq",				// solid brush?
"   {",
"     BCol SC"
"     WFi { fill } { eofill } ifelse",
"   } if",
"   BSt 2 ge BSt 8 le and",		// dense pattern?
"   {",
"     BDArr BSt 2 sub get setgray fill"
"   } if",
"   BSt 9 ge BSt 14 le and",		// brush pattern?
"   {",
"     WFi { clip } { eoclip } ifelse",
"     defM SM",
"     pathbbox",			// left upper right lower
"     3 index 3 index translate",
"     4 2 roll",			// right lower left upper
"     3 2 roll",			// right left upper lower
"     exch",				// left right lower upper
"     sub /h ED",
"     sub /w ED",
"     OMo {",
"	  NP",
"	  0 0 MT",
"	  0 h RL",
"	  w 0 RL",
"	  0 h neg RL",
"	  CP",
"	  BkCol SC",
"	  fill",
"     } if",
"     BCol SC",
"     0.3 SW",
"     NP",
"     BSt 9 eq BSt 11 eq or",		// horiz or cross pattern
"     { 0 4 h",
"       { dup 0 exch MT w exch LT } for",
"     } if",
"     BSt 10 eq BSt 11 eq or",		// vert or cross pattern
"     { 0 4 w",
"       { dup 0 MT h LT } for",
"     } if",
"     BSt 12 eq BSt 14 eq or",		// F-diag or diag cross
"     { w h gt",
"       { 0 6 w h add",
"	  { dup 0 MT h sub h LT } for"
"       } { 0 6 w h add",
"         { dup 0 exch MT w sub w exch LT } for"
"       } ifelse",
"     } if",
"     BSt 13 eq BSt 14 eq or",		// B-diag or diag cross
"     { w h gt",
"       { 0 6 w h add",
"	  { dup h MT h sub 0 LT } for",
"       } { 0 6 w h add",
"	  { dup w exch MT w sub 0 exch LT } for"
"       } ifelse",
"     } if",
"     S",
"   } if",
"   BSt 24 eq",				// CustomPattern
"     ",
"   {",
"   } if",
"   grestore",
"} D",
"",
"",
"", // for arc
"/mat matrix d",
"/ang1 D0 /ang2 D0",
"/w D0 /h D0",
"/x D0 /y D0",
"",
"/ARC {",				// Generic ARC function [ X Y W H ang1 ang2 ]
"    /ang2 ED /ang1 ED /h ED /w ED /y ED /x ED",
"    mat CM pop",
"    x w 2 div add y h 2 div add TR",
"    1 h w div neg scale",
"    ang2 0 ge",
"    {0 0 w 2 div ang1 ang1 ang2 add arc }",
"    {0 0 w 2 div ang1 ang1 ang2 add arcn} ifelse",
"    mat SM",
"} D",
"",
"",
"/C D0",
"",
"/P {",					// PdcDrawPoint [x y]
"    NP",
"    MT",
"    0.5 0.5 rmoveto",
"    0  -1 RL",
"    -1	0 RL",
"    0	1 RL",
"    CP",
"    PCol SC",
"    fill",
"} D",
"",
"/M {",					// PdcMoveTo [x y]
"    /Cy ED /Cx ED",
"} D",
"",
"/L {",					// PdcLineTo [x y]
"    NP",
"    Cx Cy MT",
"    /Cy ED /Cx ED",
"    Cx Cy LT",
"    QS",
"} D",
"",
"/DL {",				// PdcDrawLine [x1 y1 x0 y0]
"    NP",
"    MT",
"    LT",
"    QS",
"} D",
"",
"/HL {",				// PdcDrawLine [x1 y x0]
"    1 index DL",
"} D",
"",
"/VL {",				// PdcDrawLine [x y1 y0]
"    2 index exch DL",
"} D",
"",
"/R {",					// PdcDrawRect [x y w h]
"    /h ED /w ED /y ED /x ED",
"    NP",
"    x y MT",
"    0 h RL",
"    w 0 RL",
"    0 h neg RL",
"    CP",
"    BF",
"    QS",
"} D",
"",
"/ACR {",					// add clip rect
"    /h ED /w ED /y ED /x ED",
"    x y MT",
"    0 h RL",
"    w 0 RL",
"    0 h neg RL",
"    CP",
"} D",
"",
"/CLSTART {",				// clipping start
"    /clipTmp matrix CM d",		// save current matrix
"    defM SM",				// Page default matrix
"    NP",
"} D",
"",
"/CLEND {",				// clipping end
"    clip",
"    NP",
"    clipTmp SM",			// restore the current matrix
"} D",
"",
"/CLO {",				// clipping off
"    grestore",				// restore top of page state
"    gsave",				// save it back again
"    defM SM",				// set coordsys (defensive progr.)
"} D",
"",
"/xr D0 /yr D0",
"/rx D0 /ry D0 /rx2 D0 /ry2 D0",
"",
"/RR {",				// PdcDrawRoundRect [x y w h xr yr]
"    /yr ED /xr ED /h ED /w ED /y ED /x ED",
"    xr 0 le yr 0 le or",
"    {x y w h R}",		    // Do rect if one of rounding values is less than 0.
"    {xr 100 ge yr 100 ge or",
"	{x y w h E}",			 // Do ellipse if both rounding values are larger than 100
"	{",
"	 /rx xr w mul 200 div d",
"	 /ry yr h mul 200 div d",
"	 /rx2 rx 2 mul d",
"	 /ry2 ry 2 mul d",
"	 NP",
"	 x rx add y MT",
"	 x		 y		 rx2 ry2 180 -90",
"	 x		 y h add ry2 sub rx2 ry2 270 -90",
"	 x w add rx2 sub y h add ry2 sub rx2 ry2 0   -90",
"	 x w add rx2 sub y		 rx2 ry2 90  -90",
"	 ARC ARC ARC ARC",
"	 CP",
"	 BF",
"	 QS",
"	} ifelse",
"    } ifelse",
"} D",
"",
"",
"/E {",				// PdcDrawEllipse [x y w h]
"    /h ED /w ED /y ED /x ED",
"    mat CM pop",
"    x w 2 div add y h 2 div add translate",
"    1 h w div scale",
"    NP",
"    0 0 w 2 div 0 360 arc",
"    mat SM",
"    BF",
"    QS",
"} D",
"",
"",
"/A {",				// PdcDrawArc [x y w h ang1 ang2]
"    16 div exch 16 div exch",
"    NP",
"    ARC",
"    QS",
"} D",
"",
"",
"/PIE {",				// PdcDrawPie [x y w h ang1 ang2]
"    /ang2 ED /ang1 ED /h ED /w ED /y ED /x ED",
"    NP",
"    x w 2 div add y h 2 div add MT",
"    x y w h ang1 16 div ang2 16 div ARC",
"    CP",
"    BF",
"    QS",
"} D",
"",
"/CH {",				// PdcDrawChord [x y w h ang1 ang2]
"    16 div exch 16 div exch",
"    NP",
"    ARC",
"    CP",
"    BF",
"    QS",
"} D",
"",
"",
"/BZ {",				// PdcDrawQuadBezier [3 points]
"    curveto",
"    QS",
"} D",
"",
"",
"/CRGB {",				// Compute RGB [R G B] => R/255 G/255 B/255
"    255 div 3 1 roll",
"    255 div 3 1 roll",
"    255 div 3 1 roll",
"} D",
"",
"",
"/SV {",				// Save painter state
"    BSt LWi PSt Cx Cy WFi OMo BCol PCol BkCol",
"    /nS nS 1 add d",
"    gsave",
"} D",
"",
"/RS {",				// Restore painter state
"    nS 0 gt",
"    { grestore",
"      /BkCol ED /PCol ED /BCol ED /OMo ED /WFi ED",
"      /Cy ED /Cx ED /PSt ED /LWi ED /BSt ED",
"      /nS nS 1 sub d",
"    } if",
"",
"} D",
"",
"/BC {",				// PdcSetBkColor [R G B]
"    CRGB",
"    BkCol astore pop",
"} D",
"",
"/BR {",				// PdcSetBrush [style R G B]
"    CRGB",
"    BCol astore pop",
"    /BSt ED",
"} D",
"",
"/WB {",				// set white solid brush
"    1 W BR",
"} D",
"",
"/NB {",				// set nobrush
"    0 B BR",
"} D",
"",
"/PE {", // PdcSetPen [style width R G B Cap Join]
"    setlinejoin setlinecap"
"    CRGB",
"    PCol astore pop",
"    /LWi ED",
"    /PSt ED",
"    LWi 0 eq { 0.25 /LWi ED } if", // ### 3.0 remove this line
"} D",
"",
"/P1 {",				// PdcSetPen [R G B]
"    1 0 5 2 roll 0 0 PE",
"} D",
"",
"/ST {",				// SET TRANSFORM [matrix]
"    defM setmatrix",
"    concat",
"} D",
"",
"",
// this function tries to find a suitable postscript font. We try quite hard not to get courier for a 
// proportional font. The following takes an array of fonts. The algorithm will take the first one that
// gives a match (defined as not resulting in a courier font). 
// each entry in the table is an array of the form [ /Fontname x-stretch slant ]
// x-strtch can be used to stretch/squeeze the font in x direction. This gives better results when
// eg substituting helvetica for arial
// slant is an optional slant. 0 is non slanted, 0.2 is a typical value for a syntetic oblique.
// encoding can be either an encoding vector of false if the default font encoding is requested.
"/qtfindfont {",	       		// fntlist
"  true exch",  			// push a dummy on the stack, 
"  {", 				// so the loop over the array will leave a font in any case when exiting.
"    exch pop",			// (dummy | oldfont) fontarray
"    dup 0 get dup findfont",	       	// get the fontname from the array and load it
"    dup /FontName get",		// see if the font exists
"    3 -1 roll eq {",			// see if fontname and the one provided are equal
"      exit",			  
"    } {",
"      pop",
"    } ifelse",
"  } forall",
"  exch",				// font fontarray
"} d",
"",
"/qtdefinefont {",			// newname encoding font fontarray 	defines a postscript font
"  dup",
"  1 get /fxscale exch def",		// define scale, sland and encoding
"  2 get /fslant exch def",
"  exch /fencoding exch def",
"  [ fxscale 0 fslant 1 0 0 ] makefont",	// transform font accordingly
"  fencoding false eq {", 		// check if we have an encoding and use it if available
"  } {",
"    dup maxlength dict begin",		// copy font
"    {",
"      1 index /FID ne",			// don't copy FID, as it's not allowed in PS Level 1
"      {def}{pop pop}ifelse",
"    } forall",
"    /Encoding fencoding def",		// replace encoding
"    currentdict end",
"  } ifelse",
"  definefont pop",
"} d",

"/MF {",				// newname encoding fontlist
"  qtfindfont qtdefinefont",
"} D",
"",
"/MSF {",				// newname slant fontname (this is used for asian fonts, where we don't need an encoding)
"  findfont exch",			// newname font slant
"  /slant exch d",			// newname font
"  [ 1 0 slant 1 0 0 ] makefont",
"  definefont pop",
"} D",
"",
"/DF {",				// newname pointsize fontmame
"  findfont",
"  /FONTSIZE 3 -1 roll d [ FONTSIZE 0 0 FONTSIZE -1 mul 0 0 ] makefont",
"  d",
"} D",
"",
"",
"/ty 0 d",
"/Y {",
"    /ty ED",
"} D",
"",
"/Tl {", // draw underline/strikeout line: () w x y ->Tl-> () w x
"    NP 1 index exch MT",
"    1 index 0 rlineto QS",
"} D",
"",
"/T {",					// PdcDrawText2 [string fm.width x]
"    PCol SC", // really need to kill these SCs
"    ty MT",
"    1 index", // string cwidth string
"    dup length exch", // string cwidth length string
"    stringwidth pop", // string cwidth length pwidth
"    3 -1 roll", // string length pwidth cwidth
"    exch sub exch div", // string extraperchar
"    exch 0 exch", // extraperchar 0 string
"    ashow",
"} D",
"",
"/QI {",
"    /C save d",
"    pageinit",
"    /Cx  0 d",				// reset current x position
"    /Cy  0 d",				// reset current y position
"    /OMo false d",
"} D",
"",
"/QP {",				// show page
"    C restore",
"    showpage",

"} D",
0 };



// the next table is derived from a list provided by Adobe on its web
// server: http://partners.adobe.com/asn/developer/typeforum/glyphlist.txt

// the start of the header comment:
//
// Name:          Adobe Glyph List
// Table version: 1.2
// Date:          22 Oct 1998
//
// Description:
//
//   The Adobe Glyph List (AGL) list relates Unicode values (UVs) to glyph
//   names, and should be used only as described in the document "Unicode and
//   Glyph Names," at
//   http://www.adobe.com/asn/developer/typeforum/unicodegn.html .


static const struct {
    Q_UINT16 u;
    const char * g;
} unicodetoglyph[] = {
    // grep '^[0-9A-F][0-9A-F][0-9A-F][0-9A-F];' < /tmp/glyphlist.txt | sed -e 's/;/, "/' -e 's-;-" },  // -' -e 's/^/    { 0x/' | sort
    { 0x0020, "space" },  // SPACE
    { 0x0021, "exclam" },  // EXCLAMATION MARK
    { 0x0022, "quotedbl" },  // QUOTATION MARK
    { 0x0023, "numbersign" },  // NUMBER SIGN
    { 0x0024, "dollar" },  // DOLLAR SIGN
    { 0x0025, "percent" },  // PERCENT SIGN
    { 0x0026, "ampersand" },  // AMPERSAND
    { 0x0027, "quotesingle" },  // APOSTROPHE
    { 0x0028, "parenleft" },  // LEFT PARENTHESIS
    { 0x0029, "parenright" },  // RIGHT PARENTHESIS
    { 0x002A, "asterisk" },  // ASTERISK
    { 0x002B, "plus" },  // PLUS SIGN
    { 0x002C, "comma" },  // COMMA
    { 0x002D, "hyphen" },  // HYPHEN-MINUS
    { 0x002E, "period" },  // FULL STOP
    { 0x002F, "slash" },  // SOLIDUS
    { 0x0030, "zero" },  // DIGIT ZERO
    { 0x0031, "one" },  // DIGIT ONE
    { 0x0032, "two" },  // DIGIT TWO
    { 0x0033, "three" },  // DIGIT THREE
    { 0x0034, "four" },  // DIGIT FOUR
    { 0x0035, "five" },  // DIGIT FIVE
    { 0x0036, "six" },  // DIGIT SIX
    { 0x0037, "seven" },  // DIGIT SEVEN
    { 0x0038, "eight" },  // DIGIT EIGHT
    { 0x0039, "nine" },  // DIGIT NINE
    { 0x003A, "colon" },  // COLON
    { 0x003B, "semicolon" },  // SEMICOLON
    { 0x003C, "less" },  // LESS-THAN SIGN
    { 0x003D, "equal" },  // EQUALS SIGN
    { 0x003E, "greater" },  // GREATER-THAN SIGN
    { 0x003F, "question" },  // QUESTION MARK
    { 0x0040, "at" },  // COMMERCIAL AT
    { 0x0041, "A" },  // LATIN CAPITAL LETTER A
    { 0x0042, "B" },  // LATIN CAPITAL LETTER B
    { 0x0043, "C" },  // LATIN CAPITAL LETTER C
    { 0x0044, "D" },  // LATIN CAPITAL LETTER D
    { 0x0045, "E" },  // LATIN CAPITAL LETTER E
    { 0x0046, "F" },  // LATIN CAPITAL LETTER F
    { 0x0047, "G" },  // LATIN CAPITAL LETTER G
    { 0x0048, "H" },  // LATIN CAPITAL LETTER H
    { 0x0049, "I" },  // LATIN CAPITAL LETTER I
    { 0x004A, "J" },  // LATIN CAPITAL LETTER J
    { 0x004B, "K" },  // LATIN CAPITAL LETTER K
    { 0x004C, "L" },  // LATIN CAPITAL LETTER L
    { 0x004D, "M" },  // LATIN CAPITAL LETTER M
    { 0x004E, "N" },  // LATIN CAPITAL LETTER N
    { 0x004F, "O" },  // LATIN CAPITAL LETTER O
    { 0x0050, "P" },  // LATIN CAPITAL LETTER P
    { 0x0051, "Q" },  // LATIN CAPITAL LETTER Q
    { 0x0052, "R" },  // LATIN CAPITAL LETTER R
    { 0x0053, "S" },  // LATIN CAPITAL LETTER S
    { 0x0054, "T" },  // LATIN CAPITAL LETTER T
    { 0x0055, "U" },  // LATIN CAPITAL LETTER U
    { 0x0056, "V" },  // LATIN CAPITAL LETTER V
    { 0x0057, "W" },  // LATIN CAPITAL LETTER W
    { 0x0058, "X" },  // LATIN CAPITAL LETTER X
    { 0x0059, "Y" },  // LATIN CAPITAL LETTER Y
    { 0x005A, "Z" },  // LATIN CAPITAL LETTER Z
    { 0x005B, "bracketleft" },  // LEFT SQUARE BRACKET
    { 0x005C, "backslash" },  // REVERSE SOLIDUS
    { 0x005D, "bracketright" },  // RIGHT SQUARE BRACKET
    { 0x005E, "asciicircum" },  // CIRCUMFLEX ACCENT
    { 0x005F, "underscore" },  // LOW LINE
    { 0x0060, "grave" },  // GRAVE ACCENT
    { 0x0061, "a" },  // LATIN SMALL LETTER A
    { 0x0062, "b" },  // LATIN SMALL LETTER B
    { 0x0063, "c" },  // LATIN SMALL LETTER C
    { 0x0064, "d" },  // LATIN SMALL LETTER D
    { 0x0065, "e" },  // LATIN SMALL LETTER E
    { 0x0066, "f" },  // LATIN SMALL LETTER F
    { 0x0067, "g" },  // LATIN SMALL LETTER G
    { 0x0068, "h" },  // LATIN SMALL LETTER H
    { 0x0069, "i" },  // LATIN SMALL LETTER I
    { 0x006A, "j" },  // LATIN SMALL LETTER J
    { 0x006B, "k" },  // LATIN SMALL LETTER K
    { 0x006C, "l" },  // LATIN SMALL LETTER L
    { 0x006D, "m" },  // LATIN SMALL LETTER M
    { 0x006E, "n" },  // LATIN SMALL LETTER N
    { 0x006F, "o" },  // LATIN SMALL LETTER O
    { 0x0070, "p" },  // LATIN SMALL LETTER P
    { 0x0071, "q" },  // LATIN SMALL LETTER Q
    { 0x0072, "r" },  // LATIN SMALL LETTER R
    { 0x0073, "s" },  // LATIN SMALL LETTER S
    { 0x0074, "t" },  // LATIN SMALL LETTER T
    { 0x0075, "u" },  // LATIN SMALL LETTER U
    { 0x0076, "v" },  // LATIN SMALL LETTER V
    { 0x0077, "w" },  // LATIN SMALL LETTER W
    { 0x0078, "x" },  // LATIN SMALL LETTER X
    { 0x0079, "y" },  // LATIN SMALL LETTER Y
    { 0x007A, "z" },  // LATIN SMALL LETTER Z
    { 0x007B, "braceleft" },  // LEFT CURLY BRACKET
    { 0x007C, "bar" },  // VERTICAL LINE
    { 0x007D, "braceright" },  // RIGHT CURLY BRACKET
    { 0x007E, "asciitilde" },  // TILDE
    { 0x00A0, "space" },  // NO-BREAK SPACE;Duplicate
    { 0x00A1, "exclamdown" },  // INVERTED EXCLAMATION MARK
    { 0x00A2, "cent" },  // CENT SIGN
    { 0x00A3, "sterling" },  // POUND SIGN
    { 0x00A4, "currency" },  // CURRENCY SIGN
    { 0x00A5, "yen" },  // YEN SIGN
    { 0x00A6, "brokenbar" },  // BROKEN BAR
    { 0x00A7, "section" },  // SECTION SIGN
    { 0x00A8, "dieresis" },  // DIAERESIS
    { 0x00A9, "copyright" },  // COPYRIGHT SIGN
    { 0x00AA, "ordfeminine" },  // FEMININE ORDINAL INDICATOR
    { 0x00AB, "guillemotleft" },  // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    { 0x00AC, "logicalnot" },  // NOT SIGN
    { 0x00AD, "hyphen" },  // SOFT HYPHEN;Duplicate
    { 0x00AE, "registered" },  // REGISTERED SIGN
    { 0x00AF, "macron" },  // MACRON
    { 0x00B0, "degree" },  // DEGREE SIGN
    { 0x00B1, "plusminus" },  // PLUS-MINUS SIGN
    { 0x00B2, "twosuperior" },  // SUPERSCRIPT TWO
    { 0x00B3, "threesuperior" },  // SUPERSCRIPT THREE
    { 0x00B4, "acute" },  // ACUTE ACCENT
    { 0x00B5, "mu" },  // MICRO SIGN
    { 0x00B6, "paragraph" },  // PILCROW SIGN
    { 0x00B7, "periodcentered" },  // MIDDLE DOT
    { 0x00B8, "cedilla" },  // CEDILLA
    { 0x00B9, "onesuperior" },  // SUPERSCRIPT ONE
    { 0x00BA, "ordmasculine" },  // MASCULINE ORDINAL INDICATOR
    { 0x00BB, "guillemotright" }, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    { 0x00BC, "onequarter" },  // VULGAR FRACTION ONE QUARTER
    { 0x00BD, "onehalf" },  // VULGAR FRACTION ONE HALF
    { 0x00BE, "threequarters" },  // VULGAR FRACTION THREE QUARTERS
    { 0x00BF, "questiondown" },  // INVERTED QUESTION MARK
    { 0x00C0, "Agrave" },  // LATIN CAPITAL LETTER A WITH GRAVE
    { 0x00C1, "Aacute" },  // LATIN CAPITAL LETTER A WITH ACUTE
    { 0x00C2, "Acircumflex" },  // LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    { 0x00C3, "Atilde" },  // LATIN CAPITAL LETTER A WITH TILDE
    { 0x00C4, "Adieresis" },  // LATIN CAPITAL LETTER A WITH DIAERESIS
    { 0x00C5, "Aring" },  // LATIN CAPITAL LETTER A WITH RING ABOVE
    { 0x00C6, "AE" },  // LATIN CAPITAL LETTER AE
    { 0x00C7, "Ccedilla" },  // LATIN CAPITAL LETTER C WITH CEDILLA
    { 0x00C8, "Egrave" },  // LATIN CAPITAL LETTER E WITH GRAVE
    { 0x00C9, "Eacute" },  // LATIN CAPITAL LETTER E WITH ACUTE
    { 0x00CA, "Ecircumflex" },  // LATIN CAPITAL LETTER E WITH CIRCUMFLEX
    { 0x00CB, "Edieresis" },  // LATIN CAPITAL LETTER E WITH DIAERESIS
    { 0x00CC, "Igrave" },  // LATIN CAPITAL LETTER I WITH GRAVE
    { 0x00CD, "Iacute" },  // LATIN CAPITAL LETTER I WITH ACUTE
    { 0x00CE, "Icircumflex" },  // LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    { 0x00CF, "Idieresis" },  // LATIN CAPITAL LETTER I WITH DIAERESIS
    { 0x00D0, "Eth" },  // LATIN CAPITAL LETTER ETH
    { 0x00D1, "Ntilde" },  // LATIN CAPITAL LETTER N WITH TILDE
    { 0x00D2, "Ograve" },  // LATIN CAPITAL LETTER O WITH GRAVE
    { 0x00D3, "Oacute" },  // LATIN CAPITAL LETTER O WITH ACUTE
    { 0x00D4, "Ocircumflex" },  // LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    { 0x00D5, "Otilde" },  // LATIN CAPITAL LETTER O WITH TILDE
    { 0x00D6, "Odieresis" },  // LATIN CAPITAL LETTER O WITH DIAERESIS
    { 0x00D7, "multiply" },  // MULTIPLICATION SIGN
    { 0x00D8, "Oslash" },  // LATIN CAPITAL LETTER O WITH STROKE
    { 0x00D9, "Ugrave" },  // LATIN CAPITAL LETTER U WITH GRAVE
    { 0x00DA, "Uacute" },  // LATIN CAPITAL LETTER U WITH ACUTE
    { 0x00DB, "Ucircumflex" },  // LATIN CAPITAL LETTER U WITH CIRCUMFLEX
    { 0x00DC, "Udieresis" },  // LATIN CAPITAL LETTER U WITH DIAERESIS
    { 0x00DD, "Yacute" },  // LATIN CAPITAL LETTER Y WITH ACUTE
    { 0x00DE, "Thorn" },  // LATIN CAPITAL LETTER THORN
    { 0x00DF, "germandbls" },  // LATIN SMALL LETTER SHARP S
    { 0x00E0, "agrave" },  // LATIN SMALL LETTER A WITH GRAVE
    { 0x00E1, "aacute" },  // LATIN SMALL LETTER A WITH ACUTE
    { 0x00E2, "acircumflex" },  // LATIN SMALL LETTER A WITH CIRCUMFLEX
    { 0x00E3, "atilde" },  // LATIN SMALL LETTER A WITH TILDE
    { 0x00E4, "adieresis" },  // LATIN SMALL LETTER A WITH DIAERESIS
    { 0x00E5, "aring" },  // LATIN SMALL LETTER A WITH RING ABOVE
    { 0x00E6, "ae" },  // LATIN SMALL LETTER AE
    { 0x00E7, "ccedilla" },  // LATIN SMALL LETTER C WITH CEDILLA
    { 0x00E8, "egrave" },  // LATIN SMALL LETTER E WITH GRAVE
    { 0x00E9, "eacute" },  // LATIN SMALL LETTER E WITH ACUTE
    { 0x00EA, "ecircumflex" },  // LATIN SMALL LETTER E WITH CIRCUMFLEX
    { 0x00EB, "edieresis" },  // LATIN SMALL LETTER E WITH DIAERESIS
    { 0x00EC, "igrave" },  // LATIN SMALL LETTER I WITH GRAVE
    { 0x00ED, "iacute" },  // LATIN SMALL LETTER I WITH ACUTE
    { 0x00EE, "icircumflex" },  // LATIN SMALL LETTER I WITH CIRCUMFLEX
    { 0x00EF, "idieresis" },  // LATIN SMALL LETTER I WITH DIAERESIS
    { 0x00F0, "eth" },  // LATIN SMALL LETTER ETH
    { 0x00F1, "ntilde" },  // LATIN SMALL LETTER N WITH TILDE
    { 0x00F2, "ograve" },  // LATIN SMALL LETTER O WITH GRAVE
    { 0x00F3, "oacute" },  // LATIN SMALL LETTER O WITH ACUTE
    { 0x00F4, "ocircumflex" },  // LATIN SMALL LETTER O WITH CIRCUMFLEX
    { 0x00F5, "otilde" },  // LATIN SMALL LETTER O WITH TILDE
    { 0x00F6, "odieresis" },  // LATIN SMALL LETTER O WITH DIAERESIS
    { 0x00F7, "divide" },  // DIVISION SIGN
    { 0x00F8, "oslash" },  // LATIN SMALL LETTER O WITH STROKE
    { 0x00F9, "ugrave" },  // LATIN SMALL LETTER U WITH GRAVE
    { 0x00FA, "uacute" },  // LATIN SMALL LETTER U WITH ACUTE
    { 0x00FB, "ucircumflex" },  // LATIN SMALL LETTER U WITH CIRCUMFLEX
    { 0x00FC, "udieresis" },  // LATIN SMALL LETTER U WITH DIAERESIS
    { 0x00FD, "yacute" },  // LATIN SMALL LETTER Y WITH ACUTE
    { 0x00FE, "thorn" },  // LATIN SMALL LETTER THORN
    { 0x00FF, "ydieresis" },  // LATIN SMALL LETTER Y WITH DIAERESIS
    { 0x0100, "Amacron" },  // LATIN CAPITAL LETTER A WITH MACRON
    { 0x0101, "amacron" },  // LATIN SMALL LETTER A WITH MACRON
    { 0x0102, "Abreve" },  // LATIN CAPITAL LETTER A WITH BREVE
    { 0x0103, "abreve" },  // LATIN SMALL LETTER A WITH BREVE
    { 0x0104, "Aogonek" },  // LATIN CAPITAL LETTER A WITH OGONEK
    { 0x0105, "aogonek" },  // LATIN SMALL LETTER A WITH OGONEK
    { 0x0106, "Cacute" },  // LATIN CAPITAL LETTER C WITH ACUTE
    { 0x0107, "cacute" },  // LATIN SMALL LETTER C WITH ACUTE
    { 0x0108, "Ccircumflex" },  // LATIN CAPITAL LETTER C WITH CIRCUMFLEX
    { 0x0109, "ccircumflex" },  // LATIN SMALL LETTER C WITH CIRCUMFLEX
    { 0x010A, "Cdotaccent" },  // LATIN CAPITAL LETTER C WITH DOT ABOVE
    { 0x010B, "cdotaccent" },  // LATIN SMALL LETTER C WITH DOT ABOVE
    { 0x010C, "Ccaron" },  // LATIN CAPITAL LETTER C WITH CARON
    { 0x010D, "ccaron" },  // LATIN SMALL LETTER C WITH CARON
    { 0x010E, "Dcaron" },  // LATIN CAPITAL LETTER D WITH CARON
    { 0x010F, "dcaron" },  // LATIN SMALL LETTER D WITH CARON
    { 0x0110, "Dcroat" },  // LATIN CAPITAL LETTER D WITH STROKE
    { 0x0111, "dcroat" },  // LATIN SMALL LETTER D WITH STROKE
    { 0x0112, "Emacron" },  // LATIN CAPITAL LETTER E WITH MACRON
    { 0x0113, "emacron" },  // LATIN SMALL LETTER E WITH MACRON
    { 0x0114, "Ebreve" },  // LATIN CAPITAL LETTER E WITH BREVE
    { 0x0115, "ebreve" },  // LATIN SMALL LETTER E WITH BREVE
    { 0x0116, "Edotaccent" },  // LATIN CAPITAL LETTER E WITH DOT ABOVE
    { 0x0117, "edotaccent" },  // LATIN SMALL LETTER E WITH DOT ABOVE
    { 0x0118, "Eogonek" },  // LATIN CAPITAL LETTER E WITH OGONEK
    { 0x0119, "eogonek" },  // LATIN SMALL LETTER E WITH OGONEK
    { 0x011A, "Ecaron" },  // LATIN CAPITAL LETTER E WITH CARON
    { 0x011B, "ecaron" },  // LATIN SMALL LETTER E WITH CARON
    { 0x011C, "Gcircumflex" },  // LATIN CAPITAL LETTER G WITH CIRCUMFLEX
    { 0x011D, "gcircumflex" },  // LATIN SMALL LETTER G WITH CIRCUMFLEX
    { 0x011E, "Gbreve" },  // LATIN CAPITAL LETTER G WITH BREVE
    { 0x011F, "gbreve" },  // LATIN SMALL LETTER G WITH BREVE
    { 0x0120, "Gdotaccent" },  // LATIN CAPITAL LETTER G WITH DOT ABOVE
    { 0x0121, "gdotaccent" },  // LATIN SMALL LETTER G WITH DOT ABOVE
    { 0x0122, "Gcommaaccent" },  // LATIN CAPITAL LETTER G WITH CEDILLA
    { 0x0123, "gcommaaccent" },  // LATIN SMALL LETTER G WITH CEDILLA
    { 0x0124, "Hcircumflex" },  // LATIN CAPITAL LETTER H WITH CIRCUMFLEX
    { 0x0125, "hcircumflex" },  // LATIN SMALL LETTER H WITH CIRCUMFLEX
    { 0x0126, "Hbar" },  // LATIN CAPITAL LETTER H WITH STROKE
    { 0x0127, "hbar" },  // LATIN SMALL LETTER H WITH STROKE
    { 0x0128, "Itilde" },  // LATIN CAPITAL LETTER I WITH TILDE
    { 0x0129, "itilde" },  // LATIN SMALL LETTER I WITH TILDE
    { 0x012A, "Imacron" },  // LATIN CAPITAL LETTER I WITH MACRON
    { 0x012B, "imacron" },  // LATIN SMALL LETTER I WITH MACRON
    { 0x012C, "Ibreve" },  // LATIN CAPITAL LETTER I WITH BREVE
    { 0x012D, "ibreve" },  // LATIN SMALL LETTER I WITH BREVE
    { 0x012E, "Iogonek" },  // LATIN CAPITAL LETTER I WITH OGONEK
    { 0x012F, "iogonek" },  // LATIN SMALL LETTER I WITH OGONEK
    { 0x0130, "Idotaccent" },  // LATIN CAPITAL LETTER I WITH DOT ABOVE
    { 0x0131, "dotlessi" },  // LATIN SMALL LETTER DOTLESS I
    { 0x0132, "IJ" },  // LATIN CAPITAL LIGATURE IJ
    { 0x0133, "ij" },  // LATIN SMALL LIGATURE IJ
    { 0x0134, "Jcircumflex" },  // LATIN CAPITAL LETTER J WITH CIRCUMFLEX
    { 0x0135, "jcircumflex" },  // LATIN SMALL LETTER J WITH CIRCUMFLEX
    { 0x0136, "Kcommaaccent" },  // LATIN CAPITAL LETTER K WITH CEDILLA
    { 0x0137, "kcommaaccent" },  // LATIN SMALL LETTER K WITH CEDILLA
    { 0x0138, "kgreenlandic" },  // LATIN SMALL LETTER KRA
    { 0x0139, "Lacute" },  // LATIN CAPITAL LETTER L WITH ACUTE
    { 0x013A, "lacute" },  // LATIN SMALL LETTER L WITH ACUTE
    { 0x013B, "Lcommaaccent" },  // LATIN CAPITAL LETTER L WITH CEDILLA
    { 0x013C, "lcommaaccent" },  // LATIN SMALL LETTER L WITH CEDILLA
    { 0x013D, "Lcaron" },  // LATIN CAPITAL LETTER L WITH CARON
    { 0x013E, "lcaron" },  // LATIN SMALL LETTER L WITH CARON
    { 0x013F, "Ldot" },  // LATIN CAPITAL LETTER L WITH MIDDLE DOT
    { 0x0140, "ldot" },  // LATIN SMALL LETTER L WITH MIDDLE DOT
    { 0x0141, "Lslash" },  // LATIN CAPITAL LETTER L WITH STROKE
    { 0x0142, "lslash" },  // LATIN SMALL LETTER L WITH STROKE
    { 0x0143, "Nacute" },  // LATIN CAPITAL LETTER N WITH ACUTE
    { 0x0144, "nacute" },  // LATIN SMALL LETTER N WITH ACUTE
    { 0x0145, "Ncommaaccent" },  // LATIN CAPITAL LETTER N WITH CEDILLA
    { 0x0146, "ncommaaccent" },  // LATIN SMALL LETTER N WITH CEDILLA
    { 0x0147, "Ncaron" },  // LATIN CAPITAL LETTER N WITH CARON
    { 0x0148, "ncaron" },  // LATIN SMALL LETTER N WITH CARON
    { 0x0149, "napostrophe" },  // LATIN SMALL LETTER N PRECEDED BY APOSTROPHE
    { 0x014A, "Eng" },  // LATIN CAPITAL LETTER ENG
    { 0x014B, "eng" },  // LATIN SMALL LETTER ENG
    { 0x014C, "Omacron" },  // LATIN CAPITAL LETTER O WITH MACRON
    { 0x014D, "omacron" },  // LATIN SMALL LETTER O WITH MACRON
    { 0x014E, "Obreve" },  // LATIN CAPITAL LETTER O WITH BREVE
    { 0x014F, "obreve" },  // LATIN SMALL LETTER O WITH BREVE
    { 0x0150, "Ohungarumlaut" },  // LATIN CAPITAL LETTER O WITH DOUBLE ACUTE
    { 0x0151, "ohungarumlaut" },  // LATIN SMALL LETTER O WITH DOUBLE ACUTE
    { 0x0152, "OE" },  // LATIN CAPITAL LIGATURE OE
    { 0x0153, "oe" },  // LATIN SMALL LIGATURE OE
    { 0x0154, "Racute" },  // LATIN CAPITAL LETTER R WITH ACUTE
    { 0x0155, "racute" },  // LATIN SMALL LETTER R WITH ACUTE
    { 0x0156, "Rcommaaccent" },  // LATIN CAPITAL LETTER R WITH CEDILLA
    { 0x0157, "rcommaaccent" },  // LATIN SMALL LETTER R WITH CEDILLA
    { 0x0158, "Rcaron" },  // LATIN CAPITAL LETTER R WITH CARON
    { 0x0159, "rcaron" },  // LATIN SMALL LETTER R WITH CARON
    { 0x015A, "Sacute" },  // LATIN CAPITAL LETTER S WITH ACUTE
    { 0x015B, "sacute" },  // LATIN SMALL LETTER S WITH ACUTE
    { 0x015C, "Scircumflex" },  // LATIN CAPITAL LETTER S WITH CIRCUMFLEX
    { 0x015D, "scircumflex" },  // LATIN SMALL LETTER S WITH CIRCUMFLEX
    { 0x015E, "Scedilla" },  // LATIN CAPITAL LETTER S WITH CEDILLA
    { 0x015F, "scedilla" },  // LATIN SMALL LETTER S WITH CEDILLA
    { 0x0160, "Scaron" },  // LATIN CAPITAL LETTER S WITH CARON
    { 0x0161, "scaron" },  // LATIN SMALL LETTER S WITH CARON
    { 0x0162, "Tcommaaccent" },  // LATIN CAPITAL LETTER T WITH CEDILLA
    { 0x0163, "tcommaaccent" },  // LATIN SMALL LETTER T WITH CEDILLA
    { 0x0164, "Tcaron" },  // LATIN CAPITAL LETTER T WITH CARON
    { 0x0165, "tcaron" },  // LATIN SMALL LETTER T WITH CARON
    { 0x0166, "Tbar" },  // LATIN CAPITAL LETTER T WITH STROKE
    { 0x0167, "tbar" },  // LATIN SMALL LETTER T WITH STROKE
    { 0x0168, "Utilde" },  // LATIN CAPITAL LETTER U WITH TILDE
    { 0x0169, "utilde" },  // LATIN SMALL LETTER U WITH TILDE
    { 0x016A, "Umacron" },  // LATIN CAPITAL LETTER U WITH MACRON
    { 0x016B, "umacron" },  // LATIN SMALL LETTER U WITH MACRON
    { 0x016C, "Ubreve" },  // LATIN CAPITAL LETTER U WITH BREVE
    { 0x016D, "ubreve" },  // LATIN SMALL LETTER U WITH BREVE
    { 0x016E, "Uring" },  // LATIN CAPITAL LETTER U WITH RING ABOVE
    { 0x016F, "uring" },  // LATIN SMALL LETTER U WITH RING ABOVE
    { 0x0170, "Uhungarumlaut" },  // LATIN CAPITAL LETTER U WITH DOUBLE ACUTE
    { 0x0171, "uhungarumlaut" },  // LATIN SMALL LETTER U WITH DOUBLE ACUTE
    { 0x0172, "Uogonek" },  // LATIN CAPITAL LETTER U WITH OGONEK
    { 0x0173, "uogonek" },  // LATIN SMALL LETTER U WITH OGONEK
    { 0x0174, "Wcircumflex" },  // LATIN CAPITAL LETTER W WITH CIRCUMFLEX
    { 0x0175, "wcircumflex" },  // LATIN SMALL LETTER W WITH CIRCUMFLEX
    { 0x0176, "Ycircumflex" },  // LATIN CAPITAL LETTER Y WITH CIRCUMFLEX
    { 0x0177, "ycircumflex" },  // LATIN SMALL LETTER Y WITH CIRCUMFLEX
    { 0x0178, "Ydieresis" },  // LATIN CAPITAL LETTER Y WITH DIAERESIS
    { 0x0179, "Zacute" },  // LATIN CAPITAL LETTER Z WITH ACUTE
    { 0x017A, "zacute" },  // LATIN SMALL LETTER Z WITH ACUTE
    { 0x017B, "Zdotaccent" },  // LATIN CAPITAL LETTER Z WITH DOT ABOVE
    { 0x017C, "zdotaccent" },  // LATIN SMALL LETTER Z WITH DOT ABOVE
    { 0x017D, "Zcaron" },  // LATIN CAPITAL LETTER Z WITH CARON
    { 0x017E, "zcaron" },  // LATIN SMALL LETTER Z WITH CARON
    { 0x017F, "longs" },  // LATIN SMALL LETTER LONG S
    { 0x0192, "florin" },  // LATIN SMALL LETTER F WITH HOOK
    { 0x01A0, "Ohorn" },  // LATIN CAPITAL LETTER O WITH HORN
    { 0x01A1, "ohorn" },  // LATIN SMALL LETTER O WITH HORN
    { 0x01AF, "Uhorn" },  // LATIN CAPITAL LETTER U WITH HORN
    { 0x01B0, "uhorn" },  // LATIN SMALL LETTER U WITH HORN
    { 0x01E6, "Gcaron" },  // LATIN CAPITAL LETTER G WITH CARON
    { 0x01E7, "gcaron" },  // LATIN SMALL LETTER G WITH CARON
    { 0x01FA, "Aringacute" },  // LATIN CAPITAL LETTER A WITH RING ABOVE AND ACUTE
    { 0x01FB, "aringacute" },  // LATIN SMALL LETTER A WITH RING ABOVE AND ACUTE
    { 0x01FC, "AEacute" },  // LATIN CAPITAL LETTER AE WITH ACUTE
    { 0x01FD, "aeacute" },  // LATIN SMALL LETTER AE WITH ACUTE
    { 0x01FE, "Oslashacute" },  // LATIN CAPITAL LETTER O WITH STROKE AND ACUTE
    { 0x01FF, "oslashacute" },  // LATIN SMALL LETTER O WITH STROKE AND ACUTE
    { 0x0218, "Scommaaccent" },  // LATIN CAPITAL LETTER S WITH COMMA BELOW
    { 0x0219, "scommaaccent" },  // LATIN SMALL LETTER S WITH COMMA BELOW
    { 0x021A, "Tcommaaccent" },  // LATIN CAPITAL LETTER T WITH COMMA BELOW;Duplicate
    { 0x021B, "tcommaaccent" },  // LATIN SMALL LETTER T WITH COMMA BELOW;Duplicate
    { 0x02BC, "afii57929" },  // MODIFIER LETTER APOSTROPHE
    { 0x02BD, "afii64937" },  // MODIFIER LETTER REVERSED COMMA
    { 0x02C6, "circumflex" },  // MODIFIER LETTER CIRCUMFLEX ACCENT
    { 0x02C7, "caron" },  // CARON
    { 0x02C9, "macron" },  // MODIFIER LETTER MACRON;Duplicate
    { 0x02D8, "breve" },  // BREVE
    { 0x02D9, "dotaccent" },  // DOT ABOVE
    { 0x02DA, "ring" },  // RING ABOVE
    { 0x02DB, "ogonek" },  // OGONEK
    { 0x02DC, "tilde" },  // SMALL TILDE
    { 0x02DD, "hungarumlaut" },  // DOUBLE ACUTE ACCENT
    { 0x0300, "gravecomb" },  // COMBINING GRAVE ACCENT
    { 0x0301, "acutecomb" },  // COMBINING ACUTE ACCENT
    { 0x0303, "tildecomb" },  // COMBINING TILDE
    { 0x0309, "hookabovecomb" },  // COMBINING HOOK ABOVE
    { 0x0323, "dotbelowcomb" },  // COMBINING DOT BELOW
    { 0x0384, "tonos" },  // GREEK TONOS
    { 0x0385, "dieresistonos" },  // GREEK DIALYTIKA TONOS
    { 0x0386, "Alphatonos" },  // GREEK CAPITAL LETTER ALPHA WITH TONOS
    { 0x0387, "anoteleia" },  // GREEK ANO TELEIA
    { 0x0388, "Epsilontonos" },  // GREEK CAPITAL LETTER EPSILON WITH TONOS
    { 0x0389, "Etatonos" },  // GREEK CAPITAL LETTER ETA WITH TONOS
    { 0x038A, "Iotatonos" },  // GREEK CAPITAL LETTER IOTA WITH TONOS
    { 0x038C, "Omicrontonos" },  // GREEK CAPITAL LETTER OMICRON WITH TONOS
    { 0x038E, "Upsilontonos" },  // GREEK CAPITAL LETTER UPSILON WITH TONOS
    { 0x038F, "Omegatonos" },  // GREEK CAPITAL LETTER OMEGA WITH TONOS
    { 0x0390, "iotadieresistonos" },  // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS
    { 0x0391, "Alpha" },  // GREEK CAPITAL LETTER ALPHA
    { 0x0392, "Beta" },  // GREEK CAPITAL LETTER BETA
    { 0x0393, "Gamma" },  // GREEK CAPITAL LETTER GAMMA
    { 0x0394, "Delta" },  // GREEK CAPITAL LETTER DELTA;Duplicate
    { 0x0395, "Epsilon" },  // GREEK CAPITAL LETTER EPSILON
    { 0x0396, "Zeta" },  // GREEK CAPITAL LETTER ZETA
    { 0x0397, "Eta" },  // GREEK CAPITAL LETTER ETA
    { 0x0398, "Theta" },  // GREEK CAPITAL LETTER THETA
    { 0x0399, "Iota" },  // GREEK CAPITAL LETTER IOTA
    { 0x039A, "Kappa" },  // GREEK CAPITAL LETTER KAPPA
    { 0x039B, "Lambda" },  // GREEK CAPITAL LETTER LAMDA
    { 0x039C, "Mu" },  // GREEK CAPITAL LETTER MU
    { 0x039D, "Nu" },  // GREEK CAPITAL LETTER NU
    { 0x039E, "Xi" },  // GREEK CAPITAL LETTER XI
    { 0x039F, "Omicron" },  // GREEK CAPITAL LETTER OMICRON
    { 0x03A0, "Pi" },  // GREEK CAPITAL LETTER PI
    { 0x03A1, "Rho" },  // GREEK CAPITAL LETTER RHO
    { 0x03A3, "Sigma" },  // GREEK CAPITAL LETTER SIGMA
    { 0x03A4, "Tau" },  // GREEK CAPITAL LETTER TAU
    { 0x03A5, "Upsilon" },  // GREEK CAPITAL LETTER UPSILON
    { 0x03A6, "Phi" },  // GREEK CAPITAL LETTER PHI
    { 0x03A7, "Chi" },  // GREEK CAPITAL LETTER CHI
    { 0x03A8, "Psi" },  // GREEK CAPITAL LETTER PSI
    { 0x03A9, "Omega" },  // GREEK CAPITAL LETTER OMEGA;Duplicate
    { 0x03AA, "Iotadieresis" },  // GREEK CAPITAL LETTER IOTA WITH DIALYTIKA
    { 0x03AB, "Upsilondieresis" },  // GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA
    { 0x03AC, "alphatonos" },  // GREEK SMALL LETTER ALPHA WITH TONOS
    { 0x03AD, "epsilontonos" },  // GREEK SMALL LETTER EPSILON WITH TONOS
    { 0x03AE, "etatonos" },  // GREEK SMALL LETTER ETA WITH TONOS
    { 0x03AF, "iotatonos" },  // GREEK SMALL LETTER IOTA WITH TONOS
    { 0x03B0, "upsilondieresistonos" },  // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS
    { 0x03B1, "alpha" },  // GREEK SMALL LETTER ALPHA
    { 0x03B2, "beta" },  // GREEK SMALL LETTER BETA
    { 0x03B3, "gamma" },  // GREEK SMALL LETTER GAMMA
    { 0x03B4, "delta" },  // GREEK SMALL LETTER DELTA
    { 0x03B5, "epsilon" },  // GREEK SMALL LETTER EPSILON
    { 0x03B6, "zeta" },  // GREEK SMALL LETTER ZETA
    { 0x03B7, "eta" },  // GREEK SMALL LETTER ETA
    { 0x03B8, "theta" },  // GREEK SMALL LETTER THETA
    { 0x03B9, "iota" },  // GREEK SMALL LETTER IOTA
    { 0x03BA, "kappa" },  // GREEK SMALL LETTER KAPPA
    { 0x03BB, "lambda" },  // GREEK SMALL LETTER LAMDA
    { 0x03BC, "mu" },  // GREEK SMALL LETTER MU;Duplicate
    { 0x03BD, "nu" },  // GREEK SMALL LETTER NU
    { 0x03BE, "xi" },  // GREEK SMALL LETTER XI
    { 0x03BF, "omicron" },  // GREEK SMALL LETTER OMICRON
    { 0x03C0, "pi" },  // GREEK SMALL LETTER PI
    { 0x03C1, "rho" },  // GREEK SMALL LETTER RHO
    { 0x03C2, "sigma1" },  // GREEK SMALL LETTER FINAL SIGMA
    { 0x03C3, "sigma" },  // GREEK SMALL LETTER SIGMA
    { 0x03C4, "tau" },  // GREEK SMALL LETTER TAU
    { 0x03C5, "upsilon" },  // GREEK SMALL LETTER UPSILON
    { 0x03C6, "phi" },  // GREEK SMALL LETTER PHI
    { 0x03C7, "chi" },  // GREEK SMALL LETTER CHI
    { 0x03C8, "psi" },  // GREEK SMALL LETTER PSI
    { 0x03C9, "omega" },  // GREEK SMALL LETTER OMEGA
    { 0x03CA, "iotadieresis" },  // GREEK SMALL LETTER IOTA WITH DIALYTIKA
    { 0x03CB, "upsilondieresis" },  // GREEK SMALL LETTER UPSILON WITH DIALYTIKA
    { 0x03CC, "omicrontonos" },  // GREEK SMALL LETTER OMICRON WITH TONOS
    { 0x03CD, "upsilontonos" },  // GREEK SMALL LETTER UPSILON WITH TONOS
    { 0x03CE, "omegatonos" },  // GREEK SMALL LETTER OMEGA WITH TONOS
    { 0x03D1, "theta1" },  // GREEK THETA SYMBOL
    { 0x03D2, "Upsilon1" },  // GREEK UPSILON WITH HOOK SYMBOL
    { 0x03D5, "phi1" },  // GREEK PHI SYMBOL
    { 0x03D6, "omega1" },  // GREEK PI SYMBOL
    { 0x0401, "afii10023" },  // CYRILLIC CAPITAL LETTER IO
    { 0x0402, "afii10051" },  // CYRILLIC CAPITAL LETTER DJE
    { 0x0403, "afii10052" },  // CYRILLIC CAPITAL LETTER GJE
    { 0x0404, "afii10053" },  // CYRILLIC CAPITAL LETTER UKRAINIAN IE
    { 0x0405, "afii10054" },  // CYRILLIC CAPITAL LETTER DZE
    { 0x0406, "afii10055" },  // CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I
    { 0x0407, "afii10056" },  // CYRILLIC CAPITAL LETTER YI
    { 0x0408, "afii10057" },  // CYRILLIC CAPITAL LETTER JE
    { 0x0409, "afii10058" },  // CYRILLIC CAPITAL LETTER LJE
    { 0x040A, "afii10059" },  // CYRILLIC CAPITAL LETTER NJE
    { 0x040B, "afii10060" },  // CYRILLIC CAPITAL LETTER TSHE
    { 0x040C, "afii10061" },  // CYRILLIC CAPITAL LETTER KJE
    { 0x040E, "afii10062" },  // CYRILLIC CAPITAL LETTER SHORT U
    { 0x040F, "afii10145" },  // CYRILLIC CAPITAL LETTER DZHE
    { 0x0410, "afii10017" },  // CYRILLIC CAPITAL LETTER A
    { 0x0411, "afii10018" },  // CYRILLIC CAPITAL LETTER BE
    { 0x0412, "afii10019" },  // CYRILLIC CAPITAL LETTER VE
    { 0x0413, "afii10020" },  // CYRILLIC CAPITAL LETTER GHE
    { 0x0414, "afii10021" },  // CYRILLIC CAPITAL LETTER DE
    { 0x0415, "afii10022" },  // CYRILLIC CAPITAL LETTER IE
    { 0x0416, "afii10024" },  // CYRILLIC CAPITAL LETTER ZHE
    { 0x0417, "afii10025" },  // CYRILLIC CAPITAL LETTER ZE
    { 0x0418, "afii10026" },  // CYRILLIC CAPITAL LETTER I
    { 0x0419, "afii10027" },  // CYRILLIC CAPITAL LETTER SHORT I
    { 0x041A, "afii10028" },  // CYRILLIC CAPITAL LETTER KA
    { 0x041B, "afii10029" },  // CYRILLIC CAPITAL LETTER EL
    { 0x041C, "afii10030" },  // CYRILLIC CAPITAL LETTER EM
    { 0x041D, "afii10031" },  // CYRILLIC CAPITAL LETTER EN
    { 0x041E, "afii10032" },  // CYRILLIC CAPITAL LETTER O
    { 0x041F, "afii10033" },  // CYRILLIC CAPITAL LETTER PE
    { 0x0420, "afii10034" },  // CYRILLIC CAPITAL LETTER ER
    { 0x0421, "afii10035" },  // CYRILLIC CAPITAL LETTER ES
    { 0x0422, "afii10036" },  // CYRILLIC CAPITAL LETTER TE
    { 0x0423, "afii10037" },  // CYRILLIC CAPITAL LETTER U
    { 0x0424, "afii10038" },  // CYRILLIC CAPITAL LETTER EF
    { 0x0425, "afii10039" },  // CYRILLIC CAPITAL LETTER HA
    { 0x0426, "afii10040" },  // CYRILLIC CAPITAL LETTER TSE
    { 0x0427, "afii10041" },  // CYRILLIC CAPITAL LETTER CHE
    { 0x0428, "afii10042" },  // CYRILLIC CAPITAL LETTER SHA
    { 0x0429, "afii10043" },  // CYRILLIC CAPITAL LETTER SHCHA
    { 0x042A, "afii10044" },  // CYRILLIC CAPITAL LETTER HARD SIGN
    { 0x042B, "afii10045" },  // CYRILLIC CAPITAL LETTER YERU
    { 0x042C, "afii10046" },  // CYRILLIC CAPITAL LETTER SOFT SIGN
    { 0x042D, "afii10047" },  // CYRILLIC CAPITAL LETTER E
    { 0x042E, "afii10048" },  // CYRILLIC CAPITAL LETTER YU
    { 0x042F, "afii10049" },  // CYRILLIC CAPITAL LETTER YA
    { 0x0430, "afii10065" },  // CYRILLIC SMALL LETTER A
    { 0x0431, "afii10066" },  // CYRILLIC SMALL LETTER BE
    { 0x0432, "afii10067" },  // CYRILLIC SMALL LETTER VE
    { 0x0433, "afii10068" },  // CYRILLIC SMALL LETTER GHE
    { 0x0434, "afii10069" },  // CYRILLIC SMALL LETTER DE
    { 0x0435, "afii10070" },  // CYRILLIC SMALL LETTER IE
    { 0x0436, "afii10072" },  // CYRILLIC SMALL LETTER ZHE
    { 0x0437, "afii10073" },  // CYRILLIC SMALL LETTER ZE
    { 0x0438, "afii10074" },  // CYRILLIC SMALL LETTER I
    { 0x0439, "afii10075" },  // CYRILLIC SMALL LETTER SHORT I
    { 0x043A, "afii10076" },  // CYRILLIC SMALL LETTER KA
    { 0x043B, "afii10077" },  // CYRILLIC SMALL LETTER EL
    { 0x043C, "afii10078" },  // CYRILLIC SMALL LETTER EM
    { 0x043D, "afii10079" },  // CYRILLIC SMALL LETTER EN
    { 0x043E, "afii10080" },  // CYRILLIC SMALL LETTER O
    { 0x043F, "afii10081" },  // CYRILLIC SMALL LETTER PE
    { 0x0440, "afii10082" },  // CYRILLIC SMALL LETTER ER
    { 0x0441, "afii10083" },  // CYRILLIC SMALL LETTER ES
    { 0x0442, "afii10084" },  // CYRILLIC SMALL LETTER TE
    { 0x0443, "afii10085" },  // CYRILLIC SMALL LETTER U
    { 0x0444, "afii10086" },  // CYRILLIC SMALL LETTER EF
    { 0x0445, "afii10087" },  // CYRILLIC SMALL LETTER HA
    { 0x0446, "afii10088" },  // CYRILLIC SMALL LETTER TSE
    { 0x0447, "afii10089" },  // CYRILLIC SMALL LETTER CHE
    { 0x0448, "afii10090" },  // CYRILLIC SMALL LETTER SHA
    { 0x0449, "afii10091" },  // CYRILLIC SMALL LETTER SHCHA
    { 0x044A, "afii10092" },  // CYRILLIC SMALL LETTER HARD SIGN
    { 0x044B, "afii10093" },  // CYRILLIC SMALL LETTER YERU
    { 0x044C, "afii10094" },  // CYRILLIC SMALL LETTER SOFT SIGN
    { 0x044D, "afii10095" },  // CYRILLIC SMALL LETTER E
    { 0x044E, "afii10096" },  // CYRILLIC SMALL LETTER YU
    { 0x044F, "afii10097" },  // CYRILLIC SMALL LETTER YA
    { 0x0451, "afii10071" },  // CYRILLIC SMALL LETTER IO
    { 0x0452, "afii10099" },  // CYRILLIC SMALL LETTER DJE
    { 0x0453, "afii10100" },  // CYRILLIC SMALL LETTER GJE
    { 0x0454, "afii10101" },  // CYRILLIC SMALL LETTER UKRAINIAN IE
    { 0x0455, "afii10102" },  // CYRILLIC SMALL LETTER DZE
    { 0x0456, "afii10103" },  // CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
    { 0x0457, "afii10104" },  // CYRILLIC SMALL LETTER YI
    { 0x0458, "afii10105" },  // CYRILLIC SMALL LETTER JE
    { 0x0459, "afii10106" },  // CYRILLIC SMALL LETTER LJE
    { 0x045A, "afii10107" },  // CYRILLIC SMALL LETTER NJE
    { 0x045B, "afii10108" },  // CYRILLIC SMALL LETTER TSHE
    { 0x045C, "afii10109" },  // CYRILLIC SMALL LETTER KJE
    { 0x045E, "afii10110" },  // CYRILLIC SMALL LETTER SHORT U
    { 0x045F, "afii10193" },  // CYRILLIC SMALL LETTER DZHE
    { 0x0462, "afii10146" },  // CYRILLIC CAPITAL LETTER YAT
    { 0x0463, "afii10194" },  // CYRILLIC SMALL LETTER YAT
    { 0x0472, "afii10147" },  // CYRILLIC CAPITAL LETTER FITA
    { 0x0473, "afii10195" },  // CYRILLIC SMALL LETTER FITA
    { 0x0474, "afii10148" },  // CYRILLIC CAPITAL LETTER IZHITSA
    { 0x0475, "afii10196" },  // CYRILLIC SMALL LETTER IZHITSA
    { 0x0490, "afii10050" },  // CYRILLIC CAPITAL LETTER GHE WITH UPTURN
    { 0x0491, "afii10098" },  // CYRILLIC SMALL LETTER GHE WITH UPTURN
    { 0x04D9, "afii10846" },  // CYRILLIC SMALL LETTER SCHWA
    { 0x05B0, "afii57799" },  // HEBREW POINT SHEVA
    { 0x05B1, "afii57801" },  // HEBREW POINT HATAF SEGOL
    { 0x05B2, "afii57800" },  // HEBREW POINT HATAF PATAH
    { 0x05B3, "afii57802" },  // HEBREW POINT HATAF QAMATS
    { 0x05B4, "afii57793" },  // HEBREW POINT HIRIQ
    { 0x05B5, "afii57794" },  // HEBREW POINT TSERE
    { 0x05B6, "afii57795" },  // HEBREW POINT SEGOL
    { 0x05B7, "afii57798" },  // HEBREW POINT PATAH
    { 0x05B8, "afii57797" },  // HEBREW POINT QAMATS
    { 0x05B9, "afii57806" },  // HEBREW POINT HOLAM
    { 0x05BB, "afii57796" },  // HEBREW POINT QUBUTS
    { 0x05BC, "afii57807" },  // HEBREW POINT DAGESH OR MAPIQ
    { 0x05BD, "afii57839" },  // HEBREW POINT METEG
    { 0x05BE, "afii57645" },  // HEBREW PUNCTUATION MAQAF
    { 0x05BF, "afii57841" },  // HEBREW POINT RAFE
    { 0x05C0, "afii57842" },  // HEBREW PUNCTUATION PASEQ
    { 0x05C1, "afii57804" },  // HEBREW POINT SHIN DOT
    { 0x05C2, "afii57803" },  // HEBREW POINT SIN DOT
    { 0x05C3, "afii57658" },  // HEBREW PUNCTUATION SOF PASUQ
    { 0x05D0, "afii57664" },  // HEBREW LETTER ALEF
    { 0x05D1, "afii57665" },  // HEBREW LETTER BET
    { 0x05D2, "afii57666" },  // HEBREW LETTER GIMEL
    { 0x05D3, "afii57667" },  // HEBREW LETTER DALET
    { 0x05D4, "afii57668" },  // HEBREW LETTER HE
    { 0x05D5, "afii57669" },  // HEBREW LETTER VAV
    { 0x05D6, "afii57670" },  // HEBREW LETTER ZAYIN
    { 0x05D7, "afii57671" },  // HEBREW LETTER HET
    { 0x05D8, "afii57672" },  // HEBREW LETTER TET
    { 0x05D9, "afii57673" },  // HEBREW LETTER YOD
    { 0x05DA, "afii57674" },  // HEBREW LETTER FINAL KAF
    { 0x05DB, "afii57675" },  // HEBREW LETTER KAF
    { 0x05DC, "afii57676" },  // HEBREW LETTER LAMED
    { 0x05DD, "afii57677" },  // HEBREW LETTER FINAL MEM
    { 0x05DE, "afii57678" },  // HEBREW LETTER MEM
    { 0x05DF, "afii57679" },  // HEBREW LETTER FINAL NUN
    { 0x05E0, "afii57680" },  // HEBREW LETTER NUN
    { 0x05E1, "afii57681" },  // HEBREW LETTER SAMEKH
    { 0x05E2, "afii57682" },  // HEBREW LETTER AYIN
    { 0x05E3, "afii57683" },  // HEBREW LETTER FINAL PE
    { 0x05E4, "afii57684" },  // HEBREW LETTER PE
    { 0x05E5, "afii57685" },  // HEBREW LETTER FINAL TSADI
    { 0x05E6, "afii57686" },  // HEBREW LETTER TSADI
    { 0x05E7, "afii57687" },  // HEBREW LETTER QOF
    { 0x05E8, "afii57688" },  // HEBREW LETTER RESH
    { 0x05E9, "afii57689" },  // HEBREW LETTER SHIN
    { 0x05EA, "afii57690" },  // HEBREW LETTER TAV
    { 0x05F0, "afii57716" },  // HEBREW LIGATURE YIDDISH DOUBLE VAV
    { 0x05F1, "afii57717" },  // HEBREW LIGATURE YIDDISH VAV YOD
    { 0x05F2, "afii57718" },  // HEBREW LIGATURE YIDDISH DOUBLE YOD
    { 0x060C, "afii57388" },  // ARABIC COMMA
    { 0x061B, "afii57403" },  // ARABIC SEMICOLON
    { 0x061F, "afii57407" },  // ARABIC QUESTION MARK
    { 0x0621, "afii57409" },  // ARABIC LETTER HAMZA
    { 0x0622, "afii57410" },  // ARABIC LETTER ALEF WITH MADDA ABOVE
    { 0x0623, "afii57411" },  // ARABIC LETTER ALEF WITH HAMZA ABOVE
    { 0x0624, "afii57412" },  // ARABIC LETTER WAW WITH HAMZA ABOVE
    { 0x0625, "afii57413" },  // ARABIC LETTER ALEF WITH HAMZA BELOW
    { 0x0626, "afii57414" },  // ARABIC LETTER YEH WITH HAMZA ABOVE
    { 0x0627, "afii57415" },  // ARABIC LETTER ALEF
    { 0x0628, "afii57416" },  // ARABIC LETTER BEH
    { 0x0629, "afii57417" },  // ARABIC LETTER TEH MARBUTA
    { 0x062A, "afii57418" },  // ARABIC LETTER TEH
    { 0x062B, "afii57419" },  // ARABIC LETTER THEH
    { 0x062C, "afii57420" },  // ARABIC LETTER JEEM
    { 0x062D, "afii57421" },  // ARABIC LETTER HAH
    { 0x062E, "afii57422" },  // ARABIC LETTER KHAH
    { 0x062F, "afii57423" },  // ARABIC LETTER DAL
    { 0x0630, "afii57424" },  // ARABIC LETTER THAL
    { 0x0631, "afii57425" },  // ARABIC LETTER REH
    { 0x0632, "afii57426" },  // ARABIC LETTER ZAIN
    { 0x0633, "afii57427" },  // ARABIC LETTER SEEN
    { 0x0634, "afii57428" },  // ARABIC LETTER SHEEN
    { 0x0635, "afii57429" },  // ARABIC LETTER SAD
    { 0x0636, "afii57430" },  // ARABIC LETTER DAD
    { 0x0637, "afii57431" },  // ARABIC LETTER TAH
    { 0x0638, "afii57432" },  // ARABIC LETTER ZAH
    { 0x0639, "afii57433" },  // ARABIC LETTER AIN
    { 0x063A, "afii57434" },  // ARABIC LETTER GHAIN
    { 0x0640, "afii57440" },  // ARABIC TATWEEL
    { 0x0641, "afii57441" },  // ARABIC LETTER FEH
    { 0x0642, "afii57442" },  // ARABIC LETTER QAF
    { 0x0643, "afii57443" },  // ARABIC LETTER KAF
    { 0x0644, "afii57444" },  // ARABIC LETTER LAM
    { 0x0645, "afii57445" },  // ARABIC LETTER MEEM
    { 0x0646, "afii57446" },  // ARABIC LETTER NOON
    { 0x0647, "afii57470" },  // ARABIC LETTER HEH
    { 0x0648, "afii57448" },  // ARABIC LETTER WAW
    { 0x0649, "afii57449" },  // ARABIC LETTER ALEF MAKSURA
    { 0x064A, "afii57450" },  // ARABIC LETTER YEH
    { 0x064B, "afii57451" },  // ARABIC FATHATAN
    { 0x064C, "afii57452" },  // ARABIC DAMMATAN
    { 0x064D, "afii57453" },  // ARABIC KASRATAN
    { 0x064E, "afii57454" },  // ARABIC FATHA
    { 0x064F, "afii57455" },  // ARABIC DAMMA
    { 0x0650, "afii57456" },  // ARABIC KASRA
    { 0x0651, "afii57457" },  // ARABIC SHADDA
    { 0x0652, "afii57458" },  // ARABIC SUKUN
    { 0x0660, "afii57392" },  // ARABIC-INDIC DIGIT ZERO
    { 0x0661, "afii57393" },  // ARABIC-INDIC DIGIT ONE
    { 0x0662, "afii57394" },  // ARABIC-INDIC DIGIT TWO
    { 0x0663, "afii57395" },  // ARABIC-INDIC DIGIT THREE
    { 0x0664, "afii57396" },  // ARABIC-INDIC DIGIT FOUR
    { 0x0665, "afii57397" },  // ARABIC-INDIC DIGIT FIVE
    { 0x0666, "afii57398" },  // ARABIC-INDIC DIGIT SIX
    { 0x0667, "afii57399" },  // ARABIC-INDIC DIGIT SEVEN
    { 0x0668, "afii57400" },  // ARABIC-INDIC DIGIT EIGHT
    { 0x0669, "afii57401" },  // ARABIC-INDIC DIGIT NINE
    { 0x066A, "afii57381" },  // ARABIC PERCENT SIGN
    { 0x066D, "afii63167" },  // ARABIC FIVE POINTED STAR
    { 0x0679, "afii57511" },  // ARABIC LETTER TTEH
    { 0x067E, "afii57506" },  // ARABIC LETTER PEH
    { 0x0686, "afii57507" },  // ARABIC LETTER TCHEH
    { 0x0688, "afii57512" },  // ARABIC LETTER DDAL
    { 0x0691, "afii57513" },  // ARABIC LETTER RREH
    { 0x0698, "afii57508" },  // ARABIC LETTER JEH
    { 0x06A4, "afii57505" },  // ARABIC LETTER VEH
    { 0x06AF, "afii57509" },  // ARABIC LETTER GAF
    { 0x06BA, "afii57514" },  // ARABIC LETTER NOON GHUNNA
    { 0x06D2, "afii57519" },  // ARABIC LETTER YEH BARREE
    { 0x06D5, "afii57534" },  // ARABIC LETTER AE
    { 0x1E80, "Wgrave" },  // LATIN CAPITAL LETTER W WITH GRAVE
    { 0x1E81, "wgrave" },  // LATIN SMALL LETTER W WITH GRAVE
    { 0x1E82, "Wacute" },  // LATIN CAPITAL LETTER W WITH ACUTE
    { 0x1E83, "wacute" },  // LATIN SMALL LETTER W WITH ACUTE
    { 0x1E84, "Wdieresis" },  // LATIN CAPITAL LETTER W WITH DIAERESIS
    { 0x1E85, "wdieresis" },  // LATIN SMALL LETTER W WITH DIAERESIS
    { 0x1EF2, "Ygrave" },  // LATIN CAPITAL LETTER Y WITH GRAVE
    { 0x1EF3, "ygrave" },  // LATIN SMALL LETTER Y WITH GRAVE
    { 0x200C, "afii61664" },  // ZERO WIDTH NON-JOINER
    { 0x200D, "afii301" },  // ZERO WIDTH JOINER
    { 0x200E, "afii299" },  // LEFT-TO-RIGHT MARK
    { 0x200F, "afii300" },  // RIGHT-TO-LEFT MARK
    { 0x2012, "figuredash" },  // FIGURE DASH
    { 0x2013, "endash" },  // EN DASH
    { 0x2014, "emdash" },  // EM DASH
    { 0x2015, "afii00208" },  // HORIZONTAL BAR
    { 0x2017, "underscoredbl" },  // DOUBLE LOW LINE
    { 0x2018, "quoteleft" },  // LEFT SINGLE QUOTATION MARK
    { 0x2019, "quoteright" },  // RIGHT SINGLE QUOTATION MARK
    { 0x201A, "quotesinglbase" },  // SINGLE LOW-9 QUOTATION MARK
    { 0x201B, "quotereversed" },  // SINGLE HIGH-REVERSED-9 QUOTATION MARK
    { 0x201C, "quotedblleft" },  // LEFT DOUBLE QUOTATION MARK
    { 0x201D, "quotedblright" },  // RIGHT DOUBLE QUOTATION MARK
    { 0x201E, "quotedblbase" },  // DOUBLE LOW-9 QUOTATION MARK
    { 0x2020, "dagger" },  // DAGGER
    { 0x2021, "daggerdbl" },  // DOUBLE DAGGER
    { 0x2022, "bullet" },  // BULLET
    { 0x2024, "onedotenleader" },  // ONE DOT LEADER
    { 0x2025, "twodotenleader" },  // TWO DOT LEADER
    { 0x2026, "ellipsis" },  // HORIZONTAL ELLIPSIS
    { 0x202C, "afii61573" },  // POP DIRECTIONAL FORMATTING
    { 0x202D, "afii61574" },  // LEFT-TO-RIGHT OVERRIDE
    { 0x202E, "afii61575" },  // RIGHT-TO-LEFT OVERRIDE
    { 0x2030, "perthousand" },  // PER MILLE SIGN
    { 0x2032, "minute" },  // PRIME
    { 0x2033, "second" },  // DOUBLE PRIME
    { 0x2039, "guilsinglleft" },  // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
    { 0x203A, "guilsinglright" },  // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
    { 0x203C, "exclamdbl" },  // DOUBLE EXCLAMATION MARK
    { 0x2044, "fraction" },  // FRACTION SLASH
    { 0x2070, "zerosuperior" },  // SUPERSCRIPT ZERO
    { 0x2074, "foursuperior" },  // SUPERSCRIPT FOUR
    { 0x2075, "fivesuperior" },  // SUPERSCRIPT FIVE
    { 0x2076, "sixsuperior" },  // SUPERSCRIPT SIX
    { 0x2077, "sevensuperior" },  // SUPERSCRIPT SEVEN
    { 0x2078, "eightsuperior" },  // SUPERSCRIPT EIGHT
    { 0x2079, "ninesuperior" },  // SUPERSCRIPT NINE
    { 0x207D, "parenleftsuperior" },  // SUPERSCRIPT LEFT PARENTHESIS
    { 0x207E, "parenrightsuperior" },  // SUPERSCRIPT RIGHT PARENTHESIS
    { 0x207F, "nsuperior" },  // SUPERSCRIPT LATIN SMALL LETTER N
    { 0x2080, "zeroinferior" },  // SUBSCRIPT ZERO
    { 0x2081, "oneinferior" },  // SUBSCRIPT ONE
    { 0x2082, "twoinferior" },  // SUBSCRIPT TWO
    { 0x2083, "threeinferior" },  // SUBSCRIPT THREE
    { 0x2084, "fourinferior" },  // SUBSCRIPT FOUR
    { 0x2085, "fiveinferior" },  // SUBSCRIPT FIVE
    { 0x2086, "sixinferior" },  // SUBSCRIPT SIX
    { 0x2087, "seveninferior" },  // SUBSCRIPT SEVEN
    { 0x2088, "eightinferior" },  // SUBSCRIPT EIGHT
    { 0x2089, "nineinferior" },  // SUBSCRIPT NINE
    { 0x208D, "parenleftinferior" },  // SUBSCRIPT LEFT PARENTHESIS
    { 0x208E, "parenrightinferior" },  // SUBSCRIPT RIGHT PARENTHESIS
    { 0x20A1, "colonmonetary" },  // COLON SIGN
    { 0x20A3, "franc" },  // FRENCH FRANC SIGN
    { 0x20A4, "lira" },  // LIRA SIGN
    { 0x20A7, "peseta" },  // PESETA SIGN
    { 0x20AA, "afii57636" },  // NEW SHEQEL SIGN
    { 0x20AB, "dong" },  // DONG SIGN
    { 0x20AC, "Euro" },  // EURO SIGN
    { 0x2105, "afii61248" },  // CARE OF
    { 0x2111, "Ifraktur" },  // BLACK-LETTER CAPITAL I
    { 0x2113, "afii61289" },  // SCRIPT SMALL L
    { 0x2116, "afii61352" },  // NUMERO SIGN
    { 0x2118, "weierstrass" },  // SCRIPT CAPITAL P
    { 0x211C, "Rfraktur" },  // BLACK-LETTER CAPITAL R
    { 0x211E, "prescription" },  // PRESCRIPTION TAKE
    { 0x2122, "trademark" },  // TRADE MARK SIGN
    { 0x2126, "Omega" },  // OHM SIGN
    { 0x212E, "estimated" },  // ESTIMATED SYMBOL
    { 0x2135, "aleph" },  // ALEF SYMBOL
    { 0x2153, "onethird" },  // VULGAR FRACTION ONE THIRD
    { 0x2154, "twothirds" },  // VULGAR FRACTION TWO THIRDS
    { 0x215B, "oneeighth" },  // VULGAR FRACTION ONE EIGHTH
    { 0x215C, "threeeighths" },  // VULGAR FRACTION THREE EIGHTHS
    { 0x215D, "fiveeighths" },  // VULGAR FRACTION FIVE EIGHTHS
    { 0x215E, "seveneighths" },  // VULGAR FRACTION SEVEN EIGHTHS
    { 0x2190, "arrowleft" },  // LEFTWARDS ARROW
    { 0x2191, "arrowup" },  // UPWARDS ARROW
    { 0x2192, "arrowright" },  // RIGHTWARDS ARROW
    { 0x2193, "arrowdown" },  // DOWNWARDS ARROW
    { 0x2194, "arrowboth" },  // LEFT RIGHT ARROW
    { 0x2195, "arrowupdn" },  // UP DOWN ARROW
    { 0x21A8, "arrowupdnbse" },  // UP DOWN ARROW WITH BASE
    { 0x21B5, "carriagereturn" },  // DOWNWARDS ARROW WITH CORNER LEFTWARDS
    { 0x21D0, "arrowdblleft" },  // LEFTWARDS DOUBLE ARROW
    { 0x21D1, "arrowdblup" },  // UPWARDS DOUBLE ARROW
    { 0x21D2, "arrowdblright" },  // RIGHTWARDS DOUBLE ARROW
    { 0x21D3, "arrowdbldown" },  // DOWNWARDS DOUBLE ARROW
    { 0x21D4, "arrowdblboth" },  // LEFT RIGHT DOUBLE ARROW
    { 0x2200, "universal" },  // FOR ALL
    { 0x2202, "partialdiff" },  // PARTIAL DIFFERENTIAL
    { 0x2203, "existential" },  // THERE EXISTS
    { 0x2205, "emptyset" },  // EMPTY SET
    { 0x2206, "Delta" },  // INCREMENT
    { 0x2207, "gradient" },  // NABLA
    { 0x2208, "element" },  // ELEMENT OF
    { 0x2209, "notelement" },  // NOT AN ELEMENT OF
    { 0x220B, "suchthat" },  // CONTAINS AS MEMBER
    { 0x220F, "product" },  // N-ARY PRODUCT
    { 0x2211, "summation" },  // N-ARY SUMMATION
    { 0x2212, "minus" },  // MINUS SIGN
    { 0x2215, "fraction" },  // DIVISION SLASH;Duplicate
    { 0x2217, "asteriskmath" },  // ASTERISK OPERATOR
    { 0x2219, "periodcentered" },  // BULLET OPERATOR;Duplicate
    { 0x221A, "radical" },  // SQUARE ROOT
    { 0x221D, "proportional" },  // PROPORTIONAL TO
    { 0x221E, "infinity" },  // INFINITY
    { 0x221F, "orthogonal" },  // RIGHT ANGLE
    { 0x2220, "angle" },  // ANGLE
    { 0x2227, "logicaland" },  // LOGICAL AND
    { 0x2228, "logicalor" },  // LOGICAL OR
    { 0x2229, "intersection" },  // INTERSECTION
    { 0x222A, "union" },  // UNION
    { 0x222B, "integral" },  // INTEGRAL
    { 0x2234, "therefore" },  // THEREFORE
    { 0x223C, "similar" },  // TILDE OPERATOR
    { 0x2245, "congruent" },  // APPROXIMATELY EQUAL TO
    { 0x2248, "approxequal" },  // ALMOST EQUAL TO
    { 0x2260, "notequal" },  // NOT EQUAL TO
    { 0x2261, "equivalence" },  // IDENTICAL TO
    { 0x2264, "lessequal" },  // LESS-THAN OR EQUAL TO
    { 0x2265, "greaterequal" },  // GREATER-THAN OR EQUAL TO
    { 0x2282, "propersubset" },  // SUBSET OF
    { 0x2283, "propersuperset" },  // SUPERSET OF
    { 0x2284, "notsubset" },  // NOT A SUBSET OF
    { 0x2286, "reflexsubset" },  // SUBSET OF OR EQUAL TO
    { 0x2287, "reflexsuperset" },  // SUPERSET OF OR EQUAL TO
    { 0x2295, "circleplus" },  // CIRCLED PLUS
    { 0x2297, "circlemultiply" },  // CIRCLED TIMES
    { 0x22A5, "perpendicular" },  // UP TACK
    { 0x22C5, "dotmath" },  // DOT OPERATOR
    { 0x2302, "house" },  // HOUSE
    { 0x2310, "revlogicalnot" },  // REVERSED NOT SIGN
    { 0x2320, "integraltp" },  // TOP HALF INTEGRAL
    { 0x2321, "integralbt" },  // BOTTOM HALF INTEGRAL
    { 0x2329, "angleleft" },  // LEFT-POINTING ANGLE BRACKET
    { 0x232A, "angleright" },  // RIGHT-POINTING ANGLE BRACKET
    { 0x2500, "SF100000" },  // BOX DRAWINGS LIGHT HORIZONTAL
    { 0x2502, "SF110000" },  // BOX DRAWINGS LIGHT VERTICAL
    { 0x250C, "SF010000" },  // BOX DRAWINGS LIGHT DOWN AND RIGHT
    { 0x2510, "SF030000" },  // BOX DRAWINGS LIGHT DOWN AND LEFT
    { 0x2514, "SF020000" },  // BOX DRAWINGS LIGHT UP AND RIGHT
    { 0x2518, "SF040000" },  // BOX DRAWINGS LIGHT UP AND LEFT
    { 0x251C, "SF080000" },  // BOX DRAWINGS LIGHT VERTICAL AND RIGHT
    { 0x2524, "SF090000" },  // BOX DRAWINGS LIGHT VERTICAL AND LEFT
    { 0x252C, "SF060000" },  // BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
    { 0x2534, "SF070000" },  // BOX DRAWINGS LIGHT UP AND HORIZONTAL
    { 0x253C, "SF050000" },  // BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
    { 0x2550, "SF430000" },  // BOX DRAWINGS DOUBLE HORIZONTAL
    { 0x2551, "SF240000" },  // BOX DRAWINGS DOUBLE VERTICAL
    { 0x2552, "SF510000" },  // BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE
    { 0x2553, "SF520000" },  // BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE
    { 0x2554, "SF390000" },  // BOX DRAWINGS DOUBLE DOWN AND RIGHT
    { 0x2555, "SF220000" },  // BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE
    { 0x2556, "SF210000" },  // BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE
    { 0x2557, "SF250000" },  // BOX DRAWINGS DOUBLE DOWN AND LEFT
    { 0x2558, "SF500000" },  // BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE
    { 0x2559, "SF490000" },  // BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE
    { 0x255A, "SF380000" },  // BOX DRAWINGS DOUBLE UP AND RIGHT
    { 0x255B, "SF280000" },  // BOX DRAWINGS UP SINGLE AND LEFT DOUBLE
    { 0x255C, "SF270000" },  // BOX DRAWINGS UP DOUBLE AND LEFT SINGLE
    { 0x255D, "SF260000" },  // BOX DRAWINGS DOUBLE UP AND LEFT
    { 0x255E, "SF360000" },  // BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE
    { 0x255F, "SF370000" },  // BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE
    { 0x2560, "SF420000" },  // BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
    { 0x2561, "SF190000" },  // BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE
    { 0x2562, "SF200000" },  // BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE
    { 0x2563, "SF230000" },  // BOX DRAWINGS DOUBLE VERTICAL AND LEFT
    { 0x2564, "SF470000" },  // BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE
    { 0x2565, "SF480000" },  // BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE
    { 0x2566, "SF410000" },  // BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
    { 0x2567, "SF450000" },  // BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE
    { 0x2568, "SF460000" },  // BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE
    { 0x2569, "SF400000" },  // BOX DRAWINGS DOUBLE UP AND HORIZONTAL
    { 0x256A, "SF540000" },  // BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE
    { 0x256B, "SF530000" },  // BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE
    { 0x256C, "SF440000" },  // BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
    { 0x2580, "upblock" },  // UPPER HALF BLOCK
    { 0x2584, "dnblock" },  // LOWER HALF BLOCK
    { 0x2588, "block" },  // FULL BLOCK
    { 0x258C, "lfblock" },  // LEFT HALF BLOCK
    { 0x2590, "rtblock" },  // RIGHT HALF BLOCK
    { 0x2591, "ltshade" },  // LIGHT SHADE
    { 0x2592, "shade" },  // MEDIUM SHADE
    { 0x2593, "dkshade" },  // DARK SHADE
    { 0x25A0, "filledbox" },  // BLACK SQUARE
    { 0x25A1, "H22073" },  // WHITE SQUARE
    { 0x25AA, "H18543" },  // BLACK SMALL SQUARE
    { 0x25AB, "H18551" },  // WHITE SMALL SQUARE
    { 0x25AC, "filledrect" },  // BLACK RECTANGLE
    { 0x25B2, "triagup" },  // BLACK UP-POINTING TRIANGLE
    { 0x25BA, "triagrt" },  // BLACK RIGHT-POINTING POINTER
    { 0x25BC, "triagdn" },  // BLACK DOWN-POINTING TRIANGLE
    { 0x25C4, "triaglf" },  // BLACK LEFT-POINTING POINTER
    { 0x25CA, "lozenge" },  // LOZENGE
    { 0x25CB, "circle" },  // WHITE CIRCLE
    { 0x25CF, "H18533" },  // BLACK CIRCLE
    { 0x25D8, "invbullet" },  // INVERSE BULLET
    { 0x25D9, "invcircle" },  // INVERSE WHITE CIRCLE
    { 0x25E6, "openbullet" },  // WHITE BULLET
    { 0x263A, "smileface" },  // WHITE SMILING FACE
    { 0x263B, "invsmileface" },  // BLACK SMILING FACE
    { 0x263C, "sun" },  // WHITE SUN WITH RAYS
    { 0x2640, "female" },  // FEMALE SIGN
    { 0x2642, "male" },  // MALE SIGN
    { 0x2660, "spade" },  // BLACK SPADE SUIT
    { 0x2663, "club" },  // BLACK CLUB SUIT
    { 0x2665, "heart" },  // BLACK HEART SUIT
    { 0x2666, "diamond" },  // BLACK DIAMOND SUIT
    { 0x266A, "musicalnote" },  // EIGHTH NOTE
    { 0x266B, "musicalnotedbl" },  // BEAMED EIGHTH NOTES
    { 0xF6BE, "dotlessj" },  // LATIN SMALL LETTER DOTLESS J
    { 0xF6BF, "LL" },  // LATIN CAPITAL LETTER LL
    { 0xF6C0, "ll" },  // LATIN SMALL LETTER LL
    { 0xF6C1, "Scedilla" },  // LATIN CAPITAL LETTER S WITH CEDILLA;Duplicate
    { 0xF6C2, "scedilla" },  // LATIN SMALL LETTER S WITH CEDILLA;Duplicate
    { 0xF6C3, "commaaccent" },  // COMMA BELOW
    { 0xF6C4, "afii10063" },  // CYRILLIC SMALL LETTER GHE VARIANT
    { 0xF6C5, "afii10064" },  // CYRILLIC SMALL LETTER BE VARIANT
    { 0xF6C6, "afii10192" },  // CYRILLIC SMALL LETTER DE VARIANT
    { 0xF6C7, "afii10831" },  // CYRILLIC SMALL LETTER PE VARIANT
    { 0xF6C8, "afii10832" },  // CYRILLIC SMALL LETTER TE VARIANT
    { 0xF6C9, "Acute" },  // CAPITAL ACUTE ACCENT
    { 0xF6CA, "Caron" },  // CAPITAL CARON
    { 0xF6CB, "Dieresis" },  // CAPITAL DIAERESIS
    { 0xF6CC, "DieresisAcute" },  // CAPITAL DIAERESIS ACUTE ACCENT
    { 0xF6CD, "DieresisGrave" },  // CAPITAL DIAERESIS GRAVE ACCENT
    { 0xF6CE, "Grave" },  // CAPITAL GRAVE ACCENT
    { 0xF6CF, "Hungarumlaut" },  // CAPITAL DOUBLE ACUTE ACCENT
    { 0xF6D0, "Macron" },  // CAPITAL MACRON
    { 0xF6D1, "cyrBreve" },  // CAPITAL CYRILLIC BREVE
    { 0xF6D2, "cyrFlex" },  // CAPITAL CYRILLIC CIRCUMFLEX
    { 0xF6D3, "dblGrave" },  // CAPITAL DOUBLE GRAVE ACCENT
    { 0xF6D4, "cyrbreve" },  // CYRILLIC BREVE
    { 0xF6D5, "cyrflex" },  // CYRILLIC CIRCUMFLEX
    { 0xF6D6, "dblgrave" },  // DOUBLE GRAVE ACCENT
    { 0xF6D7, "dieresisacute" },  // DIAERESIS ACUTE ACCENT
    { 0xF6D8, "dieresisgrave" },  // DIAERESIS GRAVE ACCENT
    { 0xF6D9, "copyrightserif" },  // COPYRIGHT SIGN SERIF
    { 0xF6DA, "registerserif" },  // REGISTERED SIGN SERIF
    { 0xF6DB, "trademarkserif" },  // TRADE MARK SIGN SERIF
    { 0xF6DC, "onefitted" },  // PROPORTIONAL DIGIT ONE
    { 0xF6DD, "rupiah" },  // RUPIAH SIGN
    { 0xF6DE, "threequartersemdash" },  // THREE QUARTERS EM DASH
    { 0xF6DF, "centinferior" },  // SUBSCRIPT CENT SIGN
    { 0xF6E0, "centsuperior" },  // SUPERSCRIPT CENT SIGN
    { 0xF6E1, "commainferior" },  // SUBSCRIPT COMMA
    { 0xF6E2, "commasuperior" },  // SUPERSCRIPT COMMA
    { 0xF6E3, "dollarinferior" },  // SUBSCRIPT DOLLAR SIGN
    { 0xF6E4, "dollarsuperior" },  // SUPERSCRIPT DOLLAR SIGN
    { 0xF6E5, "hypheninferior" },  // SUBSCRIPT HYPHEN-MINUS
    { 0xF6E6, "hyphensuperior" },  // SUPERSCRIPT HYPHEN-MINUS
    { 0xF6E7, "periodinferior" },  // SUBSCRIPT FULL STOP
    { 0xF6E8, "periodsuperior" },  // SUPERSCRIPT FULL STOP
    { 0xF6E9, "asuperior" },  // SUPERSCRIPT LATIN SMALL LETTER A
    { 0xF6EA, "bsuperior" },  // SUPERSCRIPT LATIN SMALL LETTER B
    { 0xF6EB, "dsuperior" },  // SUPERSCRIPT LATIN SMALL LETTER D
    { 0xF6EC, "esuperior" },  // SUPERSCRIPT LATIN SMALL LETTER E
    { 0xF6ED, "isuperior" },  // SUPERSCRIPT LATIN SMALL LETTER I
    { 0xF6EE, "lsuperior" },  // SUPERSCRIPT LATIN SMALL LETTER L
    { 0xF6EF, "msuperior" },  // SUPERSCRIPT LATIN SMALL LETTER M
    { 0xF6F0, "osuperior" },  // SUPERSCRIPT LATIN SMALL LETTER O
    { 0xF6F1, "rsuperior" },  // SUPERSCRIPT LATIN SMALL LETTER R
    { 0xF6F2, "ssuperior" },  // SUPERSCRIPT LATIN SMALL LETTER S
    { 0xF6F3, "tsuperior" },  // SUPERSCRIPT LATIN SMALL LETTER T
    { 0xF6F4, "Brevesmall" },  // SMALL CAPITAL BREVE
    { 0xF6F5, "Caronsmall" },  // SMALL CAPITAL CARON
    { 0xF6F6, "Circumflexsmall" },  // SMALL CAPITAL MODIFIER LETTER CIRCUMFLEX ACCENT
    { 0xF6F7, "Dotaccentsmall" },  // SMALL CAPITAL DOT ABOVE
    { 0xF6F8, "Hungarumlautsmall" },  // SMALL CAPITAL DOUBLE ACUTE ACCENT
    { 0xF6F9, "Lslashsmall" },  // LATIN SMALL CAPITAL LETTER L WITH STROKE
    { 0xF6FA, "OEsmall" },  // LATIN SMALL CAPITAL LIGATURE OE
    { 0xF6FB, "Ogoneksmall" },  // SMALL CAPITAL OGONEK
    { 0xF6FC, "Ringsmall" },  // SMALL CAPITAL RING ABOVE
    { 0xF6FD, "Scaronsmall" },  // LATIN SMALL CAPITAL LETTER S WITH CARON
    { 0xF6FE, "Tildesmall" },  // SMALL CAPITAL SMALL TILDE
    { 0xF6FF, "Zcaronsmall" },  // LATIN SMALL CAPITAL LETTER Z WITH CARON
    { 0xF721, "exclamsmall" },  // SMALL CAPITAL EXCLAMATION MARK
    { 0xF724, "dollaroldstyle" },  // OLDSTYLE DOLLAR SIGN
    { 0xF726, "ampersandsmall" },  // SMALL CAPITAL AMPERSAND
    { 0xF730, "zerooldstyle" },  // OLDSTYLE DIGIT ZERO
    { 0xF731, "oneoldstyle" },  // OLDSTYLE DIGIT ONE
    { 0xF732, "twooldstyle" },  // OLDSTYLE DIGIT TWO
    { 0xF733, "threeoldstyle" },  // OLDSTYLE DIGIT THREE
    { 0xF734, "fouroldstyle" },  // OLDSTYLE DIGIT FOUR
    { 0xF735, "fiveoldstyle" },  // OLDSTYLE DIGIT FIVE
    { 0xF736, "sixoldstyle" },  // OLDSTYLE DIGIT SIX
    { 0xF737, "sevenoldstyle" },  // OLDSTYLE DIGIT SEVEN
    { 0xF738, "eightoldstyle" },  // OLDSTYLE DIGIT EIGHT
    { 0xF739, "nineoldstyle" },  // OLDSTYLE DIGIT NINE
    { 0xF73F, "questionsmall" },  // SMALL CAPITAL QUESTION MARK
    { 0xF760, "Gravesmall" },  // SMALL CAPITAL GRAVE ACCENT
    { 0xF761, "Asmall" },  // LATIN SMALL CAPITAL LETTER A
    { 0xF762, "Bsmall" },  // LATIN SMALL CAPITAL LETTER B
    { 0xF763, "Csmall" },  // LATIN SMALL CAPITAL LETTER C
    { 0xF764, "Dsmall" },  // LATIN SMALL CAPITAL LETTER D
    { 0xF765, "Esmall" },  // LATIN SMALL CAPITAL LETTER E
    { 0xF766, "Fsmall" },  // LATIN SMALL CAPITAL LETTER F
    { 0xF767, "Gsmall" },  // LATIN SMALL CAPITAL LETTER G
    { 0xF768, "Hsmall" },  // LATIN SMALL CAPITAL LETTER H
    { 0xF769, "Ismall" },  // LATIN SMALL CAPITAL LETTER I
    { 0xF76A, "Jsmall" },  // LATIN SMALL CAPITAL LETTER J
    { 0xF76B, "Ksmall" },  // LATIN SMALL CAPITAL LETTER K
    { 0xF76C, "Lsmall" },  // LATIN SMALL CAPITAL LETTER L
    { 0xF76D, "Msmall" },  // LATIN SMALL CAPITAL LETTER M
    { 0xF76E, "Nsmall" },  // LATIN SMALL CAPITAL LETTER N
    { 0xF76F, "Osmall" },  // LATIN SMALL CAPITAL LETTER O
    { 0xF770, "Psmall" },  // LATIN SMALL CAPITAL LETTER P
    { 0xF771, "Qsmall" },  // LATIN SMALL CAPITAL LETTER Q
    { 0xF772, "Rsmall" },  // LATIN SMALL CAPITAL LETTER R
    { 0xF773, "Ssmall" },  // LATIN SMALL CAPITAL LETTER S
    { 0xF774, "Tsmall" },  // LATIN SMALL CAPITAL LETTER T
    { 0xF775, "Usmall" },  // LATIN SMALL CAPITAL LETTER U
    { 0xF776, "Vsmall" },  // LATIN SMALL CAPITAL LETTER V
    { 0xF777, "Wsmall" },  // LATIN SMALL CAPITAL LETTER W
    { 0xF778, "Xsmall" },  // LATIN SMALL CAPITAL LETTER X
    { 0xF779, "Ysmall" },  // LATIN SMALL CAPITAL LETTER Y
    { 0xF77A, "Zsmall" },  // LATIN SMALL CAPITAL LETTER Z
    { 0xF7A1, "exclamdownsmall" },  // SMALL CAPITAL INVERTED EXCLAMATION MARK
    { 0xF7A2, "centoldstyle" },  // OLDSTYLE CENT SIGN
    { 0xF7A8, "Dieresissmall" },  // SMALL CAPITAL DIAERESIS
    { 0xF7AF, "Macronsmall" },  // SMALL CAPITAL MACRON
    { 0xF7B4, "Acutesmall" },  // SMALL CAPITAL ACUTE ACCENT
    { 0xF7B8, "Cedillasmall" },  // SMALL CAPITAL CEDILLA
    { 0xF7BF, "questiondownsmall" },  // SMALL CAPITAL INVERTED QUESTION MARK
    { 0xF7E0, "Agravesmall" },  // LATIN SMALL CAPITAL LETTER A WITH GRAVE
    { 0xF7E1, "Aacutesmall" },  // LATIN SMALL CAPITAL LETTER A WITH ACUTE
    { 0xF7E2, "Acircumflexsmall" },  // LATIN SMALL CAPITAL LETTER A WITH CIRCUMFLEX
    { 0xF7E3, "Atildesmall" },  // LATIN SMALL CAPITAL LETTER A WITH TILDE
    { 0xF7E4, "Adieresissmall" },  // LATIN SMALL CAPITAL LETTER A WITH DIAERESIS
    { 0xF7E5, "Aringsmall" },  // LATIN SMALL CAPITAL LETTER A WITH RING ABOVE
    { 0xF7E6, "AEsmall" },  // LATIN SMALL CAPITAL LETTER AE
    { 0xF7E7, "Ccedillasmall" },  // LATIN SMALL CAPITAL LETTER C WITH CEDILLA
    { 0xF7E8, "Egravesmall" },  // LATIN SMALL CAPITAL LETTER E WITH GRAVE
    { 0xF7E9, "Eacutesmall" },  // LATIN SMALL CAPITAL LETTER E WITH ACUTE
    { 0xF7EA, "Ecircumflexsmall" },  // LATIN SMALL CAPITAL LETTER E WITH CIRCUMFLEX
    { 0xF7EB, "Edieresissmall" },  // LATIN SMALL CAPITAL LETTER E WITH DIAERESIS
    { 0xF7EC, "Igravesmall" },  // LATIN SMALL CAPITAL LETTER I WITH GRAVE
    { 0xF7ED, "Iacutesmall" },  // LATIN SMALL CAPITAL LETTER I WITH ACUTE
    { 0xF7EE, "Icircumflexsmall" },  // LATIN SMALL CAPITAL LETTER I WITH CIRCUMFLEX
    { 0xF7EF, "Idieresissmall" },  // LATIN SMALL CAPITAL LETTER I WITH DIAERESIS
    { 0xF7F0, "Ethsmall" },  // LATIN SMALL CAPITAL LETTER ETH
    { 0xF7F1, "Ntildesmall" },  // LATIN SMALL CAPITAL LETTER N WITH TILDE
    { 0xF7F2, "Ogravesmall" },  // LATIN SMALL CAPITAL LETTER O WITH GRAVE
    { 0xF7F3, "Oacutesmall" },  // LATIN SMALL CAPITAL LETTER O WITH ACUTE
    { 0xF7F4, "Ocircumflexsmall" },  // LATIN SMALL CAPITAL LETTER O WITH CIRCUMFLEX
    { 0xF7F5, "Otildesmall" },  // LATIN SMALL CAPITAL LETTER O WITH TILDE
    { 0xF7F6, "Odieresissmall" },  // LATIN SMALL CAPITAL LETTER O WITH DIAERESIS
    { 0xF7F8, "Oslashsmall" },  // LATIN SMALL CAPITAL LETTER O WITH STROKE
    { 0xF7F9, "Ugravesmall" },  // LATIN SMALL CAPITAL LETTER U WITH GRAVE
    { 0xF7FA, "Uacutesmall" },  // LATIN SMALL CAPITAL LETTER U WITH ACUTE
    { 0xF7FB, "Ucircumflexsmall" },  // LATIN SMALL CAPITAL LETTER U WITH CIRCUMFLEX
    { 0xF7FC, "Udieresissmall" },  // LATIN SMALL CAPITAL LETTER U WITH DIAERESIS
    { 0xF7FD, "Yacutesmall" },  // LATIN SMALL CAPITAL LETTER Y WITH ACUTE
    { 0xF7FE, "Thornsmall" },  // LATIN SMALL CAPITAL LETTER THORN
    { 0xF7FF, "Ydieresissmall" },  // LATIN SMALL CAPITAL LETTER Y WITH DIAERESIS
    { 0xF8E5, "radicalex" },  // RADICAL EXTENDER
    { 0xF8E6, "arrowvertex" },  // VERTICAL ARROW EXTENDER
    { 0xF8E7, "arrowhorizex" },  // HORIZONTAL ARROW EXTENDER
    { 0xF8E8, "registersans" },  // REGISTERED SIGN SANS SERIF
    { 0xF8E9, "copyrightsans" },  // COPYRIGHT SIGN SANS SERIF
    { 0xF8EA, "trademarksans" },  // TRADE MARK SIGN SANS SERIF
    { 0xF8EB, "parenlefttp" },  // LEFT PAREN TOP
    { 0xF8EC, "parenleftex" },  // LEFT PAREN EXTENDER
    { 0xF8ED, "parenleftbt" },  // LEFT PAREN BOTTOM
    { 0xF8EE, "bracketlefttp" },  // LEFT SQUARE BRACKET TOP
    { 0xF8EF, "bracketleftex" },  // LEFT SQUARE BRACKET EXTENDER
    { 0xF8F0, "bracketleftbt" },  // LEFT SQUARE BRACKET BOTTOM
    { 0xF8F1, "bracelefttp" },  // LEFT CURLY BRACKET TOP
    { 0xF8F2, "braceleftmid" },  // LEFT CURLY BRACKET MID
    { 0xF8F3, "braceleftbt" },  // LEFT CURLY BRACKET BOTTOM
    { 0xF8F4, "braceex" },  // CURLY BRACKET EXTENDER
    { 0xF8F5, "integralex" },  // INTEGRAL EXTENDER
    { 0xF8F6, "parenrighttp" },  // RIGHT PAREN TOP
    { 0xF8F7, "parenrightex" },  // RIGHT PAREN EXTENDER
    { 0xF8F8, "parenrightbt" },  // RIGHT PAREN BOTTOM
    { 0xF8F9, "bracketrighttp" },  // RIGHT SQUARE BRACKET TOP
    { 0xF8FA, "bracketrightex" },  // RIGHT SQUARE BRACKET EXTENDER
    { 0xF8FB, "bracketrightbt" },  // RIGHT SQUARE BRACKET BOTTOM
    { 0xF8FC, "bracerighttp" },  // RIGHT CURLY BRACKET TOP
    { 0xF8FD, "bracerightmid" },  // RIGHT CURLY BRACKET MID
    { 0xF8FE, "bracerightbt" },  // RIGHT CURLY BRACKET BOTTOM
    { 0xFB00, "ff" },  // LATIN SMALL LIGATURE FF
    { 0xFB01, "fi" },  // LATIN SMALL LIGATURE FI
    { 0xFB02, "fl" },  // LATIN SMALL LIGATURE FL
    { 0xFB03, "ffi" },  // LATIN SMALL LIGATURE FFI
    { 0xFB04, "ffl" },  // LATIN SMALL LIGATURE FFL
    { 0xFB1F, "afii57705" },  // HEBREW LIGATURE YIDDISH YOD YOD PATAH
    { 0xFB2A, "afii57694" },  // HEBREW LETTER SHIN WITH SHIN DOT
    { 0xFB2B, "afii57695" },  // HEBREW LETTER SHIN WITH SIN DOT
    { 0xFB35, "afii57723" },  // HEBREW LETTER VAV WITH DAGESH
    { 0xFB4B, "afii57700" },  // HEBREW LETTER VAV WITH HOLAM
    // end of stuff from glyphlist.txt
    { 0xFFFF, 0 }
};


static const struct {
    QFont::CharSet cs;
    uint mib;
} unicodevalues[] = {
    { QFont::ISO_8859_1, 4 },
#ifndef QT_NO_TEXTCODEC
    { QFont::ISO_8859_2, 5 },
    { QFont::ISO_8859_3, 6 },
    { QFont::ISO_8859_4, 7 },
    { QFont::ISO_8859_5, 8 },
    { QFont::KOI8U, 2088 },
    { QFont::KOI8R, 2084 },
    { QFont::ISO_8859_6, 82 },
    { QFont::ISO_8859_7, 10 },
    { QFont::ISO_8859_8, 85 },
    { QFont::ISO_8859_10, 13 },
    { QFont::ISO_8859_11, 2259 }, // aka tis620
    // ### don't have any MIB for -12: { QFont::ISO_8859_12, 0 },
    { QFont::ISO_8859_13, 109 },
    { QFont::ISO_8859_14, 110 },
    { QFont::ISO_8859_15, 111 },
    { QFont::CP1251, 2251 },
	{ QFont::PT154, 0 },
	{ QFont::Set_Ko, 38 },
	{ QFont::KSC_5601, 38 },
	{ QFont::Set_Big5, 2026 },
	{ QFont::Big5, 2026 },
    // makeFixedStrings() below assumes that this is last
    { QFont::ISO_8859_9, 12 }
#define unicodevalues_LAST QFont::ISO_8859_9
#else
#define unicodevalues_LAST QFont::ISO_8859_1
#endif
};

// ---------------------------------------------------------------------
// postscript font substitution dictionary. We assume every postscript printer has at least 
// Helvetica, Times, Courier and Symbol

struct psfont {
    const char *psname;
    float slant;
    float xscale;
};

static const psfont Arial[] = {
    {"Arial", 0, 84.04 },
    { "Arial-Italic", 0, 84.04 },
    { "Arial-Bold", 0, 88.65 },
    { "Arial-BoldItalic", 0, 88.65 }
};

static const psfont AvantGarde[] = {
    { "AvantGarde-Book", 0, 87.43 },
    { "AvantGarde-BookOblique", 0, 88.09 },
    { "AvantGarde-Demi", 0, 88.09 },
    { "AvantGarde-DemiOblique", 0, 87.43 },
};

static const psfont Bookman [] = {
    { "Bookman-Light", 0, 93.78 },
    { "Bookman-LightItalic", 0, 91.42 },
    { "Bookman-Demi", 0, 99.86 },
    { "Bookman-DemiItalic", 0, 101.54 } 
};
	 
static const psfont Charter [] = {
    { "CharterBT-Roman", 0, 84.04 },
    { "CharterBT-Italic", 0.0, 81.92 },
    { "CharterBT-Bold", 0, 88.99 },
    { "CharterBT-BoldItalic", 0.0, 88.20 }
};

static const psfont Courier [] = {
    { "Courier", 0, 100. },
    { "Courier-Oblique", 0, 100. },
    { "Courier-Bold", 0, 100. },
    { "Courier-BoldOblique", 0, 100. }
};

static const psfont Garamond [] = {
    { "Garamond-Antiqua", 0, 78.13 },
    { "Garamond-Kursiv", 0, 78.13 },
    { "Garamond-Halbfett", 0, 78.13 },
    { "Garamond-KursivHalbfett", 0, 78.13 }
};

static const psfont GillSans [] = { // ### some estimated value for xstretch
    { "GillSans", 0, 82 },
    { "GillSans-Italic", 0, 82 },
    { "GillSans-Bold", 0, 82 },
    { "GillSans-BoldItalic", 0, 82 }
};

static const psfont Helvetica [] = {
    { "Helvetica", 0, 84.04 },
    { "Helvetica-Oblique", 0, 84.04 },
    { "Helvetica-Bold", 0, 88.65 },
    { "Helvetica-BoldOblique", 0, 88.65 }
};

static const psfont Letter [] = {
    { "LetterGothic", 0, 83.32 },
    { "LetterGothic-Italic", 0, 83.32 },
    { "LetterGothic-Bold", 0, 83.32 },
    { "LetterGothic-Bold", 0.2, 83.32 }
};

static const psfont LucidaSans [] = {
    { "LucidaSans", 0, 94.36 },
    { "LucidaSans-Oblique", 0, 94.36 },
    { "LucidaSans-Demi", 0, 98.10 },
    { "LucidaSans-DemiOblique", 0, 98.08 }
};

static const psfont LucidaSansTT [] = {
    { "LucidaSans-Typewriter", 0, 100.50 },
    { "LucidaSans-TypewriterOblique", 0, 100.50 },
    { "LucidaSans-TypewriterBold", 0, 100.50 },
    { "LucidaSans-TypewriterBoldOblique", 0, 100.50 }
};

static const psfont LucidaBright [] = {
    { "LucidaBright", 0, 93.45 },
    { "LucidaBright-Italic", 0, 91.98 },
    { "LucidaBright-Demi", 0, 96.22 },
    { "LucidaBright-DemiItalic", 0, 96.98 }
};

static const psfont Palatino [] = {
    { "Palatino-Roman", 0, 82.45 },
    { "Palatino-Italic", 0, 76.56 },
    { "Palatino-Bold", 0, 83.49 },
    { "Palatino-BoldItalic", 0, 81.51 }
};

static const psfont Symbol [] = {
    { "Symbol", 0, 82.56 },
    { "Symbol", 0.2, 82.56 },
    { "Symbol", 0, 82.56 },
    { "Symbol", 0.2, 82.56 }
};

static const psfont Tahoma [] = {
    { "Tahoma", 0, 83.45 },
    { "Tahoma", 0.2, 83.45 },
    { "Tahoma-Bold", 0, 95.59 },
    { "Tahoma-Bold", 0.2, 95.59 }
};

static const psfont Times [] = {
    { "Times-Roman", 0, 82.45 },
    { "Times-Italic", 0, 82.45 },
    { "Times-Bold", 0, 82.45 },
    { "Times-BoldItalic", 0, 82.45 }
};

static const psfont Verdana [] = {
    { "Verdana", 0, 96.06 },
    { "Verdana-Italic", 0, 96.06 },
    { "Verdana-Bold", 0, 107.12 },
    { "Verdana-BoldItalic", 0, 107.10 }
};

static const psfont Utopia [] = { // ###
    { "Utopia-Regular", 0, 84.70 },
    { "Utopia-Regular", 0.2, 84.70 },
    { "Utopia-Bold", 0, 88.01 },
    { "Utopia-Bold", 0.2, 88.01 }
};

static const psfont * const SansSerifReplacements[] = {
    Helvetica, 0
	};
static const psfont * const SerifReplacements[] = {
    Times, 0
	};
static const psfont * const FixedReplacements[] = {
    Courier, 0
	};
static const psfont * const TahomaReplacements[] = {
    Verdana, AvantGarde, Helvetica, 0
	};
static const psfont * const VerdanaReplacements[] = {
    Tahoma, AvantGarde, Helvetica, 0
	};

static const struct {
    const char * input; // spaces are stripped in here, and everything lowercase
    const psfont * ps;
    const psfont *const * replacements;
} postscriptFonts [] = {
    { "arial", Arial, SansSerifReplacements },
    { "arialmt", Arial, SansSerifReplacements },
    { "arialunicodems", Arial, SansSerifReplacements },
    { "avantgarde", AvantGarde, SansSerifReplacements },
    { "bookman", Bookman, SerifReplacements },
    { "charter", Charter, SansSerifReplacements },
    { "bitstreamcharter", Charter, SansSerifReplacements },
	{ "bitstreamcyberbit", Times, SerifReplacements }, // ###
    { "courier", Courier, 0 },
    { "couriernew", Courier, 0 },
    { "fixed", Courier, 0 },
    { "garamond", Garamond, SerifReplacements },
    { "gillsans", GillSans, SansSerifReplacements },
    { "helvetica", Helvetica, 0 },
    { "letter", Letter, FixedReplacements },
    { "lucida", LucidaSans, SansSerifReplacements },
    { "lucidasans", LucidaSans, SansSerifReplacements },
    { "lucidabright", LucidaBright, SerifReplacements },
    { "lucidasanstypewriter", LucidaSansTT, FixedReplacements },
    { "luciduxsans", LucidaSans, SansSerifReplacements },
    { "luciduxserif", LucidaBright, SerifReplacements },
    { "luciduxmono", LucidaSansTT, FixedReplacements },
    { "palatino", Palatino, SerifReplacements },
    { "symbol", Symbol, 0 },
    { "tahoma", Tahoma, TahomaReplacements },
    { "terminal", Courier, 0 },
    { "times", Times, 0 },
    { "timesnewroman", Times, 0 },
    { "verdana", Verdana, VerdanaReplacements },
    { "utopia", Utopia, SerifReplacements },
    { 0, 0, 0 }
};    

static int getPsFontType( const QFont &f )
{
    int weight = f.weight();
    bool italic = f.italic();
  
    int type = 0; // used to look up in the psname array
  // get the right modification, or build something
  if ( weight > QFont::Normal && italic )
      type = 3;
  else if ( weight > QFont::Normal )
      type = 2;
  else if ( italic )
      type = 1;
  return type;
}

static int addPsFontNameExtension( const QFont &f, QString &ps, const psfont *psf = 0 )
{
    int type = getPsFontType( f );

  if ( psf ) {
      ps = QString::fromLatin1( psf[type].psname );
  } else {
      switch ( type ) {
	  case 1:
	      ps.append( QString::fromLatin1("-Italic") );
	      break;
	  case 2:
	      ps.append( QString::fromLatin1("-Bold") );
	      break;
	  case 3:
	      ps.append( QString::fromLatin1("-BoldItalic") );
	      break;
	  case 0:
	  default:
	      break;
      }
  }
  return type;
}

static QString makePSFontName( const QFont &f, int *listpos = 0, int *ftype = 0 )
{
  QString ps;
  int i;

  QString family = f.family();
  family = family.lower();


  // try to make a "good" postscript name
  ps = family.simplifyWhiteSpace();
  i = 0;
  while( (unsigned int)i < ps.length() ) {
    if ( i == 0 || ps[i-1] == ' ' ) {
      ps[i] = ps[i].upper();
      if ( i )
	ps.remove( i-1, 1 );
      else
	i++;
    } else {
      i++;
    }
  }

  // see if the table has a better name
  i = 0;
  while( postscriptFonts[i].input &&
	 postscriptFonts[i].input != ps )
    i++;
  const psfont *psf = postscriptFonts[i].ps;

  int type = addPsFontNameExtension( f, ps, psf );
  
  if ( listpos )
      *listpos = i;
  if ( ftype )
      *ftype = type;
  return ps;
}

static void appendReplacements( QStringList &list, const psfont * const * replacements, int type, float xscale = 100. )
{
    // iterate through the replacement fonts
    while ( *replacements ) {
	const psfont *psf = *replacements;
	QString ps = "[ /" + QString::fromLatin1( psf[type].psname ) + " " + QString::number( xscale / psf[type].xscale ) + " " +
	     QString::number( psf[type].slant ) + " ]";
	list.append( ps );
	++replacements;
    }
}

static QStringList makePSFontNameList( const QFont &f, const QString &psname = QString::null, bool useNameForLookup = FALSE )
{
    int i;
    int type;
    QStringList list;
    QString ps = psname;
	
    if ( !ps.isEmpty() && !useNameForLookup ) {
	QString best = "[ /" + ps + " 1.0 0.0 ]";
	list.append( best );
    }

    ps = makePSFontName( f, &i, &type );

    const psfont *psf = postscriptFonts[i].ps;
    const psfont * const * replacements = postscriptFonts[i].replacements;
    float xscale = 100; 
    if ( psf ) {
	// xscale for the "right" font is always 1. We scale the replacements...
	xscale = psf->xscale;
	ps = "[ /" + QString::fromLatin1( psf[type].psname ) + " 1.0 " +
	     QString::number( psf[type].slant ) + " ]";
    } else {
	ps = "[ /" + ps + " 1.0 0.0 ]";
	// only add default replacement fonts in case this font was unknown.
	QFontInfo fi( f );
	if ( fi.fixedPitch() ) {
	    replacements = FixedReplacements;
	} else {
	    replacements = SansSerifReplacements;
	    // 100 is courier, but most fonts are not as wide as courier. Using 100
	    // here would make letters overlap for some fonts. This value is empirical.
	    xscale = 83; 
	}
    }
    list.append( ps );
    
    if ( replacements )
	appendReplacements( list, replacements, type, xscale);
    return list;
}

static void emitPSFontNameList( QTextStream &s, const QString &psname, const QStringList &list )
{
    s << "/" << psname << "List [\n";
    s << list.join("\n  ");
    s << "\n] d\n";
}


static QString *fixed_ps_header = 0;
static QIntDict<QString> * font_vectors = 0;

static void cleanup()
{
    delete fixed_ps_header;
    fixed_ps_header = 0;
    delete font_vectors;
    font_vectors = 0;
}


static QString wordwrap( const QString & s )
{
    QString result;

    int ip; // input pointer
    int oll; // output line length
    int cils, cols; // canidate input line start, -o-
    bool needws; // need to insert ws before next character
    bool havews; // have just inserted something like ws

    ip = 0;
    oll = 0;
    cils = cols = 0;
    havews = FALSE;
    needws = FALSE;
    while( ip < (int)s.length() ) {
	if ( oll > 79 && cils > 0 ) {
	    result.truncate( cols );
	    ip = cils;
	    result += '\n';
	    oll = 0;
	    cils = 0;
	    havews = TRUE;
	}
	if ( havews && oll > 0 ) {
	    cils = ip;
	    cols = result.length();
	}
	if ( isspace( s[ip] ) ) {
	    if ( !havews )
		needws = TRUE;
	    cils = ip+1;
	    cols = result.length();
	} else if ( s[ip] == '/'  || s[ip] == '{' || s[ip] == '}' ||
		    s[ip] == '[' || s[ip] == ']' ) {
	    havews = s[ip] != '/';
	    needws = FALSE;
	    cils = ip;
	    cols = result.length();
	    result += s[ip];
	    oll++;
	} else {
	    if ( needws ) {
		cols = result.length();
		cils = ip;
		result += ' ';
		oll++;
		needws = FALSE;
	    }
	    result += s[ip];
	    oll++;
	    havews = FALSE;
	}
	ip++;
    }
    return result;
}


static void makeFixedStrings()
{
    if ( fixed_ps_header )
	return;
    qAddPostRoutine( cleanup );

    fixed_ps_header = new QString;
    const char * const * headerLine = ps_header;
    while ( *headerLine ) {
	fixed_ps_header->append( QString::fromLatin1(*headerLine++) );
	fixed_ps_header->append( '\n' );
    }

    *fixed_ps_header = wordwrap( *fixed_ps_header );

    // fonts.
    font_vectors = new QIntDict<QString>( 59 );
    font_vectors->setAutoDelete( TRUE );

    int i = 0;
    int k;
    int l = 0; // unicode to glyph cursor
    QString vector;
    QString glyphname;
    QString unicodestring;
    do {
	vector.sprintf( "/FE%d [", (int)unicodevalues[i].cs );
	glyphname = "";
	l = 0;
	// the first 128 positions are the same always
	for( k=0; k<128; k++ ) {
	    while( unicodetoglyph[l].u < k )
		l++;
	    if ( unicodetoglyph[l].u == k )
		glyphname = QString::fromLatin1(unicodetoglyph[l].g);
	    else
		glyphname = QString::fromLatin1("ND");
	    vector += QString::fromLatin1(" /");
	    vector += glyphname;
	}
	// the next 128 are particular to each encoding
#ifndef QT_NO_TEXTCODEC
	QTextCodec * codec;
	codec = QTextCodec::codecForMib( (int)unicodevalues[i].mib );
	for( k=128; k<256; k++ ) {
	    int value = 0xFFFD;
	    uchar as8bit[2];
	    as8bit[0] = k;
	    as8bit[1] = '\0';
	    if ( codec ) {
		value = codec->toUnicode( (char*) as8bit, 1 )[0].unicode();
	    }
	    if ( value == 0xFFFD ) {
		glyphname = QString::fromLatin1("ND");
	    } else {
		if ( l && unicodetoglyph[l].u > value )
		    l = 0;
		while( unicodetoglyph[l].u < value )
		    l++;
		if ( unicodetoglyph[l].u == value )
		    glyphname = QString::fromLatin1(unicodetoglyph[l].g);
		else
		    glyphname = QString::fromLatin1("ND");
	    }
	    vector += QString::fromLatin1(" /");
	    vector += glyphname;
	}
#endif
	vector += QString::fromLatin1(" ] d");
	vector = wordwrap( vector );
	font_vectors->insert( (int)(unicodevalues[i].cs),
			      new QString( vector ) );
    } while ( unicodevalues[i++].cs != unicodevalues_LAST );
}

class QPSPrinterFontPrivate;

struct QPSPrinterPrivate {
    QPSPrinterPrivate( int filedes )
	: buffer( 0 ), realDevice( 0 ), fd( filedes ), fonts(27, FALSE), fontBuffer(0), savedImage( 0 ),
	  dirtypen( FALSE ), dirtybrush( FALSE ), currentFontCodec( 0 ),
	  fm( 0 ), textY( 0 )
    {
	headerFontNames.setAutoDelete( TRUE );
	pageFontNames.setAutoDelete( TRUE );
	fonts.setAutoDelete( TRUE );
	headerEncodings.setAutoDelete( FALSE );
	pageEncodings.setAutoDelete( FALSE );
	currentFontFile = 0;
    }

    QBuffer * buffer;
    int pagesInBuffer;
    QIODevice * realDevice;
    int fd;
    QDict<QString> headerFontNames;
    QDict<QString> pageFontNames;
    QIntDict<void> headerEncodings;
    QIntDict<void> pageEncodings;
    QDict<QPSPrinterFontPrivate> fonts;
    QPSPrinterFontPrivate *currentFontFile;
    int headerFontNumber;
    int pageFontNumber;
    QBuffer * fontBuffer;
    QTextStream fontStream;
    bool dirtyClipping;
    bool firstClipOnPage;
    QRect boundingBox;
    QImage * savedImage;
    QPen cpen;
    QBrush cbrush;
    bool dirtypen;
    bool dirtybrush;
    QTextCodec * currentFontCodec;
    QString currentFont;
    QFontMetrics * fm;
    int textY;
    QFont currentUsed;
    QFont currentSet;
};



QPSPrinter::QPSPrinter( QPrinter *prt, int fd )
    : QPaintDevice( QInternal::Printer | QInternal::ExternalDevice )
{
    printer = prt;
    d = new QPSPrinterPrivate( fd );
}


QPSPrinter::~QPSPrinter()
{
    if ( d->fd >= 0 )
#if defined(_OS_WIN32_)
	::_close( d->fd );
#else
	::close( d->fd );
#endif
    delete d;
}


// ========================== FONT FILE CLASSES  ===============

#ifdef _WS_X11_
// including all of X11/Xlib.h caused parse errors
// #include <X11/Xlib.h>
// extern "C" void*  XOpenDisplay(char*);
// extern "C" char** XGetFontPath(void*, int*);
// extern "C" void   XCloseDisplay(void*);
// extern "C" void   XFreeFontPath(char**);
#endif

class QPSPrinterFontPrivate {
public:
    virtual QString postScriptFontName() { return psname; }
    virtual QString defineFont( QTextStream &stream, QString ps, const QFont &f, int cs, const QString &key, 
			     QPSPrinterPrivate *d );
    virtual void download(QTextStream& s) = 0;
    virtual ~QPSPrinterFontPrivate() {}
    virtual void drawText( QTextStream &stream, uint spaces, const QPoint &p, 
			   const QString &text, QPSPrinterPrivate *d, QPainter *paint);
protected:
    QString psname;
    QStringList replacementList;
};


void QPSPrinterFontPrivate::drawText( QTextStream &stream, uint spaces, const QPoint &p, 
				 const QString &text, QPSPrinterPrivate *d, QPainter *paint)
{
    QCString tmpC;
#ifndef QT_NO_TEXTCODEC
    if ( d->currentFontCodec ) {
	tmpC = d->currentFontCodec->fromUnicode( text );
    } else 
#endif
	tmpC=text.local8Bit();
    char *tmp = new char[tmpC.length()*2 + 2];
#if defined(CHECK_NULL)
    CHECK_PTR( tmp );
#endif
    const char* from = (const char*)tmpC;

	// first scan to see whether it'll look good with just ()
    int parenlevel = 0;
    bool parensOK = TRUE;
    while( from && *from ) {
	if ( *from == '(' )
	    parenlevel++;
	else if ( *from == ')' )
	    parenlevel--;
	if ( parenlevel < 0 )
	    parensOK = FALSE;
	from++;
    }
    if ( parenlevel != 0 )
	parensOK = FALSE;

	// then scan again, outputting the stuff
    from = (const char *)tmpC;
    char * to = tmp;
    while ( from && *from ) {
	if ( *from == '\\' ||
	     ( !parensOK && ( *from == '(' || *from == ')' ) ) )
	    *to++ = '\\'; // escape special chars if necessary
	*to++ = *from++;
    }
    *to = '\0';

    if ( qstrlen( tmp ) > 0 ) {
	int x = p.x();
	if ( spaces > 0 )
	    x += spaces * d->fm->width( ' ' );
	int y = p.y();
	if ( y != d->textY || d->textY == 0 )
	    stream << y << " Y";
	d->textY = y;
	int w = d->fm->width( text );
	stream << "(" << tmp << ")" << w << " " << x;
	if ( paint->font().underline() )
	    stream << ' ' << y + d->fm->underlinePos() << " Tl";
	if ( paint->font().strikeOut() )
	    stream << ' ' << y + d->fm->strikeOutPos() << " Tl";
	stream << " T\n";
    }
    if ( tmp )
	delete [] tmp;
}


QString QPSPrinterFontPrivate::defineFont( QTextStream &stream, QString ps, const QFont &f, int cs, const QString &key, 
				   QPSPrinterPrivate *d )
{
    QString key2;
    key2.sprintf( "%s %d", ps.ascii(), cs );
    QString *tmp = d->headerFontNames.find( key );

    QString fontEncoding, fontName;
    fontEncoding.sprintf( " FE%d", cs );
    if ( d->buffer ) {
	if ( !d->headerEncodings.find( cs ) ) {
	    QString * vector = font_vectors->find( cs );
	    if ( vector ) {
		d->fontStream << *vector << "\n";
		d->headerEncodings.insert( cs, (void*)42 );
	    } else {
		d->fontStream << "% wanted font encoding "
			      << cs << "\n";
	    }
	}
	if ( tmp ) {
	    fontName = *tmp;
	} else {
	    fontName.sprintf( "/F%d", ++d->headerFontNumber );
	    if ( cs != QFont::AnyCharSet )
		d->fontStream << fontName << fontEncoding << " "
			      << ps << "List MF\n";
	    else 
		d->fontStream << fontName << " false "
			      << ps << "List MF\n";		
	    d->headerFontNames.insert( key2, new QString( fontName ) );
	}
	++d->headerFontNumber;
	d->fontStream << "/F" << d->headerFontNumber << " "
		      << f.pointSize() << fontName << " DF\n";
	fontName.sprintf( "F%d", d->headerFontNumber );
	d->headerFontNames.insert( key, new QString( fontName ) );
    } else {
	if ( !d->headerEncodings.find( cs ) &&
	     !d->pageEncodings.find( cs ) ) {
	    QString * vector = font_vectors->find( cs );
	    if ( !vector )
		vector = font_vectors->find( QFont::Latin1 );
	    stream << *vector << "\n";
	    d->pageEncodings.insert( cs, (void*)42 );
	}
	if ( !tmp )
	    tmp = d->pageFontNames.find( key );
	if ( tmp ) {
	    fontName = *tmp;
	} else {
	    fontName.sprintf( "/F%d", ++d->pageFontNumber );
	    if ( cs != QFont::AnyCharSet )
		stream << fontName << fontEncoding << " " << ps << "List MF\n";
	    else
		stream << fontName << " false " << ps << "List MF\n";
	    d->pageFontNames.insert( key2, new QString( fontName ) );
	}
	++d->pageFontNumber;
	stream << "/F" << d->pageFontNumber << " "
	       << f.pointSize() << fontName << " DF\n";
	fontName.sprintf( "F%d", d->pageFontNumber );
	d->pageFontNames.insert( key, new QString( fontName ) );
    }
    return fontName;
}

// ================== TTF ====================


typedef Q_UINT8  BYTE;
typedef Q_UINT16 USHORT;
typedef Q_UINT16 uFWord;
typedef Q_INT16 SHORT;
typedef Q_INT16 FWord;
typedef Q_UINT32   ULONG;  
typedef Q_INT32  FIXED;

typedef struct {
  Q_INT16 whole;
  Q_UINT16 fraction;
} Fixed; // 16.16 bit fixed-point number

static ULONG getULONG(BYTE *p) 
{
  int x;
  ULONG val=0;    
  
  for(x=0; x<4; x++) {
    val *= 0x100;
    val += p[x];	
  }
  
  return val;
} 

static USHORT getUSHORT(BYTE *p)
{
  int x;
  USHORT val=0;    
  
  for(x=0; x<2; x++) {
    val *= 0x100;
    val += p[x];	
  }
  
  return val;
} 

static Fixed getFixed(BYTE *s)
{
  Fixed val={0,0};
  
  val.whole = ((s[0] * 256) + s[1]);
  val.fraction = ((s[2] * 256) + s[3]);    
  
  return val;
}

static FWord getFWord(BYTE* s)  { return (FWord)  getUSHORT(s); }
static uFWord getuFWord(BYTE* s) { return (uFWord) getUSHORT(s); }
static SHORT getSHORT(BYTE* s)  { return (SHORT)  getUSHORT(s); }

static const char * const Apple_CharStrings[]={ 
  ".notdef",".null","nonmarkingreturn","space","exclam","quotedbl","numbersign", 
  "dollar","percent","ampersand","quotesingle","parenleft","parenright", 
  "asterisk","plus", "comma","hyphen","period","slash","zero","one","two",
  "three","four","five","six","seven","eight","nine","colon","semicolon", 
  "less","equal","greater","question","at","A","B","C","D","E","F","G","H","I",
  "J","K", "L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
  "bracketleft","backslash","bracketright","asciicircum","underscore","grave",
  "a","b","c","d","e","f","g","h","i","j","k", "l","m","n","o","p","q","r","s", 
  "t","u","v","w","x","y","z","braceleft","bar","braceright","asciitilde",
  "Adieresis","Aring","Ccedilla","Eacute","Ntilde","Odieresis","Udieresis",
  "aacute","agrave","acircumflex","adieresis","atilde","aring","ccedilla",
  "eacute","egrave","ecircumflex","edieresis","iacute","igrave","icircumflex", 
  "idieresis","ntilde","oacute","ograve","ocircumflex","odieresis","otilde",
  "uacute","ugrave","ucircumflex","udieresis","dagger","degree","cent",
  "sterling","section","bullet","paragraph","germandbls","registered",
  "copyright","trademark","acute","dieresis","notequal","AE","Oslash",
  "infinity","plusminus","lessequal","greaterequal","yen","mu","partialdiff",
  "summation","product","pi","integral","ordfeminine","ordmasculine","Omega",
  "ae","oslash","questiondown","exclamdown","logicalnot","radical","florin", 
  "approxequal","Delta","guillemotleft","guillemotright","ellipsis",
  "nobreakspace","Agrave","Atilde","Otilde","OE","oe","endash","emdash",
  "quotedblleft","quotedblright","quoteleft","quoteright","divide","lozenge",
  "ydieresis","Ydieresis","fraction","currency","guilsinglleft","guilsinglright",
  "fi","fl","daggerdbl","periodcentered","quotesinglbase","quotedblbase",
  "perthousand","Acircumflex","Ecircumflex","Aacute","Edieresis","Egrave",
  "Iacute","Icircumflex","Idieresis","Igrave","Oacute","Ocircumflex","apple",
  "Ograve","Uacute","Ucircumflex","Ugrave","dotlessi","circumflex","tilde",
  "macron","breve","dotaccent","ring","cedilla","hungarumlaut","ogonek","caron",
  "Lslash","lslash","Scaron","scaron","Zcaron","zcaron","brokenbar","Eth","eth",
  "Yacute","yacute","Thorn","thorn","minus","multiply","onesuperior",
  "twosuperior","threesuperior","onehalf","onequarter","threequarters","franc",
  "Gbreve","gbreve","Idot","Scedilla","scedilla","Cacute","cacute","Ccaron",
  "ccaron","dmacron","markingspace","capslock","shift","propeller","enter", 
  "markingtabrtol","markingtabltor","control","markingdeleteltor",
  "markingdeletertol","option","escape","parbreakltor","parbreakrtol",
  "newpage","checkmark","linebreakltor","linebreakrtol","markingnobreakspace",
  "diamond","appleoutline"};

typedef struct {
  int*    epts_ctr;			/* array of contour endpoints */
  int     num_pts, num_ctr;		/* number of points, number of coutours */
  FWord*  xcoor, *ycoor;		/* arrays of x and y coordinates */
  BYTE*   tt_flags;			/* array of TrueType flags */
  double* area_ctr;
  char*   check_ctr;
  int*    ctrset;  		/* in contour index followed by out contour index */
} charproc_data;


class QPSPrinterFontTTF
  : public QPSPrinterFontPrivate {
public:
  QPSPrinterFontTTF(const QFont &f, QByteArray& data);
  virtual void    download(QTextStream& s);
  //  virtual ~QPSPrinterFontTTF();
private:
  QByteArray     data;
  bool    defective; // if we can't process this file

  BYTE*   getTable(const char *);
  QString glyphName(int glyph);
  USHORT  unicode_for_glyph(int glyphindex);
  int   topost(FWord x) { return (int)( ((int)(x) * 1000 + HUPM) / unitsPerEm ); }

#ifdef Q_PRINTER_USE_TYPE42
  void sfnts_pputBYTE(BYTE n,QTextStream& s, 
				    int& string_len, int& line_len, bool& in_string);
  void sfnts_pputUSHORT(USHORT n,QTextStream& s,
				      int& string_len, int& line_len, bool& in_string);
  void sfnts_pputULONG(ULONG n,QTextStream& s,
				     int& string_len, int& line_len, bool& in_string);
  void sfnts_end_string(QTextStream& s,
				      int& string_len, int& line_len, bool& in_string);
  void sfnts_new_table(ULONG length,QTextStream& s,
				     int& string_len, int& line_len, bool& in_string);
  void sfnts_glyf_table(ULONG oldoffset, 
				      ULONG correct_total_length,
				      QTextStream& s,
				      int& string_len, int& line_len, bool& in_string);
  void download_sfnts(QTextStream& s);
#endif
      
  void charproc(int charindex, QTextStream& s);
  BYTE* charprocFindGlyphData(int charindex);
  void charprocComposite(BYTE *glyph, QTextStream& s);
  void charprocLoad(BYTE *glyph, charproc_data* cd);

  int target_type;			/* 42 or 3 */
  
  int numTables;			/* number of tables present */
  QString PostName;			/* Font's PostScript name */
  QString FullName;			/* Font's full name */
  QString FamilyName;			/* Font's family name */
  QString Style;			/* Font's style string */
  QString Copyright;			/* Font's copyright string */
  QString Version;			/* Font's version string */
  QString Trademark;			/* Font's trademark string */
  int llx,lly,urx,ury;		/* bounding box */
  
  Fixed TTVersion;			/* Truetype version number from offset table */
  Fixed MfrRevision;			/* Revision number of this font */
  
  BYTE *offset_table; 		/* Offset table in memory */
  BYTE *post_table;			/* 'post' table in memory */
  
  BYTE *loca_table;			/* 'loca' table in memory */
  BYTE *glyf_table;			/* 'glyf' table in memory */
  BYTE *hmtx_table;			/* 'hmtx' table in memory */
  
  USHORT numberOfHMetrics;
  int unitsPerEm;			/* unitsPerEm converted to int */
  int HUPM;				/* half of above */
  
  int numGlyphs;			/* from 'post' table */
  
  int indexToLocFormat;		/* short or long offsets */

};


#ifdef Q_PRINTER_USE_TYPE42
#ifndef _OS_OSF_
extern "C" char* getenv(char*);
#endif
#endif

QPSPrinterFontTTF::QPSPrinterFontTTF(const QFont &f, QByteArray& d)
{
  data = d;
  defective = FALSE;
  
  BYTE *ptr;
  
  target_type = 3;  // will work on any printer 
  //target_type = 42; // works with gs, faster, better quality
  
#ifdef Q_PRINTER_USE_TYPE42
  char* environment_preference = getenv("QT_TTFTOPS");
  if (environment_preference) {
    if (QString(environment_preference) == "42") 
      target_type = 42;
    else if (QString(environment_preference) == "3") 
      target_type = 3;
    else 
      qWarning("The value of QT_TTFTOPS must be 42 or 3");
  }
#endif
  offset_table = (unsigned char*) data.data(); /* first 12 bytes */
    
  /* Determine how many directory entries there are. */
  numTables = getUSHORT( offset_table + 4 );

  /* Extract information from the "Offset" table. */
  TTVersion = getFixed( offset_table );
    
  /* Load the "head" table and extract information from it. */
  ptr = getTable("head");
  if ( !ptr ) {
      defective = TRUE;
      return;
  }
  MfrRevision = getFixed( ptr + 4 );		/* font revision number */
  unitsPerEm = getUSHORT( ptr + 18 );
  HUPM = unitsPerEm / 2;
#ifdef DEBUG_TRUETYPE
  printf("unitsPerEm=%d",(int)unitsPerEm);
#endif
  llx = topost( getFWord( ptr + 36 ) );		/* bounding box info */
  lly = topost( getFWord( ptr + 38 ) );
  urx = topost( getFWord( ptr + 40 ) );
  ury = topost( getFWord( ptr + 42 ) );
  indexToLocFormat = getSHORT( ptr + 50 );	/* size of 'loca' data */
  if(indexToLocFormat != 0 && indexToLocFormat != 1) {
    qWarning("TrueType font is unusable because indexToLocFormat != 0");
    defective = TRUE;
    return;
  }
  if( getSHORT(ptr+52) != 0 ) {
    qWarning("TrueType font is unusable because glyphDataFormat != 0");
    defective = TRUE;
    return;
  }
    
  // === Load information from the "name" table ===
  
  /* Set default values to avoid future references to */
  /* undefined pointers. */
  
  psname = FullName = FamilyName = Version = Style = "unknown";
  Copyright = "No copyright notice";
  Trademark = "No trademark notice";

  BYTE* table_ptr = getTable("name");		/* pointer to table */
  if ( !table_ptr ) {
      defective = TRUE;
      return;
  }
  int numrecords = getUSHORT( table_ptr + 2 );	/* number of names */
  char* strings = (char *)table_ptr + getUSHORT( table_ptr + 4 ); /* start of string storage */
    
  BYTE* ptr2 = table_ptr + 6;
  for(int x=0; x < numrecords; x++,ptr2+=12) {
      int platform = getUSHORT(ptr2);
    //int encoding = getUSHORT(ptr2+2);
    //int language = getUSHORT(ptr2+4);
    int nameid   = getUSHORT(ptr2+6);
    int length   = getUSHORT(ptr2+8);
    int offset   = getUSHORT(ptr2+10);

    if( platform == 1 && nameid == 0 )
      Copyright.setLatin1(strings+offset,length);

    if( platform == 1 && nameid == 1 )
      FamilyName.setLatin1(strings+offset,length);

    if( platform == 1 && nameid == 2 ) 
      Style.setLatin1(strings+offset,length);

    if( platform == 1 && nameid == 4 ) 
      FullName.setLatin1(strings+offset,length);

    if( platform == 1 && nameid == 5 )
      Version.setLatin1(strings+offset,length);

    if( platform == 1 && nameid == 6 )
      psname.setLatin1(strings+offset,length);

    if( platform == 1 && nameid == 7 ) 
      Trademark.setLatin1(strings+offset,length);

  }

  //read_cmap(font);
    
  /* We need to have the PostScript table around. */

  post_table = getTable("post");
  if ( !post_table ) {
      defective = TRUE;
      return;
  }
  Fixed post_format = getFixed( post_table );

  if( post_format.whole != 2 || post_format.fraction != 0 ) {
    qWarning("TrueType font does not have a format 2.0 'post' table");
    defective = TRUE;
  }

  numGlyphs = getUSHORT( post_table + 32 );
  replacementList = makePSFontNameList( f, psname );
}

void QPSPrinterFontTTF::download(QTextStream& s)
{
    emitPSFontNameList( s, psname, replacementList );
  if (defective) {
    s << "% Font ";
    s << FullName;
    s << " cannot be downloaded\n";
    return;
  }

  // === write header ===
  int VMMin;
  int VMMax;
  
  s << "%%BeginFont: ";
  s << FullName;
  s << "\n";
  if( target_type == 42 ) {
    s << "%!PS-TrueTypeFont-"
      << TTVersion.whole
      << "."
      << TTVersion.fraction 
      << "-"
      << MfrRevision.whole 
      << "."
      << MfrRevision.fraction 
      << "\n";
  } else {
    /* If it is not a Type 42 font, we will use a different format. */
    s << "%!PS-Adobe-3.0 Resource-Font\n";
  }	/* See RBIIp 641 */

  if( Copyright != (char*)NULL ) {
    s << "%%Copyright: ";
    s << Copyright;
    s << "\n";
  }

  if( target_type == 42 )
    s << "%%Creator: Converted from TrueType to type 42 by Qt using PPR\n";
  else
    s << "%%Creator: Converted from TrueType by Qt using PPR\n";
  
  /* If VM usage information is available, print it. */
  if( target_type == 42 )
    {
      VMMin = (int)getULONG( post_table + 16 );
      VMMax = (int)getULONG( post_table + 20 );
      if( VMMin > 0 && VMMax > 0 )
	s << "%%VMUsage: " << VMMin << " " << VMMax << "\n";
    }
  
  /* Start the dictionary which will eventually */
  /* become the font. */
  if( target_type != 3 ) {
    s << "15 dict begin\n";
  }  else {
    s << "25 dict begin\n";
    
    /* Type 3 fonts will need some subroutines here. */
    s << "/_d{bind def}bind def\n";
    s << "/_m{moveto}_d\n";
    s << "/_l{lineto}_d\n";
    s << "/_cl{closepath eofill}_d\n";
    s << "/_c{curveto}_d\n";
    s << "/_sc{7 -1 roll{setcachedevice}{pop pop pop pop pop pop}ifelse}_d\n";
    s << "/_e{exec}_d\n";
  }
  
  s << "/FontName /";
  s << psname;
  s << " def\n";
  s << "/PaintType 0 def\n";
  
  if(target_type == 42)
    s << "/FontMatrix[1 0 0 1 0 0]def\n";
  else
    s << "/FontMatrix[.001 0 0 .001 0 0]def\n";
  
  s << "/FontBBox[";
  s<< llx;
  s << " ";
  s<< lly;
  s << " ";
  s<< urx;
  s << " ";
  s<< ury;
  s << "]def\n";

  s << "/FontType ";
  s<< target_type;
  s << " def\n";

  // === write encoding ===
  
  s << "/Encoding StandardEncoding def\n";

  // === write fontinfo dict ===

  /* We create a sub dictionary named "FontInfo" where we */
  /* store information which though it is not used by the */
  /* interpreter, is useful to some programs which will */
  /* be printing with the font. */
  s << "/FontInfo 10 dict dup begin\n";
  
  /* These names come from the TrueType font's "name" table. */
  s << "/FamilyName (";
  s << FamilyName;
  s << ") def\n";
  
  s << "/FullName (";
  s << FullName;
  s << ") def\n";
  
  s << "/Notice (";
  s << Copyright;
  s << " ";
  s << Trademark;
  s << ") def\n";
    
  /* This information is not quite correct. */
  s << "/Weight (";
  s << Style;
  s << ") def\n";
  
  /* Some fonts have this as "version". */
  s << "/Version (";
  s << Version;
  s << ") def\n";
  
  /* Some information from the "post" table. */
  Fixed ItalicAngle = getFixed( post_table + 4 );
  s << "/ItalicAngle ";
  s << ItalicAngle.whole;
  s << ".";
  s << ItalicAngle.fraction;
  s << " def\n";
  
  s << "/isFixedPitch ";
  s << (getULONG( post_table + 12 ) ? "true" : "false" );
  s << " def\n";
  
  s << "/UnderlinePosition ";
  s << (int)getFWord( post_table + 8 );
  s << " def\n";
  
  s << "/UnderlineThickness ";
  s << (int)getFWord( post_table + 10 );
  s << " def\n";
  
  s << "end readonly def\n";    

#ifdef Q_PRINTER_USE_TYPE42
  /* If we are generating a type 42 font, */
  /* emmit the sfnts array. */
  if( target_type == 42 )
    download_sfnts(s);
#endif  
  /* If we are generating a Type 3 font, we will need to */
  /* have the 'loca' and 'glyf' tables arround while */
  /* we are generating the CharStrings. */
  if(target_type == 3)
    {
      BYTE *ptr;			/* We need only one value */
      ptr = getTable("hhea");
      numberOfHMetrics = getUSHORT(ptr + 34);
      
      loca_table = getTable("loca");
      glyf_table = getTable("glyf");
      hmtx_table = getTable("hmtx");
    }
  
  // ===  CharStrings array ===

  s << "/CharStrings ";
  s << numGlyphs;
  s << " dict dup begin\n";
  
  // Emmit one key-value pair for each glyph. 
  for(int x=0; x < numGlyphs; x++) {
    if(target_type == 42)	{
      s << "/";
      s << glyphName(x);
      s << " ";
      s << x;
      s << " def\n";
    } else { /* type 3 */
      s << "/";
      s << glyphName(x);
      s << "{";
      charproc(x,s);
      s << "}_d\n";	/* "} bind def" */
    }
  }
  
  s << "end readonly def\n";

  // === trailer ===

  /* If we are generating a type 3 font, we need to provide */
  /* a BuildGlyph and BuildChar proceedures. */
  if( target_type == 3 ) {
    s << "\n";
    
    s << "/BuildGlyph\n";
    s << " {exch begin\n";		/* start font dictionary */
    s << " CharStrings exch\n";
    s << " 2 copy known not{pop /.notdef}if\n";
    s << " true 3 1 roll get exec\n";
    s << " end}_d\n";
    
    s << "\n";
    
    /* This proceedure is for compatiblity with */
    /* level 1 interpreters. */
    s << "/BuildChar {\n";
    s << " 1 index /Encoding get exch get\n";
    s << " 1 index /BuildGlyph get exec\n";
    s << "}_d\n";    	
    
    s << "\n";
    
  }
  
  /* If we are generating a type 42 font, we need to check to see */
  /* if this PostScript interpreter understands type 42 fonts.  If */
  /* it doesn't, we will hope that the Apple TrueType rasterizer */
  /* has been loaded and we will adjust the font accordingly. */
  /* I found out how to do this by examining a TrueType font */
  /* generated by a Macintosh.  That is where the TrueType interpreter */
  /* setup instructions and part of BuildGlyph came from. */
  else if( target_type == 42 ) {
    s << "\n";
    
    /* If we have no "resourcestatus" command, or FontType 42 */
    /* is unknown, leave "true" on the stack. */
    s << "systemdict/resourcestatus known\n";
    s << " {42 /FontType resourcestatus\n";
    s << "   {pop pop false}{true}ifelse}\n";
    s << " {true}ifelse\n";
    
    /* If true, execute code to produce an error message if */
    /* we can't find Apple's TrueDict in VM. */
    s << "{/TrueDict where{pop}{(%%[ Error: no TrueType rasterizer ]%%)= flush}ifelse\n";
    
    /* Since we are expected to use Apple's TrueDict TrueType */
    /* reasterizer, change the font type to 3. */    	
    s << "/FontType 3 def\n";
    
    /* Define a string to hold the state of the Apple */
    /* TrueType interpreter. */
    s << " /TrueState 271 string def\n";
    
    /* It looks like we get information about the resolution */
    /* of the printer and store it in the TrueState string. */
    s << " TrueDict begin sfnts save\n";
    s << " 72 0 matrix defaultmatrix dtransform dup\n";
    s << " mul exch dup mul add sqrt cvi 0 72 matrix\n";
    s << " defaultmatrix dtransform dup mul exch dup\n";
    s << " mul add sqrt cvi 3 -1 roll restore\n";
    s << " TrueState initer end\n";
    
    /* This BuildGlyph procedure will look the name up in the */
    /* CharStrings array, and then check to see if what it gets */
    /* is a procedure.  If it is, it executes it, otherwise, it */
    /* lets the TrueType rasterizer loose on it. */
    
    /* When this proceedure is executed the stack contains */
    /* the font dictionary and the character name.  We */
    /* exchange arguments and move the dictionary to the */
    /* dictionary stack. */
    s << " /BuildGlyph{exch begin\n";
    /* stack: charname */
    
    /* Put two copies of CharStrings on the stack and consume */
    /* one testing to see if the charname is defined in it, */
    /* leave the answer on the stack. */
    s << "  CharStrings dup 2 index known\n";
    /* stack: charname CharStrings bool */
    
    /* Exchange the CharStrings dictionary and the charname, */
    /* but if the answer was false, replace the character name */
    /* with ".notdef". */ 
    s << "    {exch}{exch pop /.notdef}ifelse\n";
    /* stack: CharStrings charname */
    
    /* Get the value from the CharStrings dictionary and see */
    /* if it is executable. */
    s << "  get dup xcheck\n";
    /* stack: CharStrings_entry */
    
    /* If is a proceedure.  Execute according to RBIIp 277-278. */
    s << "    {currentdict systemdict begin begin exec end end}\n";
    
    /* Is a TrueType character index, let the rasterizer at it. */
    s << "    {TrueDict begin /bander load cvlit exch TrueState render end}\n";
    
    s << "    ifelse\n";
    
    /* Pop the font's dictionary off the stack. */
    s << " end}bind def\n";
    
    /* This is the level 1 compatibility BuildChar procedure. */
    /* See RBIIp 281. */
    s << " /BuildChar{\n";
    s << "  1 index /Encoding get exch get\n";
    s << "  1 index /BuildGlyph get exec\n";
    s << " }bind def\n";    	
    
    /* Here we close the condition which is true */
    /* if the printer has no built-in TrueType */
    /* rasterizer. */
    s << "}if\n";
    s << "\n";
  } /* end of if Type 42 not understood. */
  
  s << "FontName currentdict end definefont pop\n";
  s << "%%EndFont\n";
}

BYTE* QPSPrinterFontTTF::getTable(const char* name) 
{
  BYTE *ptr;
  int x;
  
  /* We must search the table directory. */
  ptr = offset_table + 12;
  x=0;
  while (x != numTables) {
    if( strncmp((const char *)ptr,name,4) == 0 ) {
      ULONG offset,length;
      BYTE *table;
      
      offset = getULONG( ptr + 8 );
      length = getULONG( ptr + 12 );	    
      
      table = offset_table + offset;
      return table;
    }
    
    x++;
    ptr += 16;
  }

  return 0;
}

QString QPSPrinterFontTTF::glyphName(int charindex)
{
  USHORT c;
  if ((c=unicode_for_glyph(charindex)) != 0xffff) {
    //fprintf(stdout,"glyph %04x char %04x\n",charindex,c);
    for (int i=0; unicodetoglyph[i].u != 0xffff; i++) {
      //if (unicodetoglyph[i].u >  c) break;
      if (unicodetoglyph[i].u == c) {
	  //qDebug("using name %s for char %04x\n",unicodetoglyph[i].g,c);
	  return unicodetoglyph[i].g;
      }
    }
  }

  int GlyphIndex = (int)getUSHORT( post_table + 34 + (charindex * 2) );
  
  if( GlyphIndex <= 257 ) {		/* If a standard Apple name, */
      //qDebug("post name for glyph is %s 1\n",Apple_CharStrings[GlyphIndex]);
    return QString(Apple_CharStrings[GlyphIndex]);
  } else {			/* Otherwise, use one */
    				/* of the pascal strings. */
    GlyphIndex -= 258;
      
    /* Set pointer to start of Pascal strings. */
    char* ptr = (char*)(post_table + 34 + (numGlyphs * 2));
      
    int len = (int)*(ptr++);	/* Step thru the strings */
    while(GlyphIndex--) {	/* until we get to the one */
      			        /* that we want. */
      ptr += len;
      len = (int)*(ptr++);
    }
      
    return QString::fromLatin1(ptr,len);
  }
}

USHORT QPSPrinterFontTTF::unicode_for_glyph(int glyphindex)
{
  unsigned char* cmap = getTable("cmap");
  int pos = 0;

  //USHORT version = getUSHORT(cmap + pos); 
  pos += 2;
  USHORT nmaps   = getUSHORT(cmap + pos); pos += 2;

  //fprintf(stderr,"cmap version %d (should be 0), %d maps\n",version,nmaps);

  ULONG offset;
  int i;
  for (i=0; i<nmaps; i++) {
    USHORT platform = getUSHORT(cmap+pos); pos+=2;
    USHORT encoding = getUSHORT(cmap+pos); pos+=2;
           offset   = getULONG( cmap+pos); pos+=4;
	   //fprintf(stderr,"[%d] plat %d enc %d\n",i,platform,encoding);
    if (platform == 3 && encoding == 1) break; // unicode
  }
  if (i==nmaps) {
    //qWarning("Font does not have unicode encoding\n");
    return 0xffff; // no unicode encoding!
  }

  //fprintf(stderr,"Doing Unicode encoding\n");
  
  pos = offset;
  USHORT format = getUSHORT(cmap+pos); pos+=2;
  //fprintf(stderr,"Unicode cmap format %d\n",format);
  
  if (format != 4) {
    //qWarning("Unicode cmap format is not 4");
    return 0xffff;
  }
    
  pos += 2; // version
  pos += 2; // length
  USHORT segcount = getUSHORT(cmap+pos) / 2; pos+=2;

  //fprintf(stderr,"Unicode cmap seg count %d\n",segcount);

  // skip search data
  pos += 2; 
  pos += 2; 
  pos += 2; 

  unsigned char* endcode    = cmap + offset + 14;
  unsigned char* startcode  = cmap + offset + 16 + 2*segcount;
  unsigned char* iddelta    = cmap + offset + 16 + 4*segcount;
  unsigned char* idrangeoff = cmap + offset + 16 + 6*segcount;
  //unsigned char* glyphid    = cmap + offset + 16 + 8*segcount;
  for (i=0; i<segcount; i++) {
    USHORT endcode_i    = getUSHORT(endcode   +2*i);
    USHORT startcode_i  = getUSHORT(startcode +2*i);
    USHORT iddelta_i    = getUSHORT(iddelta   +2*i);
    USHORT idrangeoff_i = getUSHORT(idrangeoff+2*i);
    
    //fprintf(stderr,"[%d] %04x-%04x (%d %d)\n",
    //    i,startcode_i,endcode_i,iddelta_i,idrangeoff_i);

    if (endcode_i == 0xffff) break; // last dummy segment

    if (idrangeoff_i == 0) {
      for (USHORT c = startcode_i; c <= endcode_i; c++) {
	USHORT g = c + iddelta_i; // glyph index
	if (g >= numGlyphs) {
	  qWarning("incorrect glyph index in cmap");
	  return 0xffff;
	}
	if (g == glyphindex) return c;
      }
    } else {
      for (USHORT c = startcode_i; c <= endcode_i; c++) {
	USHORT g = getUSHORT(idrangeoff+2*i 
			     + 2*(c - startcode_i)
			     + idrangeoff_i);
	if (g >= numGlyphs) {
	  qWarning("incorrect glyph index in cmap");
	  return 0xffff;
	}
	if (g == glyphindex) return c;
      }
    }
  }
  return 0xffff;
}


#ifdef Q_PRINTER_USE_TYPE42
// ****************** SNFTS ROUTINES *******

/*-------------------------------------------------------------------
** sfnts routines
** These routines generate the PostScript "sfnts" array which 
** contains one or more strings which contain a reduced version
** of the TrueType font.
**
** A number of functions are required to accomplish this rather
** complicated task. 
-------------------------------------------------------------------*/

// Write a BYTE as a hexadecimal value as part of the sfnts array.

void QPSPrinterFontTTF::sfnts_pputBYTE(BYTE n,QTextStream& s, 
				  int& string_len, int& line_len, bool& in_string)
{
  static const char hexdigits[]="0123456789ABCDEF";
  
  if(!in_string) {
    s << "<";
    string_len=0;
    line_len++;
    in_string=TRUE;
  }

  s << hexdigits[ n / 16 ] ;
  s << hexdigits[ n % 16 ] ;
  string_len++;
  line_len+=2;

  if(line_len > 70) {
    s << "\n";
    line_len=0;
  }
}
   
// Write a USHORT as a hexadecimal value as part of the sfnts array.

void QPSPrinterFontTTF::sfnts_pputUSHORT(USHORT n,QTextStream& s,
				  int& string_len, int& line_len, bool& in_string)
{
  sfnts_pputBYTE(n / 256,s, string_len, line_len, in_string);
  sfnts_pputBYTE(n % 256,s, string_len, line_len, in_string);
}


// Write a ULONG as part of the sfnts array.

void QPSPrinterFontTTF::sfnts_pputULONG(ULONG n,QTextStream& s,
				  int& string_len, int& line_len, bool& in_string)
{
  int x1 = n % 256;   n /= 256;
  int x2 = n % 256;   n /= 256;
  int x3 = n % 256;   n /= 256;
  
  sfnts_pputBYTE(n,s , string_len, line_len, in_string);
  sfnts_pputBYTE(x3,s, string_len, line_len, in_string);
  sfnts_pputBYTE(x2,s, string_len, line_len, in_string);
  sfnts_pputBYTE(x1,s, string_len, line_len, in_string);
}

/*
** This is called whenever it is 
** necessary to end a string in the sfnts array.
**
** (The array must be broken into strings which are
** no longer than 64K characters.)
*/
void QPSPrinterFontTTF::sfnts_end_string(QTextStream& s,
				    int& string_len, int& line_len, bool& in_string)
{
  if(in_string)	{
    string_len=0;		/* fool sfnts_pputBYTE() */
    
    // s << "\n% dummy byte:\n";

    // extra byte for pre-2013 compatibility 
    sfnts_pputBYTE(0, s, string_len, line_len, in_string);	

    s << ">";
    line_len++;
  }
  
  in_string=FALSE;
}

/*
** This is called at the start of each new table.
** The argement is the length in bytes of the table
** which will follow.  If the new table will not fit
** in the current string, a new one is started.
*/
void QPSPrinterFontTTF::sfnts_new_table(ULONG length,QTextStream& s,
				   int& string_len, int& line_len, bool& in_string)
{
  if( (string_len + length) > 65528 )    
    sfnts_end_string(s, string_len, line_len, in_string);
}

/*
** We may have to break up the 'glyf' table.  That is the reason
** why we provide this special routine to copy it into the sfnts
** array.
*/
void QPSPrinterFontTTF::sfnts_glyf_table(ULONG oldoffset, 
				    ULONG correct_total_length,
				    QTextStream& s,
				    int& string_len, int& line_len, bool& in_string)

{
  int x;
  ULONG off;
  ULONG length;
  int c;
  ULONG total=0;		/* running total of bytes written to table */
  
  loca_table = getTable("loca");

  int font_off = oldoffset;
  
  /* Copy the glyphs one by one */
  for(x=0; x < numGlyphs; x++) {
    /* Read the glyph offset from the index-to-location table. */
    if(indexToLocFormat == 0) {
      off = getUSHORT( loca_table + (x * 2) );
      off *= 2;
      length = getUSHORT( loca_table + ((x+1) * 2) );
      length *= 2;
      length -= off;
    } else {
      off = getULONG( loca_table + (x * 4) );
      length = getULONG( loca_table + ((x+1) * 4) );
      length -= off;
    }
      
    //  fprintf(stderr,"glyph length=%d",(int)length);
      
    /* Start new string if necessary. */
    sfnts_new_table( (int)length, s, string_len, line_len, in_string );
      
    /* 
    ** Make sure the glyph is padded out to a
    ** two byte boundary.
    */
    if( length % 2 ) {
      qWarning("TrueType font contains a 'glyf' table without 2 byte padding");
      defective = TRUE;
      return;
    }
      
    /* Copy the bytes of the glyph. */
    while( length-- ) {
      c = offset_table[ font_off ];
      font_off++;
	
      sfnts_pputBYTE(c, s, string_len, line_len, in_string);
      total++;		/* add to running total */
    }	    
  }

  /* Pad out to full length from table directory */
  while( total < correct_total_length )	{
    sfnts_pputBYTE(0, s, string_len, line_len, in_string);
    total++;
  }

  /* Look for unexplainable descrepancies between sizes */ 
  if( total != correct_total_length ) {
    qWarning("QPSPrinterFontTTF::sfnts_glyf_table: total != correct_total_length");
    defective = TRUE;
    return;
  }
}

/*
** Here is the routine which ties it all together.
**
** Create the array called "sfnts" which 
** holds the actual TrueType data.
*/

void QPSPrinterFontTTF::download_sfnts(QTextStream& s)
{
  // tables worth including in a type 42 font
  char *table_names[]= {
    "cvt ",
    "fpgm",
    "glyf",
    "head",
    "hhea",
    "hmtx",
    "loca",
    "maxp",
    "prep"
  };

  struct {			/* The location of each of */
    ULONG oldoffset;	/* the above tables. */
    ULONG newoffset;
    ULONG length;
    ULONG checksum;
  } tables[9];
    	
  int c;			/* Input character. */
  int diff;
  int count;			/* How many `important' tables did we find? */
  
  BYTE* ptr = offset_table + 12; // original table directory
  ULONG nextoffset=0;
  count=0;
    
  /* 
  ** Find the tables we want and store there vital
  ** statistics in tables[].
  */
  for(int x=0; x < 9; x++ ) {
    do {
      diff = strncmp( (char*)ptr, table_names[x], 4 );

      if( diff > 0 ) {		/* If we are past it. */
	tables[x].length = 0;
	diff = 0;		
      }
      else if( diff < 0 ) {		/* If we haven't hit it yet. */
	ptr += 16;
      }
      else if( diff == 0 ) {	/* Here it is! */
	tables[x].newoffset = nextoffset;
	tables[x].checksum = getULONG( ptr + 4 );
	tables[x].oldoffset = getULONG( ptr + 8 );
	tables[x].length = getULONG( ptr + 12 );
	nextoffset += ( ((tables[x].length + 3) / 4) * 4 );
	count++;
	ptr += 16;	    	
      }
    } while(diff != 0);
  } /* end of for loop which passes over the table directory */
  
  /* Begin the sfnts array. */

  s << "/sfnts[<";

  bool in_string=TRUE;
  int  string_len=0;
  int  line_len=8;
  
  /* Generate the offset table header */
  /* Start by copying the TrueType version number. */
  ptr = offset_table;
  for(int x=0; x < 4; x++)
    sfnts_pputBYTE( *(ptr++) , s, string_len, line_len, in_string );
  
  /* Now, generate those silly numTables numbers. */
  sfnts_pputUSHORT(count,s, string_len, line_len, in_string);		/* number of tables */
  if( count == 9 ) {
    sfnts_pputUSHORT(7,s, string_len, line_len, in_string);		/* searchRange */
    sfnts_pputUSHORT(3,s, string_len, line_len, in_string);		/* entrySelector */
    sfnts_pputUSHORT(81,s, string_len, line_len, in_string);		/* rangeShift */
  }    
  else {
    qWarning("Fewer than 9 tables selected"); 	
  }
  
  /* Now, emmit the table directory. */
  for(int x=0; x < 9; x++) {
    if( tables[x].length == 0 )	/* Skip missing tables */
      continue;
      
    /* Name */
    sfnts_pputBYTE( table_names[x][0], s, string_len, line_len, in_string);
    sfnts_pputBYTE( table_names[x][1], s, string_len, line_len, in_string);
    sfnts_pputBYTE( table_names[x][2], s, string_len, line_len, in_string);
    sfnts_pputBYTE( table_names[x][3], s, string_len, line_len, in_string);
    
    /* Checksum */
    sfnts_pputULONG( tables[x].checksum, s, string_len, line_len, in_string );
    
    /* Offset */
    sfnts_pputULONG( tables[x].newoffset + 12 + (count * 16), s,
		     string_len, line_len, in_string );
    
    /* Length */
    sfnts_pputULONG( tables[x].length, s,
		     string_len, line_len, in_string );
  }
  
  /* Now, send the tables */
  for(int x=0; x < 9; x++) {
    if( tables[x].length == 0 )	/* skip tables that aren't there */
      continue;
      
    /* 'glyf' table gets special treatment */
    if( strcmp(table_names[x],"glyf")==0 ) {
      sfnts_glyf_table(tables[x].oldoffset,tables[x].length, s,
		       string_len, line_len, in_string);
    } else { // other tables should not exceed 64K (not always true; Sivan)
      if( tables[x].length > 65535 ) {
	qWarning("TrueType font has a table which is too long");	    
	defective = TRUE;
	return;
      }
	  
      /* Start new string if necessary. */
      sfnts_new_table(tables[x].length, s,
		      string_len, line_len, in_string);
	  
      int font_off = tables[x].oldoffset;
      /* Copy the bytes of the table. */
      for( int y=0; y < (int)tables[x].length; y++ ) {
	c = offset_table[ font_off ];
	font_off++;
	      
	sfnts_pputBYTE(c, s, string_len, line_len, in_string);
      }	    
    }
      
    /* Padd it out to a four byte boundary. */
    int y=tables[x].length;
    while( (y % 4) != 0 ) {
      sfnts_pputBYTE(0, s, string_len, line_len, in_string);
      y++;
    }
      
  } /* End of loop for all tables */
  
  /* Close the array. */
  sfnts_end_string(s, string_len, line_len, in_string);    
  s << "]def\n";
} 
#endif

// ****************** Type 3 CharProcs *******

/*
** This routine is used to break the character
** procedure up into a number of smaller 
** procedures.  This is necessary so as not to
** overflow the stack on certain level 1 interpreters.
**
** Prepare to push another item onto the stack,
** starting a new proceedure if necessary.
**
** Not all the stack depth calculations in this routine
** are perfectly accurate, but they do the job.
*/
static int stack_depth = 0;
static void stack(int num_pts, int newnew, QTextStream& s)
{
  if( num_pts > 25 ) {		/* Only do something of we will */
    				/* have a log of points. */
    if(stack_depth == 0) {
      s << "{";
      stack_depth=1;
    }

    stack_depth += newnew;		/* Account for what we propose to add */

    if(stack_depth > 100) {
      s << "}_e{";
      stack_depth = 3 + newnew;	/* A rough estimate */
    }
  }
}

static void stack_end(QTextStream& s)			/* called at end */
{
  if(stack_depth) {
    s << "}_e";
    stack_depth=0;
  }
}

// postscript drawing commands

static void PSMoveto(FWord x, FWord y, QTextStream& ts) 
{
  ts << x;
  ts << " ";
  ts << y;
  ts << " _m\n";
}

static void PSLineto(FWord x, FWord y, QTextStream& ts) 
{
  ts << x;
  ts << " ";
  ts << y;
  ts << " _l\n";
}

/* Emmit a PostScript "curveto" command. */
static void PSCurveto(FWord* xcoor, FWord* ycoor,
		      FWord x, FWord y, int s, int t, QTextStream& ts) 
{
  int N, i;
  double sx[3], sy[3], cx[4], cy[4];
  
  N = t-s+2;
  for(i=0; i<N-1; i++) {
    sx[0] = i==0?xcoor[s-1]:(xcoor[i+s]+xcoor[i+s-1])/2;
    sy[0] = i==0?ycoor[s-1]:(ycoor[i+s]+ycoor[i+s-1])/2;
    sx[1] = xcoor[s+i];
    sy[1] = ycoor[s+i];
    sx[2] = i==N-2?x:(xcoor[s+i]+xcoor[s+i+1])/2;
    sy[2] = i==N-2?y:(ycoor[s+i]+ycoor[s+i+1])/2;
    cx[3] = sx[2];
    cy[3] = sy[2];
    cx[1] = (2*sx[1]+sx[0])/3;
    cy[1] = (2*sy[1]+sy[0])/3;
    cx[2] = (sx[2]+2*sx[1])/3;
    cy[2] = (sy[2]+2*sy[1])/3;

    ts << (int)cx[1];
    ts << " ";
    ts << (int)cy[1];
    ts << " ";
    ts << (int)cx[2];
    ts << " ";
    ts << (int)cy[2];
    ts << " ";
    ts << (int)cx[3];
    ts << " ";
    ts << (int)cy[3];
    ts << " _c\n";
  }
}

/* The PostScript bounding box. */
/* Variables to hold the character data. */

//void load_char(struct TTFONT *font, BYTE *glyph);
//void clear_data();

//void PSMoveto(FWord x, FWord y, QTextStream& ts);
//void PSLineto(FWord x, FWord y, QTextStream& ts);
//void PSCurveto(FWord x, FWord y, int s, int t, QTextStream& ts);

//double area(FWord *x, FWord *y, int n);
//int nextinctr(int co, int ci);
//int nextoutctr(int co);
//int nearout(int ci);
//double intest(int co, int ci);
#define sqr(x) ((x)*(x))

#define NOMOREINCTR -1
#define NOMOREOUTCTR -1

/*
** Find the area of a contour?
*/
static double area(FWord *x, FWord *y, int n)
{ 
  int i;
  double sum;
      
  sum=x[n-1]*y[0]-y[n-1]*x[0];
  for (i=0; i<=n-2; i++) sum += x[i]*y[i+1] - y[i]*x[i+1];
  return sum;
}   

static int nextoutctr(int /*co*/, charproc_data* cd)
{
  int j;
  
  for(j=0; j<cd->num_ctr; j++) 
    if (cd->check_ctr[j]==0 && cd->area_ctr[j] < 0) {
      cd->check_ctr[j]=1;
      return j;
    }
  
  return NOMOREOUTCTR;
} /* end of nextoutctr() */

static int nextinctr(int co, int /*ci*/, charproc_data* cd)
{
  int j;
  
  for(j=0; j<cd->num_ctr; j++)
    if (cd->ctrset[2*j+1]==co) 
      if (cd->check_ctr[ cd->ctrset[2*j] ]==0) {
	cd->check_ctr[ cd->ctrset[2*j] ]=1;
	return cd->ctrset[2*j];
      }
  
  return NOMOREINCTR;
}

static double intest(int co, int ci, charproc_data* cd)
{
  int i, j, start, end;
  double r1, r2, a;
  FWord xi[3], yi[3];
  
  j=start=(co==0)?0:(cd->epts_ctr[co-1]+1);
  end=cd->epts_ctr[co];
  i=(ci==0)?0:(cd->epts_ctr[ci-1]+1);
  xi[0] = cd->xcoor[i];
  yi[0] = cd->ycoor[i];
  r1=sqr(cd->xcoor[start] - xi[0]) 
     + sqr(cd->ycoor[start] - yi[0]);
  
  for (i=start; i<=end; i++) {
    r2 = sqr(cd->xcoor[i] - xi[0])+sqr(cd->ycoor[i] - yi[0]);
    if (r2 < r1) {
      r1=r2; j=i;
    }
  }
  xi[1]=cd->xcoor[j-1]; yi[1]=cd->ycoor[j-1];
  xi[2]=cd->xcoor[j+1]; yi[2]=cd->ycoor[j+1];
  if (j==start) { xi[1]=cd->xcoor[end]; yi[1]=cd->ycoor[end]; }
  if (j==end) { xi[2]=cd->xcoor[start]; yi[2]=cd->ycoor[start]; }
  a=area(xi, yi, 3);
  
  return a;
} /* end of intest() */

/*
** find the nearest out contour to a specified in contour.
*/
static int nearout(int ci, charproc_data* cd)
{
  int k = 0;			/* !!! is this right? */
  int co;
  double a, a1=0;
  
  for (co=0; co < cd->num_ctr; co++) {
    if(cd->area_ctr[co] < 0) {
      a=intest(co,ci, cd);
      if (a<0 && a1==0)	{
	k=co;
	a1=a;
      }
      if(a<0 && a1!=0 && a>a1) {
	k=co;
	a1=a;
      }
    }
  }
			
  return k;
} /* end of nearout() */


/*
** We call this routine to emmit the PostScript code
** for the character we have loaded with load_char().
*/
void PSConvert(QTextStream& s, charproc_data* cd)
{    
  int i,j,k,fst,start_offpt;
  int end_offpt=0;
  
  cd->area_ctr = new double[cd->num_ctr];
  memset(cd->area_ctr, 0, (cd->num_ctr*sizeof(double)));

  cd->check_ctr = new char[cd->num_ctr];
  memset(cd->check_ctr, 0, (cd->num_ctr*sizeof(char)));

  cd->ctrset = new int[2*(cd->num_ctr)];
  memset(cd->ctrset, 0, (cd->num_ctr*2*sizeof(int)));
  
  cd->check_ctr[0]=1;
  cd->area_ctr[0]=area(cd->xcoor, cd->ycoor, cd->epts_ctr[0]+1);
  
  for (i=1; i<cd->num_ctr; i++) 
    cd->area_ctr[i]=area(cd->xcoor+cd->epts_ctr[i-1]+1, 
			 cd->ycoor+cd->epts_ctr[i-1]+1, 
			 cd->epts_ctr[i]-cd->epts_ctr[i-1]);
  
  for (i=0; i<cd->num_ctr; i++) {
    if (cd->area_ctr[i]>0) {
      cd->ctrset[2*i]=i; 
      cd->ctrset[2*i+1]=nearout(i,cd);
    } else {
      cd->ctrset[2*i]=-1; 
      cd->ctrset[2*i+1]=-1;
    }
  }

  /* Step thru the coutours. */
  /* I believe that a contour is a detatched */
  /* set of curves and lines. */
  i=j=k=0;
  while (i < cd->num_ctr ) {
    fst = j = (k==0) ? 0 : (cd->epts_ctr[k-1]+1);
    
    /* Move to the first point on the contour. */
    stack(cd->num_pts,3,s);
    PSMoveto(cd->xcoor[j],cd->ycoor[j],s);
    start_offpt = 0;		/* No off curve points yet. */
      
    /* Step thru the remaining points of this contour. */
    for(j++; j <= cd->epts_ctr[k]; j++) {
      if (!(cd->tt_flags[j]&1))	{ /* Off curve */
	if (!start_offpt) 
	  { start_offpt = end_offpt = j; }
	else
	  end_offpt++;
      } else {			/* On Curve */
	if (start_offpt) {
	  stack(cd->num_pts,7,s);
	  PSCurveto(cd->xcoor,cd->ycoor,
		    cd->xcoor[j],cd->ycoor[j],
		    start_offpt,end_offpt,s);
	  start_offpt = 0;
	} else {
	  stack(cd->num_pts,3,s);
	  PSLineto(cd->xcoor[j], cd->ycoor[j],s);
	}
      }
    }
      
    /* Do the final curve or line */
    /* of this coutour. */
    if (start_offpt) {
      stack(cd->num_pts,7,s); 
      PSCurveto(cd->xcoor,cd->ycoor,
		cd->xcoor[fst],cd->ycoor[fst],
		start_offpt,end_offpt,s);
    } else {
      stack(cd->num_pts,3,s); 
      PSLineto(cd->xcoor[fst],cd->ycoor[fst],s);
    }
      
    k=nextinctr(i,k,cd);
      
    if (k==NOMOREINCTR)
      i=k=nextoutctr(i,cd);
      
    if (i==NOMOREOUTCTR)
      break;
  }
  
  /* Now, we can fill the whole thing. */
  stack(cd->num_pts,1,s);
  s << "_cl";		/* "closepath eofill" */
  
  /* Free our work arrays. */
  delete cd->area_ctr;
  delete cd->check_ctr;
  delete cd->ctrset;
} 


/*
** Load the simple glyph data pointed to by glyph.
** The pointer "glyph" should point 10 bytes into
** the glyph data.
*/
void QPSPrinterFontTTF::charprocLoad(BYTE *glyph, charproc_data* cd)
{
  int x;
  BYTE c, ct;
  
  /* Read the contour endpoints list. */
  cd->epts_ctr = new int[cd->num_ctr];
  //cd->epts_ctr = (int *)myalloc(cd->num_ctr,sizeof(int));
  for (x = 0; x < cd->num_ctr; x++) {
    cd->epts_ctr[x] = getUSHORT(glyph);
    glyph += 2;
  }
  
  /* From the endpoint of the last contour, we can */
  /* determine the number of points. */
  cd->num_pts = cd->epts_ctr[cd->num_ctr-1]+1;
#ifdef DEBUG_TRUETYPE
  fprintf(stderr,"num_pts=%d",num_pts);
  fprintf(stderr,"% num_pts=%d\n",num_pts);
#endif

  /* Skip the instructions. */
  x = getUSHORT(glyph);
  glyph += 2;
  glyph += x;

  /* Allocate space to hold the data. */
  //cd->tt_flags = (BYTE *)myalloc(num_pts,sizeof(BYTE));
  //cd->xcoor    = (FWord *)myalloc(num_pts,sizeof(FWord));
  //cd->ycoor    = (FWord *)myalloc(num_pts,sizeof(FWord));
  cd->tt_flags = new BYTE[cd->num_pts];
  cd->xcoor    = new FWord[cd->num_pts];
  cd->ycoor    = new FWord[cd->num_pts];

  /* Read the flags array, uncompressing it as we go. */
  /* There is danger of overflow here. */
  for (x = 0; x < cd->num_pts; ) {
    cd->tt_flags[x++] = c = *(glyph++);
    
    if (c&8) {		/* If next byte is repeat count, */
      ct = *(glyph++);
      
      if( (x + ct) > cd->num_pts ) {
	qWarning("Fatal Error in TT flags");
	return;
      }
      
      while (ct--)
	cd->tt_flags[x++] = c;
    }
  }

  /* Read the x coordinates */
  for (x = 0; x < cd->num_pts; x++) {
    if (cd->tt_flags[x] & 2) {		/* one byte value with */
                                        /* external sign */
      c = *(glyph++);
      cd->xcoor[x] = (cd->tt_flags[x] & 0x10) ? c : (-1 * (int)c);
    } else if(cd->tt_flags[x] & 0x10) {	/* repeat last */
      cd->xcoor[x] = 0;
    } else {				/* two byte signed value */
      cd->xcoor[x] = getFWord(glyph);
      glyph+=2;
    }
  }

  /* Read the y coordinates */
  for(x = 0; x < cd->num_pts; x++) {
    if (cd->tt_flags[x] & 4) {		/* one byte value with */
      				        /* external sign */
      c = *(glyph++);
      cd->ycoor[x] = (cd->tt_flags[x] & 0x20) ? c : (-1 * (int)c);
    } else if (cd->tt_flags[x] & 0x20) {	/* repeat last value */
      cd->ycoor[x] = 0;
    } else {			/* two byte signed value */
      cd->ycoor[x] = getUSHORT(glyph);
      glyph+=2;
    }
  }

  /* Convert delta values to absolute values. */
  for(x = 1; x < cd->num_pts; x++) {
    cd->xcoor[x] += cd->xcoor[x-1];
    cd->ycoor[x] += cd->ycoor[x-1];
  }

  for(x=0; x < cd->num_pts; x++) {
    cd->xcoor[x] = topost(cd->xcoor[x]);
    cd->ycoor[x] = topost(cd->ycoor[x]);
  }
}

#define ARG_1_AND_2_ARE_WORDS 1
#define ARGS_ARE_XY_VALUES 2
#define ROUND_XY_TO_GRID 4
#define WE_HAVE_A_SCALE 8
/* RESERVED 16 */
#define MORE_COMPONENTS 32
#define WE_HAVE_AN_X_AND_Y_SCALE 64
#define WE_HAVE_A_TWO_BY_TWO 128
#define WE_HAVE_INSTRUCTIONS 256
#define USE_MY_METRICS 512

/*
** Emmit PostScript code for a composite character.
*/
void QPSPrinterFontTTF::charprocComposite(BYTE *glyph, QTextStream& s)
{
  USHORT flags;
  USHORT glyphIndex;
  int arg1;
  int arg2;
  USHORT xscale;
  USHORT yscale;
  USHORT scale01;
  USHORT scale10;
    
  /* Once around this loop for each component. */
  do {
    flags = getUSHORT(glyph);	/* read the flags word */ 
    glyph += 2;

    glyphIndex = getUSHORT(glyph);	/* read the glyphindex word */
    glyph += 2;
	
    if(flags & ARG_1_AND_2_ARE_WORDS) {
                                /* The tt spec. seems to say these are signed. */
      arg1 = getSHORT(glyph);
      glyph += 2;
      arg2 = getSHORT(glyph);
      glyph += 2;
    } else {			/* The tt spec. does not clearly indicate */
    	    			/* whether these values are signed or not. */
      arg1 = *(glyph++);
      arg2 = *(glyph++);
    }

    if(flags & WE_HAVE_A_SCALE) {
      xscale = yscale = getUSHORT(glyph);
      glyph += 2;	    
      scale01 = scale10 = 0;
    } else if(flags & WE_HAVE_AN_X_AND_Y_SCALE) {
      xscale = getUSHORT(glyph);
      glyph += 2;
      yscale = getUSHORT(glyph);
      glyph += 2;
      scale01 = scale10 = 0;
    } else if(flags & WE_HAVE_A_TWO_BY_TWO) {
      xscale = getUSHORT(glyph);
      glyph += 2;
      scale01 = getUSHORT(glyph);
      glyph += 2;
      scale10 = getUSHORT(glyph);
      glyph += 2;
      yscale = getUSHORT(glyph);
      glyph += 2;
    } else {
      xscale = yscale = scale01 = scale10 = 0;
    }

    /* Debugging */
#ifdef DEBUG_TRUETYPE
    fprintf(stderr,"% flags=%d, arg1=%d, arg2=%d, xscale=%d, yscale=%d, scale01=%d, scale10=%d\n",
	    (int)flags,arg1,arg2,(int)xscale,(int)yscale,(int)scale01,(int)scale10);
#endif
    
    /* If we have an (X,Y) shif and it is non-zero, */
    /* translate the coordinate system. */
    if( flags & ARGS_ARE_XY_VALUES ) {
      if( arg1 != 0 || arg2 != 0 ) {
	s <<"gsave ";
	s << topost(arg1);
	s << " ";
	s << topost(arg2);
	s << " translate\n";
	
	//fprintf(stderr,"gsave %d %d translate\n", topost(arg1), topost(arg2) );
      }
    } else {
      s << "% unimplemented shift, arg1=";
      s << arg1;
      s << ", arg2=";
      s << arg2;
      s << "\n";
    }

    /* Invoke the CharStrings procedure to print the component. */
    s << "false CharStrings /";
    s << glyphName(glyphIndex);
    s << " get exec\n";

    //	printf("false CharStrings /%s get exec\n",
    //ttfont_CharStrings_getname(font,glyphIndex));

    /* If we translated the coordinate system, */
    /* put it back the way it was. */
    if( flags & ARGS_ARE_XY_VALUES && (arg1 != 0 || arg2 != 0) )
      s <<"grestore ";
  } while(flags & MORE_COMPONENTS);
}

/*
** Return a pointer to a specific glyph's data.
*/
BYTE* QPSPrinterFontTTF::charprocFindGlyphData(int charindex)
{
  ULONG off;
  ULONG length;

  /* Read the glyph offset from the index to location table. */
  if(indexToLocFormat == 0) {
    off = getUSHORT( loca_table + (charindex * 2) );
    off *= 2;
    length = getUSHORT( loca_table + ((charindex+1) * 2) );
    length *= 2;
    length -= off;
  } else {
    off = getULONG( loca_table + (charindex * 4) );
    length = getULONG( loca_table + ((charindex+1) * 4) );
    length -= off;
  }

  if(length > 0)
    return glyf_table + off;
  else
    return (BYTE*)NULL;
}

void QPSPrinterFontTTF::charproc(int charindex, QTextStream& s)
{
  int llx,lly,urx,ury;
  int advance_width;
  charproc_data cd;

#ifdef DEBUG_TRUETYPE
  s << "% tt_type3_charproc for ";
  s << charindex;
  s << "\n";
#endif

  /* Get a pointer to the data. */
  BYTE* glyph = charprocFindGlyphData( charindex );

  /* If the character is blank, it has no bounding box, */
  /* otherwise read the bounding box. */
  if( glyph == (BYTE*)NULL ) {
    llx=lly=urx=ury=0;	/* A blank char has an all zero BoundingBox */
    cd.num_ctr=0;		/* Set this for later if()s */
  } else {
    /* Read the number of contours. */
    cd.num_ctr = getSHORT(glyph);
      
    /* Read PostScript bounding box. */
    llx = getFWord(glyph + 2);
    lly = getFWord(glyph + 4);
    urx = getFWord(glyph + 6);
    ury = getFWord(glyph + 8);     
      
    /* Advance the pointer. */
    glyph += 10;
  }
  
  /* If it is a simple character, load its data. */
  if (cd.num_ctr > 0)
    charprocLoad(glyph, &cd);
  else
    cd.num_pts=0;
  
  /* Consult the horizontal metrics table to determine */
  /* the character width. */
  if( charindex < numberOfHMetrics )
    advance_width = getuFWord( hmtx_table + (charindex * 4) );    	
  else
    advance_width = getuFWord( hmtx_table + ((numberOfHMetrics-1) * 4) );
  
  /* Execute setcachedevice in order to inform the font machinery */
  /* of the character bounding box and advance width. */
  stack(cd.num_pts,7,s);
  s << topost(advance_width);
  s << " 0 ";
  s << topost(llx);
  s << " ";
  s << topost(lly);
  s << " ";
  s << topost(urx);
  s << " ";
  s << topost(ury);
  s << " _sc\n";

  /* If it is a simple glyph, convert it, */
  /* otherwise, close the stack business. */
  if( cd.num_ctr > 0 ) {        // simple
    PSConvert(s,&cd);
    delete cd.tt_flags;
    delete cd.xcoor;
    delete cd.ycoor;
    delete cd.epts_ctr;
  } else if( cd.num_ctr < 0 ) { // composite 
    charprocComposite(glyph,s);
  }
  
  stack_end(s);
} /* end of tt_type3_charproc() */


// ================== PFA ====================

class QPSPrinterFontPFA
  : public QPSPrinterFontPrivate {
public:
  QPSPrinterFontPFA(const QFont &f, QByteArray& data);
  virtual void    download(QTextStream& s);
private:
  QByteArray     data;
};

QPSPrinterFontPFA::QPSPrinterFontPFA(const QFont &f, QByteArray& d)
{
  data = d;

  int pos = 0;
  char* p = data.data();
  QString fontname;
  
  if (p[ pos ] != '%' || p[ pos+1 ] != '!') { // PFA marker
    qWarning("invalid pfa file");
    return;
  }
  
  char* fontnameptr = strstr(p+pos,"/FontName");
  if (fontnameptr == NULL)
    return;
  
  fontnameptr += strlen("/FontName") + 1;
  while (*fontnameptr == ' ' || *fontnameptr == '/') fontnameptr++;
  int l=0;
  while (fontnameptr[l] != ' ') l++;
  
  psname = QString::fromLatin1(fontnameptr,l);
  replacementList = makePSFontNameList( f, psname );
}

void QPSPrinterFontPFA::download(QTextStream& s)
{
  char* p = data.data();

  emitPSFontNameList( s, psname, replacementList );
  s << "% Font resource\n";
  for (int i=0; i < (int)data.size(); i++) s << p[i];
  s << "% End of font resource\n";
}

// ================== PFB ====================

class QPSPrinterFontPFB
  : public QPSPrinterFontPrivate {
public:
  QPSPrinterFontPFB(const QFont &f, QByteArray& data);
  virtual void    download(QTextStream& s);
private:
  QByteArray     data;
};

QPSPrinterFontPFB::QPSPrinterFontPFB(const QFont &f, QByteArray& d)
{
  data = d;

  int pos = 0;
  int len;
  int typ;
  unsigned char* p = (unsigned char*) data.data();
  QString fontname;
  
  if (p[ pos ] != 0x80) { // PFB marker
    qWarning("pfb file does not start with 0x80");
    return;
  }
  pos++;
  typ = p[ pos ]; // 1=ascii 2=binary 3=done
  pos++;
  len = p[ pos ];	    pos++;
  len |= (p[ pos ] << 8) ; pos++;
  len |= (p[ pos ] << 16); pos++;
  len |= (p[ pos ] << 24); pos++;
  
  //printf("font block type %d len %d\n",typ,len);
  
  char* fontnameptr = strstr((char*)p+pos,"/FontName");
  if (fontnameptr == NULL)
    return;
  
  fontnameptr += strlen("/FontName") + 1;
  while (*fontnameptr == ' ' || *fontnameptr == '/') fontnameptr++;
  int l=0;
  while (fontnameptr[l] != ' ') l++;
  
  psname = QString::fromLatin1(fontnameptr,l);
  replacementList = makePSFontNameList( f, psname );
}

void QPSPrinterFontPFB::download(QTextStream& s)
{
  unsigned char* p = (unsigned char*) data.data();
  int pos;
  int len;
  int typ;
  
  int hexcol = 0;
  int line_length = 64;

  emitPSFontNameList( s, psname, replacementList );
  s << "% Font resource\n";

  pos = 0;
  typ = -1;
  while (typ != 3) { // not end of file 
    if (p[ pos ] != 0x80) // PFB marker
      return; // pfb file does not start with 0x80
    pos++;
    typ = p[ pos ]; // 1=ascii 2=binary 3=done
    pos++;
  
    if (typ == 3) break;

    len = p[ pos ];	    pos++;
    len |= (p[ pos ] << 8) ; pos++;
    len |= (p[ pos ] << 16); pos++;
    len |= (p[ pos ] << 24); pos++;
    
    //printf("font block type %d len %d\n",typ,len);
    
    if (typ==1) {
      for (int j=0; j<len; j++) {
	if (hexcol) { s << "\n"; hexcol = 0; }
	//qWarning(QString::fromLatin1((char*)(p+pos),1));
	if (p[pos] == '\r' || p[pos] == '\n') {
	  s << "\n";
        while (p[pos] == '\r' || p[pos] == '\n') pos++;
	} else {
	s << QString::fromLatin1((char*)(p+pos),1);
	pos++;
	}
      }
    }
    if (typ==2) {
      static char *hexchar = "0123456789abcdef";
      for (int j=0; j<len; j++) {
	/* trim hexadecimal lines to line_length columns */
	if (hexcol >= line_length) {
	  s << "\n";
	  hexcol = 0;
	}
	s << QString::fromLatin1(hexchar+((p[pos] >> 4) & 0xf),1);
	s << QString::fromLatin1(hexchar+((p[pos]     ) & 0xf),1);
	pos++;
	//putc(hexchar[(*p >> 4) & 0xf], ofp);
	//putc(hexchar[*p & 0xf], ofp);
	hexcol += 2;
      }

      //pos += len;
    }
  }
  s << "% End of font resource\n";
}

// ================== AFontFileNotFound ============



class QPSPrinterFontNotFound
  : public QPSPrinterFontPrivate {
public:
  QPSPrinterFontNotFound(const QFont& f);
  virtual void    download(QTextStream& s);
private:
  QByteArray     data;
};

QPSPrinterFontNotFound::QPSPrinterFontNotFound(const QFont& f)
{
    psname = makePSFontName( f );
    replacementList = makePSFontNameList( f );
}

void QPSPrinterFontNotFound::download(QTextStream& s)
{
    emitPSFontNameList( s, psname, replacementList );
  s << "% No embeddable font for ";
  s << psname;
  s << " found\n";
}

// =================== A font file for asian ============

class QPSPrinterFontAsian
  : public QPSPrinterFontPrivate {
public:
      void download(QTextStream& s);
      QString defineFont( QTextStream &stream, QString ps, const QFont &f, int cs, const QString &key, 
			  QPSPrinterPrivate *d ) = 0;
      QString emitDef( QTextStream &stream, const QString &ps, const QString &latinName, 
		    const QFont &f, float slant, const QString &key, const QString &key2, QPSPrinterPrivate *d );
};

void QPSPrinterFontAsian::download(QTextStream& s)
{
    emitPSFontNameList( s, psname, replacementList );
  s << "% Asian postscript font requested. Using ";
  s << psname << endl;
}


QString QPSPrinterFontAsian::emitDef( QTextStream &stream, const QString &ps, const QString &latinName, 
				   const QFont &f, float slant, const QString &key, const QString &key2, QPSPrinterPrivate *d)
{
    QString fontName;
    QString lfontName;

    QString *tmp = d->headerFontNames.find( key );

    if ( d->buffer ) {
	if ( tmp ) {
	    fontName = *tmp;
	} else {
	    lfontName.sprintf( "/F%d ", ++d->headerFontNumber );
	    d->fontStream << lfontName << " " << slant << "/" << latinName << " MSF\n";
	    d->headerFontNames.insert( key2, new QString( fontName ) );
	    fontName.sprintf( "/F%das ", d->headerFontNumber );
	    d->fontStream << fontName << slant << ps << " MSF\n";
	    d->headerFontNames.insert( key2, new QString( fontName ) );
	}
	++d->headerFontNumber;
	d->fontStream << "/F" << d->headerFontNumber << " "
		      << f.pointSize() << lfontName << " DF\n";
	d->fontStream << "/F" << d->headerFontNumber << "as "
		      << f.pointSize() << fontName << " DF\n";
	fontName.sprintf( "F%d", d->headerFontNumber );
	d->headerFontNames.insert( key, new QString( fontName ) );
    } else {
	if ( !tmp )
	    tmp = d->pageFontNames.find( key );
	if ( tmp ) {
	    fontName = *tmp;
	} else {
	    lfontName.sprintf( "/F%d ", ++d->pageFontNumber );
	    stream << lfontName << " " << slant << "/" << latinName << " MSF\n";
	    d->pageFontNames.insert( key2, new QString( fontName ) );
	    fontName.sprintf( "/F%das ", d->pageFontNumber );
	    stream << fontName << slant << ps << " MSF\n";
	    d->pageFontNames.insert( key2, new QString( fontName ) );
	}
	++d->pageFontNumber;
	stream << "/F" << d->pageFontNumber << " "
	       << f.pointSize() << lfontName << " DF\n";
	stream << "/F" << d->pageFontNumber << "as "
	       << f.pointSize() << fontName << " DF\n";
	fontName.sprintf( "F%d", d->pageFontNumber );
	d->pageFontNames.insert( key, new QString( fontName ) );
    }
    return fontName;

}

// ----------- Japanese --------------

class QPSPrinterFontJapanese
  : public QPSPrinterFontAsian {
public:
      QPSPrinterFontJapanese(const QFont& f);
      void drawText( QTextStream &stream, uint spaces, const QPoint &p, 
		     const QString &text, QPSPrinterPrivate *d, QPainter *paint);
      QString defineFont( QTextStream &stream, QString ps, const QFont &f, int cs, const QString &key, 
			  QPSPrinterPrivate *d );
private:
    const QJpUnicodeConv *convJP;
};

QPSPrinterFontJapanese::QPSPrinterFontJapanese(const QFont& f)
{
    psname = makePSFontName( f );
    // now try to convert that to some japanese postscript font in a sensible way.
    // as we don't support font substitution at the moment, we just use the
    // two fonts that should be available on any system: Ryumin-Light and GothicBBB-Medium
    
    if ( psname.contains( "Helvetica" ) || psname.contains( "Bold" ) )
	if ( psname.contains("Italic") || psname.contains("Oblique") ) {
	    psname = "GothicBBB-Oblique";
	} else {
	    psname = "GothicBBB";
	}
    else
	if ( psname.contains("Italic") || psname.contains("Oblique") ) {
	    psname = "Ryumin-Oblique";
	} else {
	    psname = "Ryumin";
	}
    convJP = 0;
    replacementList = makePSFontNameList( f );
}


QString QPSPrinterFontJapanese::defineFont( QTextStream &stream, QString ps, const QFont &f, int cs, const QString &key, 
				       QPSPrinterPrivate *d )
{
    QString key2;
    key2.sprintf( "%s %d", ps.ascii(), cs );

    float slant = 0;
    QString latinName;
    // do correct mapping
    if ( ps == "GothicBBB-Oblique" ) {
	slant = 0.2;
	ps = "/GothicBBB-Medium-H";
	latinName = "Helvetica-Oblique";
    } else if ( ps == "GothicBBB" ) {
	ps = "/GothicBBB-Medium-H";
	latinName = "Helvetica";
    } else if ( ps == "Ryumin-Oblique" ) {
	slant = 0.2;
	ps = "/Ryumin-Light-H";
	latinName = "Times-Italic";
    } else if ( ps == "Ryumin" ) {
	ps = "/Ryumin-Light-H";
	latinName = "Times-Roman";
    }
    return emitDef( stream, ps, latinName, f, slant, key, key2, d );
}

void QPSPrinterFontJapanese::drawText( QTextStream &stream, uint spaces, const QPoint &p, 
				  const QString &text, QPSPrinterPrivate *d, QPainter *paint)
{
    if ( !convJP )
	convJP = QJpUnicodeConv::newConverter(JU_Default);

    int x = p.x();
    if ( spaces > 0 )
	x += spaces * d->fm->width( ' ' );
    int y = p.y();
    if ( y != d->textY || d->textY == 0 )
	stream << y << " Y";
    d->textY = y;
    QString mdf;
    if ( paint->font().underline() )
	mdf += " " + QString().setNum( y + d->fm->underlinePos() ) + " Tl";
    if ( paint->font().strikeOut() )
	mdf += " " + QString().setNum( y + d->fm->strikeOutPos() ) + " Tl";
    int code = 0, codeOld = 0;
    QChar ch;
    QCString out, oneChar;
    int l = text.length();
    for ( int i = 0; i <= l; i++ ) {
	out += oneChar;
	oneChar = "";
	codeOld = code;
	if ( i < l ) {
	    ch = text.at(i);
	    if ( !ch.row() ) {
		code = 0;
		if ( ch == '(' || ch == ')' || ch == '\\' )
		    oneChar += "\\";
		oneChar += ch.cell();
	    } else {
		code = 1;
		QChar ch = convJP->UnicodeToJisx0208( ch.unicode());
		if ( ch.isNull() ) {
		    oneChar += 0x22;
		    oneChar += 0x22; // open box
		} else {
		    char chj = ch.row();
		    if ( chj == '(' || chj == ')' || chj == '\\' )
			oneChar += "\\";
		    oneChar += chj;
		    chj = ch.cell();
		    if ( chj == '(' || chj == ')' || chj == '\\' )
			oneChar += "\\";
		    oneChar += chj;
		}
	    }
	}
	if ( !out.isEmpty() && (code != codeOld || i == l) ) {
	    if ( codeOld == 0 ) {
		stream << " " << d->currentFont << " F";
	    } else {
		stream << " " << d->currentFont << "as F";
	    }
	    int w = d->fm->width( out );
	    stream << "(" << out << ")" << w << " " << x << mdf << " T";
	    if ( i < l ) {
		stream << " ";
	    } else {
		stream << "\n";
	    }
	    x += w;
	    out = "";
	}
    }
}

// ----------- Korean --------------

// sans serif
static const psfont SMGothic [] = {
    { "SMGothic-Medium-KSC-EUC-H", 0, 100. },
    { "SMGothic-Medium-KSC-EUC-H", 0.2, 100. },
    { "SMGothic-DemiBold-KSC-EUC-H", 0, 100. },
    { "SMGothic-DemiBold-KSC-EUC-H", 0.2, 100. }
};	

// serif
static const psfont SMMyungjo [] = {
    { "SMMyungjo-Light-KSC-EUC-H", 0, 100. },
    { "SMMyungjo-Light-KSC-EUC-H", 0.2, 100. },
    { "SMMyungjo-Bold-KSC-EUC-H", 0, 100. },
    { "SMMyungjo-Bold-KSC-EUC-H", 0.2, 100. }
};	
    
static const psfont MKai [] = {
    { "MingMT-Light-KSC-EUC-H", 0, 100. },
    { "MingMT-Light-KSC-EUC-H", 0.2, 100. },
    { "MKai-Medium-KSC-EUC-H", 0, 100. },
    { "MKai-Medium-KSC-EUC-H", 0.2, 100. },
};	
    

static const psfont Munhwa [] = {
    { "Munhwa-Regular-KSC-EUC-H", 0, 100. },
    { "Munhwa-Regular-KSC-EUC-H", 0.2, 100. },
    { "Munhwa-Bold-KSC-EUC-H", 0, 100. },
    { "Munhwa-Bold-KSC-EUC-H", 0.2, 100. }
};	

static const psfont * const KoreanReplacements[] = {
    SMMyungjo, SMGothic, Munhwa, MKai, Helvetica, 0
	};

class QPSPrinterFontKorean
  : public QPSPrinterFontAsian {
public:
      QPSPrinterFontKorean(const QFont& f);
      QString defineFont( QTextStream &stream, QString ps, const QFont &f, int cs, const QString &key, 
			  QPSPrinterPrivate *d );
};

QPSPrinterFontKorean::QPSPrinterFontKorean(const QFont& f)
{
    int type = getPsFontType( f );
    psname = SMMyungjo[type].psname;
    appendReplacements( replacementList, KoreanReplacements, type );
}


QString QPSPrinterFontKorean::defineFont( QTextStream &stream, QString ps, const QFont &f, int /*cs*/, const QString &key, 
				       QPSPrinterPrivate *d )
{
    return QPSPrinterFontPrivate::defineFont( stream, ps, f, QFont::AnyCharSet, key, d );
}

// ----------- traditional chinese ------------

static const psfont MSung [] = {
    { "MSung-Light-B5-H", 0, 100. },
    { "MSung-Light-B5-H", 0.2, 100. },
    { "MSung-Light-B5-H", 0, 100. },
    { "MSung-Light-B5-H", 0.2, 100. },
};	

static const psfont MOESung [] = {
    { "MOESung-Regular-B5-H", 0, 100. },
    { "MOESung-Regular-B5-H", 0.2, 100. },
    { "MOESung-Regular-B5-H", 0, 100. },
    { "MOESung-Regular-B5-H", 0.2, 100. },
};	

static const psfont * const MSungReplacements[] = {
    MSung, MOESung, Helvetica, 0
	};

class QPSPrinterFontTraditionalChinese
  : public QPSPrinterFontAsian {
public:
      QPSPrinterFontTraditionalChinese(const QFont& f);
      QString defineFont( QTextStream &stream, QString ps, const QFont &f, int cs, const QString &key, 
			  QPSPrinterPrivate *d );
};

QPSPrinterFontTraditionalChinese::QPSPrinterFontTraditionalChinese(const QFont& f)
{
    int type = getPsFontType( f );
    psname = MSung[type].psname;
    appendReplacements( replacementList, MSungReplacements, type );
}


QString QPSPrinterFontTraditionalChinese::defineFont( QTextStream &stream, QString ps, const QFont &f, int /*cs*/, const QString &key, 
				       QPSPrinterPrivate *d )
{
    return QPSPrinterFontPrivate::defineFont( stream, ps, f, QFont::AnyCharSet, key, d );
}

// ----------- simplified chinese ------------



static const psfont Simplified [] = {
    { "MSung-Light-GBK-EUC-H", 0, 100. },
    { "MSung-Light-GBK-EUC-H", 0.2, 100. },
    { "MKai-Medium-GBK-EUC-H", 0, 100. },
    { "MKai-Medium-GBK-EUC-H", 0.2, 100. },
};	

static const psfont MSungGBK [] = {
    { "MSung-Light-GBK-EUC-H", 0, 100. },
    { "MSung-Light-GBK-EUC-H", 0.2, 100. },
    { "MSung-Light-GBK-EUC-H", 0, 100. },
    { "MSung-Light-GBK-EUC-H", 0.2, 100. },
};	

static const psfont FangSong [] = {
    { "CFangSong-Light-GBK-EUC-H", 0, 100. },
    { "CFangSong-Light-GBK-EUC-H", 0.2, 100. },
    { "CFangSong-Light-GBK-EUC-H", 0, 100. },
    { "CFangSong-Light-GBK-EUC-H", 0.2, 100. },
};	

static const psfont * const SimplifiedReplacements[] = {
    Simplified, MSungGBK, FangSong, Helvetica, 0
	};

class QPSPrinterFontSimplifiedChinese
  : public QPSPrinterFontAsian {
public:
      QPSPrinterFontSimplifiedChinese(const QFont& f);
      QString defineFont( QTextStream &stream, QString ps, const QFont &f, int cs, const QString &key, 
			  QPSPrinterPrivate *d );
};

QPSPrinterFontSimplifiedChinese::QPSPrinterFontSimplifiedChinese(const QFont& f)
{
    int type = getPsFontType( f );
    psname = MSungGBK[type].psname;
    appendReplacements( replacementList, SimplifiedReplacements, type );
}


QString QPSPrinterFontSimplifiedChinese::defineFont( QTextStream &stream, QString ps, const QFont &f, int /*cs*/, const QString &key, 
				       QPSPrinterPrivate *d )
{
    return QPSPrinterFontPrivate::defineFont( stream, ps, f, QFont::AnyCharSet, key, d );
}

// ================== QPSPrinterFont ====================

class QPSPrinterFont {
public:
  QPSPrinterFont(const QFont& f, QPSPrinterPrivate *priv);
  ~QPSPrinterFont();
  QString postScriptFontName()     { return p->postScriptFontName(); }
    QString defineFont( QTextStream &stream, QString ps, const QFont &f, int cs, const QString &key, 
			     QPSPrinterPrivate *d )
    { return p->defineFont( stream, ps, f, cs, key, d ); }
    void    download(QTextStream& s) { p->download(s); }
    QPSPrinterFontPrivate *handle() { return p; }
private:
  QByteArray       data;
  QPSPrinterFontPrivate* p;
};

QPSPrinterFont::~QPSPrinterFont() 
{
    // the dict in QFontPrivate does deletion for us.
    //  delete p;
}
  

QPSPrinterFont::QPSPrinterFont(const QFont& f, QPSPrinterPrivate *priv)
    : p(0)
{
    QString fontfilename;
    QString xfontname;
    QString fontname;

    // ### implement similar code for QWS and WIN
#ifdef _WS_X11_
    int npaths;

    bool xlfd = FALSE;
    
    int index = f.rawName().find('-');
    if (index == 0) { 
	// this is an XLFD font name 
	for (int i=0; i < 6; i++) {
	    index = f.rawName().find('-',index+1);
	}
	xfontname = f.rawName().mid(0,index);
	xlfd = TRUE;
    } else
#endif
	xfontname = makePSFontName( f );

    // ### somehow the font dict doesn't seem to work without this. Don't ask me why...
    priv->fonts.size();
    
    p = priv->fonts.find(xfontname);
    if ( !p ) {
	QFont::CharSet cs = f.charSet();
	// ### the next four lines are a hack to not produce invalid postscript with unicode
	// fonts. As we currently can't deal with Unicode fonts in Postscript, we convert
	// to the most common text and assume the current locale. If that fails we use latin1.
	if ( cs == QFont::Unicode )
	    cs = QFont::charSetForLocale();
	if ( cs == QFont::Unicode )
	    cs = QFont::ISO_8859_1;
	if ( cs == QFont::Set_Ja || cs == QFont::JIS_X_0208) {
	    p = new QPSPrinterFontJapanese( f );
	} else if ( cs == QFont::Set_Ko || cs == QFont::KSC_5601) {
	    p = new QPSPrinterFontKorean( f );
	} else if ( cs == QFont::Set_Big5 || cs == QFont::Big5 || cs == QFont::Set_Zh_TW) {
	    p = new QPSPrinterFontTraditionalChinese( f );
	} else if ( cs == QFont::Set_GBK || cs == QFont::GB_2312 || cs == QFont::Set_Zh) {
	    p = new QPSPrinterFontSimplifiedChinese( f );
	} else {
	    //qDebug("didnt find font for %s", xfontname.latin1());
#ifdef _WS_X11_
#ifdef QT_XFT
	    if ( qt_use_xft() ) {
		XftFont *font = (XftFont *) qt_ft_font( &f );
		char *filename;
		XftPatternGetString (font->pattern, XFT_FILE, 0, &filename);
		//qDebug("filename for font is '%s'", filename);
		if ( filename )
		    fontfilename = QString::fromLatin1( filename );
	    } else
#endif
	    if ( xlfd ) {
		char** font_path; 
		font_path = XGetFontPath( qt_xdisplay(), &npaths);
		for (int i=0; i<npaths && fontfilename.length()==0; i++) {
		    if ((font_path[i])[0] != '/') continue; // not a path name, a font server
		    QString fontmapname;
		    int num = 0;
		    // search font.dir and font.scale for the right file
		    while ( num < 2 ) {
			if ( num == 0 )
			    fontmapname.sprintf("%s%s",font_path[i],"/fonts.scale");
			else
			    fontmapname.sprintf("%s%s",font_path[i],"/fonts.dir");
			//qWarning(fontmapname);
			QFile fontmap(fontmapname);
			if (fontmap.open(IO_ReadOnly)) {
			    while (!fontmap.atEnd()) {
				QString mapping;
				fontmap.readLine(mapping,256);
				// fold to lower (since X folds to lowercase)
				//qWarning(xfontname);
				//qWarning(mapping);
				if (mapping.lower().contains(xfontname.lower())) {
				    index = mapping.find(' ',0);
				    QString ffn = mapping.mid(0,index);
				// remove the most common bitmap formats
				    if( !ffn.contains( ".pcf" ) && !ffn.contains( " .bdf" ) &&
					!ffn.contains( ".spd" ) && !ffn.contains( ".phont" ) ) {
					fontfilename = font_path[i] + QString("/") + ffn;
					if ( QFile::exists(fontfilename) ) {
					    //qDebug("found font file %s", fontfilename.latin1());
					    break;
					} else // unset fontfilename
					    fontfilename = QString();
				    }
				}
			    }
			    fontmap.close(); 
			}
			num++;
		    }
		}
		XFreeFontPath(font_path);
	    }

	    // memory mapping would be better here
	    if (fontfilename.length() > 0) { // maybe there is no file name
		QFile fontfile(fontfilename);
		if ( fontfile.exists() ) {
		    //printf("font name %s size = %d\n",fontfilename.latin1(),fontfile.size());
		    data = QByteArray( fontfile.size() );
      
		    fontfile.open(IO_Raw | IO_ReadOnly);
		    fontfile.readBlock(data.data(), fontfile.size());
		    fontfile.close(); 
		} 		    
	    }
  

	    enum { NONE, PFB, PFA, TTF } type = NONE;

	    if (!data.isNull() && data.size() > 0) {
		unsigned char* d = (unsigned char *)data.data();
		if (d[0] == 0x80 && d[6] == '%' && d[7] == '!' && d[8] == 'P' && d[9] == 'S' )                
		    type = PFB;
		else if (d[0] == '%' && d[1] == '!' && d[2] == 'P' && d[3] == 'S')  
		    type = PFA;
		else if (d[0]==0x00 && d[1]==0x01 && d[2]==0x00 && d[3]==0x00) 
		    type = TTF;
		else 
		    type = NONE;
	    } else
		type = NONE;

	    //qWarning(xfontname);
	    switch (type) {
		case TTF : 
		    p = new QPSPrinterFontTTF(f, data);
		    break;
		case PFB:  
		    p = new QPSPrinterFontPFB(f, data);
		    break;
		case PFA:  
		    p = new QPSPrinterFontPFA(f, data);   
		    break;
		case NONE: 
		default:   
		    p=new QPSPrinterFontNotFound( f ); break;
	    }
#else	
	    p = new QPSPrinterFontNotFound( f );
#endif
	}
	//qDebug("inserting %s int dict (%p)", xfontname.latin1(), p);
	priv->fonts.insert( xfontname, p );
    } 
//     else
// 	qDebug("found font file for %s", xfontname.latin1());

    //qDebug("font dict size=%d", priv->fonts.size());
}    

	    
// ================= END OF PS FONT METHODS ============

void QPSPrinter::setFont( const QFont & fnt )
{
    QFont f = fnt;
    if ( f.rawMode() ) {
	QFont fnt( QString::fromLatin1("Helvetica"), 12 );
	setFont( fnt );
	return;
    }
    if ( f.pointSize() == 0 ) {
#if defined(CHECK_RANGE)
	qWarning( "QPrinter: Cannot set a font with zero point size." );
#endif
	f.setPointSize(QApplication::font().pointSize());
	if ( f.pointSize() == 0 )
	    f.setPointSize( 11 );
    }

    if ( !fixed_ps_header )
	makeFixedStrings();

    QPSPrinterFont ff( f, d );
    QString ps = ff.postScriptFontName();

    QString s = ps;
    s.append( ' ' );
    s.prepend( ' ' );
    if ( !fontsUsed.contains( s ) )
      if (d->buffer)
	ff.download(d->fontStream);

    int i;
    QString key;
    int cs = (int)f.charSet();
    if ( cs == QFont::Unicode )
	cs = QFont::charSetForLocale();
    if ( cs == QFont::Unicode )
	cs = QFont::ISO_8859_1;

    key.sprintf( "%s %d %d", ps.ascii(), f.pointSize(), cs );
    QString * tmp;
    tmp = d->headerFontNames.find( key );
    if ( !tmp && !d->buffer )
	tmp = d->pageFontNames.find( key );

    QString fontName;
    if ( tmp )
	fontName = *tmp;

    if ( fontName.isEmpty() ) {
	fontName = ff.defineFont( stream, ps, f, cs, key, d );
    }
    stream << fontName << " F\n";

    ps.append( ' ' );
    ps.prepend( ' ' );
    if ( !fontsUsed.contains( ps ) )
	fontsUsed += ps;

    QTextCodec * codec = 0;
#ifndef QT_NO_TEXTCODEC
    i = 0;
    do {
	if ( unicodevalues[i].cs == f.charSet() )
	    codec = QTextCodec::codecForMib( unicodevalues[i++].mib );
    } while( codec == 0 && unicodevalues[i++].cs != unicodevalues_LAST );
#endif
    d->currentFont = fontName;
    d->currentFontCodec = codec;
    d->currentFontFile = ff.handle();
}


static void ps_r7( QTextStream& stream, const char * s, int l )
{
    int i = 0;
    uchar line[79];
    int col = 0;

    while( i < l ) {
	line[col++] = s[i++];
	if ( col >= 76 ) {
	    line[col++] = '\n';
	    line[col++] = '\0';
	    stream << (const char *)line;
	    col = 0;
	}
    }
    if ( col > 0 ) {
	while( (col&3) != 0 )
	    line[col++] = '%'; // use a comment as padding
	line[col++] = '\n';
	line[col++] = '\0';
	stream << (const char *)line;
    }
}


static const int quoteSize = 3; // 1-8 pixels
static const int maxQuoteLength = 4+16+32+64+128; // magic extended quote
static const int quoteReach = 10; // ... 1-1024 pixels back
static const int tableSize = 1024; // 2 ** quoteReach;

static const int hashSize = 29;

static const int None = INT_MAX;


static void emitBits( QByteArray & out, int & byte, int & bit,
		      int numBits, uint data )
{
    int b = 0;
    uint d = data;
    while( b < numBits ) {
	if ( bit == 0 )
	    out[byte] = 0;
	if ( d & 1 )
	    out[byte] = (uchar)out[byte] | ( 1 << bit );
	d = d >> 1;
	b++;
	bit++;
	if ( bit > 6 ) {
	    bit = 0;
	    byte++;
	    if ( byte == (int)out.size() )
		out.resize( byte*2 );
	}
    }
}


QByteArray compress( const QImage & image ) {
    int size = image.width()*image.height();
    int pastPixel[tableSize];
    int mostRecentPixel[hashSize];
    QRgb *pixel = new QRgb[size+1];

    int i = 0;
    if ( image.depth() == 8 ) {
	for( int y=0; y < image.height(); y++ ) {
	    uchar * s = image.scanLine( y );
	    for( int x=0; x < image.width(); x++ ) {
		pixel[i] = image.color( s[x] );
		if ( qAlpha( pixel[i] )< 0x40 ) // 25% alpha, convert to white
		    pixel[i] = qRgb( 0xff, 0xff, 0xff );
		else
		    pixel[i] &= RGB_MASK;
		i++;
	    }
	}
    } else {
	for( int y=0; y < image.height(); y++ ) {
	    QRgb * s = (QRgb*)(image.scanLine( y ));
	    for( int x=0; x < image.width(); x++ )
		for( x=0; x < image.width(); x++ ) {
		    pixel[i] = (*s++);
		    if ( qAlpha( pixel[i] ) < 0x40 ) // 25% alpha, convert to white -                   
			pixel[i] = qRgb( 0xff, 0xff, 0xff );
		    else
			pixel[i] &= RGB_MASK;
		    i++;
	   }
	}
    }

    pixel[size] = 0;

    /* this compression function emits blocks of data, where each
       block is an unquoted series of pixels, or a quote from earlier
       pixels. if the six-letter string "banana" were a six-pixel
       image, it might be unquoted "ban" followed by a 3-pixel quote
       from -2.  note that the final "a" is then copied from the
       second "a", which is copied from the first "a" in the same copy
       operation.

       the scanning for quotable blocks uses a cobol-like loop and a
       hash table: we know how many pixels we need to quote, hash the
       first and last pixel we need, and then go backwards in time
       looking for some spot where those pixels of those two colours
       occur at the right distance from each other.

       when we find a spot, we'll try a string-compare of all the
       intervening pixels. we only do a maximum of 128 both-ends
       compares or 64 full-string compares. it's more important to be
       fast than get the ultimate in compression. */

    for( i=0; i < hashSize; i++ )
	mostRecentPixel[i] = None;
    int index = 0;
    int emittedUntil = 0;
    QByteArray out( 49 );
    int outOffset = 0;
    int outBit = 0;
    /* we process pixels serially, emitting as necessary/possible. */
    while( index <= size ) {
	int bestCandidate = None;
	int bestLength = 0;
	i = index % tableSize;
	int h = pixel[index] % hashSize;
	int start, end;
	start = end = pastPixel[i] = mostRecentPixel[h];
	mostRecentPixel[h] = index;
	/* if our first candidate quote is unusable, or we don't need
	   to quote because we've already emitted something for this
	   pixel, just skip. */
	if ( start < index - tableSize || index >= size ||
	     emittedUntil > index)
	    start = end = None;
	int attempts = 0;
	/* scan for suitable quote candidates: not too far back, and
	   if we've found one that's as big as it can get, don't look
	   for more */
	while( start != None && end != None &&
	       bestLength < maxQuoteLength &&
	       start >= index - tableSize &&
	       end >= index - tableSize + bestLength ) {
	    /* scan backwards, looking for something good enough to
	       try a (slow) string comparison. we maintain indexes to
	       the start and the end of the quote candidate here */
	    while( start != None && end != None &&
		   ( pixel[start] != pixel[index] ||
		     pixel[end] != pixel[index+bestLength] ) ) {
		if ( attempts++ > 128 ) {
		    start = None;
		} else if ( pixel[end] % hashSize ==
			    pixel[index+bestLength] % hashSize ) {
		    /* we move the area along the end index' chain */
		    end = pastPixel[end%tableSize];
		    start = end - bestLength;
		} else if ( pixel[start] % hashSize ==
			    pixel[index] % hashSize ) {
		    /* ... or along the start index' chain */
		    start = pastPixel[start%tableSize];
		    end = start + bestLength;
		} else {
#if 0
		    /* this should never happen: both the start and
		       the end pointers ran off their tracks. */
		    qDebug( "oops! %06x %06x %06x %06x %5d %5d %5d %d",
			    pixel[start], pixel[end],
			    pixel[index], pixel[index+bestLength],
			    start, end, index, bestLength );
#endif
		    /* but if it should happen, no problem. we'll just
		       say we found nothing, and the compression will
		       be a bit worse. */
		    start = None;
		}
		/* if we've moved either index too far to use the
		   quote candidate, let's just give up here. there's
		   also a guard against "start" insanity. */
		if ( start < index - tableSize || start < 0 || start >= index )
		    start = None;
		if ( end < index - tableSize + bestLength || end < bestLength )
		    end = None;
	    }
	    /* ok, now start and end point to an area of suitable
	       length whose first and last points match, or one/both
	       is/are set to None. */
	    if ( start != None && end != None ) {
		/* slow string compare... */
		int length = 0;
		while( length < maxQuoteLength &&
		       index+length < size &&
		       pixel[start+length] == pixel[index+length] )
		    length++;
		/* if we've found something that overlaps the index
		   point, maybe we can move the quote point back?  if
		   we're copying 10 pixels from 8 pixels back (an
		   overlap of 2), that'll be faster than copying from
		   4 pixels back (an overlap of 6). */
		if ( start + length > index && length > 0 ) {
		    int d = index-start;
		    int equal = TRUE;
		    while( equal && start + length > index &&
			   start > d && start-d >= index-tableSize ) {
			int i = 0;
			while( equal && i < d ) {
			    if( pixel[start+i] != pixel[start+i-d] )
				equal = FALSE;
			    i++;
			}
			if ( equal )
			    start -= d;
		    }
		}
		/* if what we have is longer than the best previous
		   candidate, we'll use this one. */
		if ( length > bestLength ) {
		    attempts = 0;
		    bestCandidate = start;
		    bestLength = length;
		    if ( length < maxQuoteLength && index + length < size )
			end = mostRecentPixel[pixel[index+length]%hashSize];
		} else {
		    /* and if it ins't, we'll try some more. but we'll
		       count each string compare extra, since they're
		       so expensive. */
		    attempts += 2;
		    if ( attempts > 128 ) {
			start = None;
		    } else if ( pastPixel[start%tableSize] + bestLength <
				pastPixel[end%tableSize] ) {
			start = pastPixel[start%tableSize];
			end = start + bestLength;
		    } else {
			end = pastPixel[end%tableSize];
			start = end - bestLength;
		    }
		}
		/* again, if we can't make use of the current quote
		   candidate, we don't try any more */
		if ( start < index - tableSize || start < 0 || start > size+1 )
		    start = None;
		if ( end < index - tableSize + bestLength || end < 0 || end > size+1 )
		    end = None;
	    }
	}
	/* at this point, bestCandidate is a candidate of bestLength
	 length, or else it's None. if we have such a candidate, or
	 we're at the end, we have to emit all unquoted data. */
	if ( index == size || bestCandidate != None ) {
	    /* we need a double loop, because there's a maximum length
	       on the "unquoted data" section. */
	    while( emittedUntil < index ) {
		int l = QMIN( 8, index - emittedUntil );
		emitBits( out, outOffset, outBit,
			  1, 0 );
		emitBits( out, outOffset, outBit,
			  quoteSize, l-1 );
		while( l-- ) {
		    emitBits( out, outOffset, outBit,
			      8, qRed( pixel[emittedUntil] ) );
		    emitBits( out, outOffset, outBit,
			      8, qGreen( pixel[emittedUntil] ) );
		    emitBits( out, outOffset, outBit,
			      8, qBlue( pixel[emittedUntil] ) );
		    emittedUntil++;
		}
	    }
	}
	/* if we have some quoted data to output, do it. */
	if ( bestCandidate != None ) {
	    emitBits( out, outOffset, outBit,
		      1, 1 );
	    if ( bestLength < 5 ) {
		emitBits( out, outOffset, outBit,
			  quoteSize, bestLength - 1 );
	    } else if ( bestLength - 4 <= 16 ) {
		emitBits( out, outOffset, outBit,
			  quoteSize, 4 );
		emitBits( out, outOffset, outBit,
			  4, bestLength - 1 - 4 );
	    } else if ( bestLength - 4 - 16 <= 32 ) {
		emitBits( out, outOffset, outBit,
			  quoteSize, 5 );
		emitBits( out, outOffset, outBit,
			  5, bestLength - 1 - 4 - 16 );
	    } else if ( bestLength - 4 - 16 - 32 <= 64 ) {
		emitBits( out, outOffset, outBit,
			  quoteSize, 6 );
		emitBits( out, outOffset, outBit,
			  6, bestLength - 1 - 4 - 16 - 32 );
	    } else /* if ( bestLength - 4 - 16 - 32 - 64 <= 128 ) */ {
		emitBits( out, outOffset, outBit,
			  quoteSize, 7 );
		emitBits( out, outOffset, outBit,
			  7, bestLength - 1 - 4 - 16 - 32 - 64 );
	    }
	    emitBits( out, outOffset, outBit,
		      quoteReach, index - bestCandidate - 1 );
	    emittedUntil += bestLength;
	}
	index++;
    }
    /* we've output all the data; time to clean up and finish off the
       last characters. */
    if ( outBit )
	outOffset++;
    out.truncate( outOffset );
    i = 0;
    /* we have to make sure the date is encoded in a stylish way :) */
    while( i < outOffset ) {
	uchar c = out[i];
	c += 42;
	if ( c > 'Z' && ( c != 't' || i == 0 || out[i-1] != 'Q' ) )
	    c += 84;
	out[i] = c;
	i++;
    }
    delete [] pixel;
    return out;
}

#undef XCOORD
#undef YCOORD
#undef WIDTH
#undef HEIGHT
#undef POINT
#undef RECT
#undef INT_ARG

#define XCOORD(x)	(float)(x)
#define YCOORD(y)	(float)(y)
#define WIDTH(w)	(float)(w)
#define HEIGHT(h)	(float)(h)

#define POINT(index)	XCOORD(p[index].point->x()) << ' ' <<		\
			YCOORD(p[index].point->y()) << ' '
#define RECT(index)	XCOORD(p[index].rect->normalize().x())  << ' ' <<     \
			YCOORD(p[index].rect->normalize().y())  << ' ' <<     \
			WIDTH (p[index].rect->normalize().width()) << ' ' <<  \
			HEIGHT(p[index].rect->normalize().height()) << ' '
#define INT_ARG(index)	p[index].ival << ' '

static char returnbuffer[13];
static const char * color( const QColor &c, QPrinter * printer )
{
    if ( c == Qt::black )
	qstrcpy( returnbuffer, "B " );
    else if ( c == Qt::white )
	qstrcpy( returnbuffer, "W " );
    else if ( c.red() == c.green() && c.red() == c.blue() )
	sprintf( returnbuffer, "%d d2 ", c.red() );
    else if ( printer->colorMode() == QPrinter::GrayScale )
	sprintf( returnbuffer, "%d d2 ",
		 qGray( c.red(), c.green(),c.blue() ) );
    else
	sprintf( returnbuffer, "%d %d %d ",
		 c.red(), c.green(), c.blue() );
    return returnbuffer;
}


static const char * psCap( Qt::PenCapStyle p )
{
    if ( p == Qt::SquareCap )
	return "2 ";
    else if ( p == Qt::RoundCap )
	return "1 ";
    return "0 ";
}


static const char * psJoin( Qt::PenJoinStyle p ) {
    if ( p == Qt::BevelJoin )
	return "2 ";
    else if ( p == Qt::RoundJoin )
	return "1 ";
    return "0 ";
}


bool QPSPrinter::cmd( int c , QPainter *paint, QPDevCmdParam *p )
{
    if ( c == PdcBegin ) {		// start painting
	d->pagesInBuffer = 0;
	d->buffer = new QBuffer();
	d->buffer->open( IO_WriteOnly );
	stream.setEncoding( QTextStream::Latin1 );
	stream.setDevice( d->buffer );
	d->fontBuffer = new QBuffer();
	d->fontBuffer->open( IO_WriteOnly );
	d->fontStream.setEncoding( QTextStream::Latin1 );
	d->fontStream.setDevice( d->fontBuffer );
	d->headerFontNumber = 0;
	pageCount           = 1;		// initialize state
	dirtyMatrix         = TRUE;
	d->dirtyClipping    = TRUE;
	dirtyNewPage        = TRUE;
	d->firstClipOnPage  = TRUE;
	d->boundingBox = QRect( 0, 0, -1, -1 );
	fontsUsed = QString::fromLatin1("");

	d->fm = new QFontMetrics( paint->fontMetrics() );

	stream << "%%Page: " << pageCount << " " << pageCount << endl
	       << "%%BeginPageSetup\n"
	       << "QI\n"
	       << "%%EndPageSetup\n";
	return TRUE;
    }

    if ( c == PdcEnd ) {			// painting done
	bool pageCountAtEnd = (d->buffer == 0);
	if ( !pageCountAtEnd )
	    emitHeader( TRUE );
	stream << "QP\n"
	       << "%%Trailer\n";
	if ( pageCountAtEnd )
	    stream << "%%Pages: " << pageCount << "\n%%DocumentFonts: "
		   << fontsUsed.simplifyWhiteSpace() << '\n';
	stream << "%%EOF\n";
	stream.unsetDevice();
	d->realDevice->close();
	if ( d->fd >= 0 )
	    ::close( d->fd );
	d->fd = -1;
	delete d->realDevice;
	d->realDevice = 0;
	delete d->fm;
    }

    if ( c >= PdcDrawFirst && c <= PdcDrawLast ) {
	if ( !paint )
	    return FALSE; // sanity
	if ( dirtyMatrix )
	    matrixSetup( paint );
	if ( dirtyNewPage )
	    newPageSetup( paint );
	if ( d->dirtyClipping )	// Must be after matrixSetup and newPageSetup
	    clippingSetup( paint );
	if ( d->dirtypen ) {
	    // we special-case for narrow solid lines with the default
	    // cap and join styles
	    if ( d->cpen.style() == Qt::SolidLine && d->cpen.width() == 0 &&
		 d->cpen.capStyle() == Qt::FlatCap &&
		 d->cpen.joinStyle() == Qt::MiterJoin )
		stream << color( d->cpen.color(), printer ) << "P1\n";
	    else
		stream << (int)d->cpen.style() << ' ' << d->cpen.width()
		       << ' ' << color( d->cpen.color(), printer )
		       << psCap( d->cpen.capStyle() )
		       << psJoin( d->cpen.joinStyle() ) << "PE\n";
	    d->dirtypen = FALSE;
	}
	if ( d->dirtybrush ) {
	    // we special-case for nobrush and solid white, since
	    // those are the two most common brushes
	    if ( d->cbrush.style() == Qt::NoBrush )
		stream << "NB\n";
	    else if ( d->cbrush.style() == Qt::SolidPattern &&
		      d->cbrush.color() == Qt::white )
		stream << "WB\n";
	    else
		stream << (int)d->cbrush.style() << ' '
		       << color( d->cbrush.color(), printer ) << "BR\n";
	    d->dirtybrush = FALSE;
	}
    }

    switch( c ) {
    case PdcDrawPoint:
	stream << POINT(0) << "P\n";
	break;
    case PdcMoveTo:
	stream << POINT(0) << "M\n";
	break;
    case PdcLineTo:
	stream << POINT(0) << "L\n";
	break;
    case PdcDrawLine:
	if ( p[0].point->y() == p[1].point->y() )
	    stream << POINT(1) << p[0].point->x() << " HL\n";
	else if ( p[0].point->x() == p[1].point->x() )
	    stream << POINT(1) << p[0].point->y() << " VL\n";
	else
	    stream << POINT(1) << POINT(0) << "DL\n";
	break;
    case PdcDrawRect:
	stream << RECT(0) << "R\n";
	break;
    case PdcDrawRoundRect:
	stream << RECT(0) << INT_ARG(1) << INT_ARG(2) << "RR\n";
	break;
    case PdcDrawEllipse:
	stream << RECT(0) << "E\n";
	break;
    case PdcDrawArc:
	stream << RECT(0) << INT_ARG(1) << INT_ARG(2) << "A\n";
	break;
    case PdcDrawPie:
	stream << RECT(0) << INT_ARG(1) << INT_ARG(2) << "PIE\n";
	break;
    case PdcDrawChord:
	stream << RECT(0) << INT_ARG(1) << INT_ARG(2) << "CH\n";
	break;
    case PdcDrawLineSegments:
	if ( p[0].ptarr->size() > 0 ) {
	    QPointArray a = *p[0].ptarr;
	    QPoint pt;
	    stream << "NP\n";
	    for ( int i=0; i<(int)a.size(); i+=2 ) {
		pt = a.point( i );
		stream << XCOORD(pt.x()) << ' '
		       << YCOORD(pt.y()) << " MT\n";
		pt = a.point( i+1 );
		stream << XCOORD(pt.x()) << ' '
		       << YCOORD(pt.y()) << " LT\n";
	    }
	    stream << "QS\n";
	}
	break;
    case PdcDrawPolyline:
	if ( p[0].ptarr->size() > 1 ) {
	    QPointArray a = *p[0].ptarr;
	    QPoint pt = a.point( 0 );
	    stream << "NP\n"
		   << XCOORD(pt.x()) << ' ' << YCOORD(pt.y()) << " MT\n";
	    for ( int i=1; i<(int)a.size(); i++ ) {
		pt = a.point( i );
		stream << XCOORD(pt.x()) << ' '
		       << YCOORD(pt.y()) << " LT\n";
	    }
	    stream << "QS\n";
	}
	break;
    case PdcDrawPolygon:
	if ( p[0].ptarr->size() > 2 ) {
	    QPointArray a = *p[0].ptarr;
	    if ( p[1].ival )
		stream << "/WFi true d\n";
	    QPoint pt = a.point(0);
	    stream << "NP\n";
	    stream << XCOORD(pt.x()) << ' '
		   << YCOORD(pt.y()) << " MT\n";
	    for( int i=1; i<(int)a.size(); i++) {
		pt = a.point( i );
		stream << XCOORD(pt.x()) << ' '
		       << YCOORD(pt.y()) << " LT\n";
	    }
	    stream << "CP BF QS\n";
	    if ( p[1].ival )
		stream << "/WFi false d\n";
	}
	break;
    case PdcDrawQuadBezier:
	if ( p[0].ptarr->size() == 4 ) {
	    stream << "NP\n";
	    QPointArray a = *p[0].ptarr;
	    stream << XCOORD(a[0].x()) << ' '
		   << YCOORD(a[0].y()) << " MT ";
	    for ( int i=1; i<4; i++ ) {
		stream << XCOORD(a[i].x()) << ' '
		       << YCOORD(a[i].y()) << ' ';
	    }
	    stream << "BZ\n";
	}
	break;
    case PdcDrawText2: {
	uint spaces = 0;
	QString tmp = *p[1].str;
	while( spaces < tmp.length() && tmp[(int)spaces] == ' ' )
	    spaces++;
	if ( spaces )
	    tmp = tmp.mid( spaces, tmp.length() );
	while ( tmp.length() > 0 && tmp[(int)tmp.length()-1].isSpace() )
	    tmp.truncate( tmp.length()-1 );
	if( tmp.length() == 0 )
	    break;
	if ( d->currentSet != d->currentUsed || !d->currentFontFile ) {
	    d->currentUsed = d->currentSet;
	    setFont( d->currentSet );
	}
	if( d->currentFontFile ) // better not crash in case somethig goes wrong.
	    d->currentFontFile->drawText( stream, spaces, *p[0].point, tmp, d, paint); 
	break;
    }
    case PdcDrawText2Formatted:
	return FALSE;			// uses QPainter instead
    case PdcDrawPixmap: {
	if ( p[1].pixmap->isNull() )
	    break;
	QPoint pnt = *(p[0].point);
	QImage img;
	img = *(p[1].pixmap);
	drawImage( paint, pnt, img );
	break;
    }
    case PdcDrawImage: {
	if ( p[1].image->isNull() )
	    break;
	QPoint pnt = *(p[0].point);
	QImage img = *(p[1].image);
	drawImage( paint, pnt, img );
	break;
    }
    case PdcSetBkColor:
	stream << color( *(p[0].color), printer ) << "BC\n";
	break;
    case PdcSetBkMode:
	if ( p[0].ival == Qt::TransparentMode )
	    stream << "/OMo false d\n";
	else
	    stream << "/OMo true d\n";
	break;
    case PdcSetROP:
#if defined(CHECK_RANGE)
	if ( p[0].ival != Qt::CopyROP )
	    qWarning( "QPrinter: Raster operation setting not supported" );
#endif
	break;
    case PdcSetBrushOrigin:
	break;
    case PdcSetFont:
	d->currentSet = *(p[0].font);
	// turn these off - they confuse the 'avoid font change' logic
	d->currentSet.setUnderline( FALSE );
	d->currentSet.setStrikeOut( FALSE );
	break;
    case PdcSetPen:
	if ( d->cpen != *(p[0].pen) ) {
	    d->dirtypen = TRUE;
	    d->cpen = *(p[0].pen);
	}
	break;
    case PdcSetBrush:
	if ( p[0].brush->style() == Qt::CustomPattern ) {
#if defined(CHECK_RANGE)
	    qWarning( "QPrinter: Pixmap brush not supported" );
#endif
	    return FALSE;
	}
	if ( d->cbrush != *(p[0].brush) ) {
	    d->dirtybrush = TRUE;
	    d->cbrush = *(p[0].brush);
	}
	break;
    case PdcSetTabStops:
    case PdcSetTabArray:
	return FALSE;
    case PdcSetUnit:
	break;
    case PdcSetVXform:
    case PdcSetWindow:
    case PdcSetViewport:
    case PdcSetWXform:
    case PdcSetWMatrix:
    case PdcRestoreWMatrix:
	dirtyMatrix = TRUE;
	break;
    case PdcSetClip:
	d->dirtyClipping = TRUE;
	break;
    case PdcSetClipRegion:
	d->dirtyClipping = TRUE;
	break;
    case NewPage:
	pageCount++;
	stream << "QP\n%%Page: "
	       << pageCount << ' ' << pageCount << endl
	       << "%%BeginPageSetup\n"
	       << "QI\n"
	       << "%%EndPageSetup\n";
	dirtyNewPage       = TRUE;
	d->dirtyClipping   = TRUE;
	d->firstClipOnPage = TRUE;
	delete d->savedImage;
	d->savedImage = 0;
	d->textY = 0;
	break;
    case AbortPrinting:
	break;
    default:
	break;
    }
    return TRUE;
}


void QPSPrinter::drawImage( QPainter *paint, const QPoint &pnt,
			    const QImage &img )
{
    int width  = img.width();
    int height = img.height();

    if ( img.isNull() )
	return;

    if ( width * height > 21830 ) { // 65535/3, tolerance for broken printers
	int images, subheight;
	images = ( width * height + 21829 ) / 21830;
	subheight = ( height + images-1 ) / images;
	while ( subheight * width > 21830 ) {
	    images++;
	    subheight = ( height + images-1 ) / images;
	}
	int y = 0;
	while( y < height ) {
	    drawImage( paint, QPoint( pnt.x(), pnt.y()+y ),
		       img.copy( 0, y, width, QMIN( subheight, height-y ) ) );
	    y += subheight;
	}
    } else {
	if ( pnt.x() || pnt.y() )
	    stream << pnt.x() << " " << pnt.y() << " TR\n";
	stream << "/sl " << width*3*height << " string d\n";
	stream << "sl rC\n";
	QByteArray out;
	if ( img.depth() < 8 )
	    out = compress( img.convertDepth( 8 ) );
	else if ( img.depth() > 8 && img.depth() < 24 )
	    out = compress( img.convertDepth( 24 ) );
	else
	    out = compress( img );
	ps_r7( stream, out, out.size() );
	stream << "pop\n";
	stream << width << ' ' << height << " 8[1 0 0 1 0 0]{sl}QCI\n";
	if ( pnt.x() || pnt.y() )
	    stream << -pnt.x() << " " << -pnt.y() << " TR\n";
    }
}


void QPSPrinter::matrixSetup( QPainter *paint )
{
#ifndef QT_NO_TRANSFORMATIONS
    QWMatrix tmp;
    if ( paint->hasViewXForm() ) {
	QRect viewport = paint->viewport();
	QRect window   = paint->window();
	tmp.translate( viewport.x(), viewport.y() );
	tmp.scale( 1.0 * viewport.width()  / window.width(),
		   1.0 * viewport.height() / window.height() );
	tmp.translate( -window.x(), -window.y() );
    }
    if ( paint->hasWorldXForm() ) {
	tmp = paint->worldMatrix() * tmp;
    }
    stream << "["
	   << tmp.m11() << ' ' << tmp.m12() << ' '
	   << tmp.m21() << ' ' << tmp.m22() << ' '
	   << tmp.dx()	<< ' ' << tmp.dy()
	   << "]ST\n";
#else
    QPoint p(0,0);
    p = paint->xForm(p);
    stream << "["
	   << 0 << ' ' << 0 << ' '
	   << 0 << ' ' << 0 << ' '
	   << p.x()    << ' ' << p.y()
	   << "]ST\n";
#endif
    dirtyMatrix = FALSE;
}

void QPSPrinter::orientationSetup()
{
    if ( printer->orientation() == QPrinter::Landscape )
	stream << "QLS\n";
}



static QString stripHeader( const QString & header, const char * data,
			    int len, bool useFonts )
{
    // first pass: find and mark all identifiers
    QDict<int> ids( 257 );
    ids.setAutoDelete( TRUE );

    int i=0;
    int size = header.length();
    int * used = new int[size];
    for( i=0; i<size; i++ )
	used[i] = 0;
    i=0;
    used[0] = 0x10000000;
    while( i < size ) {
	while( i < size && header[i] != '/' )
	    i++;
	if ( header[i] == '/' && isalpha( header[i+1].latin1() ) ) {
	    i++;
	    int j=i;
	    while( j < size && isalnum( header[j].latin1() ) )
		j++;
	    char id[10];
	    strncpy( id, header.ascii() + i, j-i );
	    id[j-i] = '\0';
	    while( j < size && isspace( header[j].latin1() ) )
		j++;
	    if ( header[j] == '{' ) {
		j++;
		int k, l;
		l = 1;
		while( l && j < size ) {
		    while( j < size && !isalpha( header[j].latin1() ) ) {
			if ( header[j] == '{' )
			    l++;
			else if ( header[j] == '}' )
			    l--;
			j++;
		    }
		    if ( l ) {
			k = j;
			while( j < size && isalnum( header[j].latin1() ) )
			    j++;
			if( j - k < 10 ) {
			    char id2[10];
			    strncpy( id2, header.ascii() + k, j-k );
			    id2[j-k] = '\0';
			    int * offs = ids.find( id2 );
			    if ( offs )
				used[k] = *offs;
			}
		    }
		}
	    } else if ( header[j] == '[' ) {
		// handle array defintions
		int l=0;
		do {
		    if ( header[j] == '[' )
			l++;
		    else if ( header[j] == ']' )
			l--;
		    j++;
		} while( j < size && l );
	    } else if ( header[j] == 'D' && header[j+1] == '0' &&
			!isalnum( header[j+2].latin1() ) ) {
		ids.insert( id, new int(i-1) );
		used[i-1] = 0x20000000;
		int *tmp = ids.find( "D0" );
		if ( tmp )
		    used[j] = *tmp;
		j = j+2;
	    } else if ( header[j] == 'E' && header[j+1] == 'D' &&
			!isalnum( header[j+2].latin1() ) ) {
		ids.insert( id, new int(i-1) );
		used[i-1] = 0x20000000;
		int *tmp = ids.find( "ED" );
		if ( tmp )
		    used[j] = *tmp;
		j = j+2;
	    } else {
		// handle other variables.
		while( j < size && !isspace( header[j].latin1() ) )
		    j++;
		while( j < size && isspace( header[j].latin1() ) )
		    j++;
		// hack: then skip dict
		if ( !qstrncmp( header.ascii()+j, "dict", 4 ) &&
		     !isalnum( header[j+4].latin1() ) )
		    j += 4;
		// worse: string
		if ( !qstrncmp( header.ascii()+j, "string", 6 ) &&
		     !isalnum( header[j+6].latin1() ) )
		    j += 6;
	    }
	    while( j < size && isspace( header[j].latin1() ) )
		j++;
	    if ( header[j] == 'D' && !isalnum( header[j+1].latin1() ) ) {
		ids.insert( id, new int(i-1) );
		used[i-1] = 0x20000000;
		i = j+1;
	    } else if ( !qstrncmp( header.ascii()+j, "d", 1 ) &&
			!isalnum( header[j+1].latin1() ) ) {
		ids.insert( id, new int(i-1) );
		used[i-1] = 0x20000000;
		i = j+1;
	    } else {
		i = j;
	    }
	} else {
	    i++;
	}
    }

    // second pass: mark the identifiers used in the document

    // we know CM, QI and QP are used
    int * tmp;
    if ( (tmp = ids.take("CM")) != 0 ) {
	used[*tmp] = 0x10000000;
	delete tmp;
    }
    if ( (tmp = ids.take("QI")) != 0 ) {
	used[*tmp] = 0x10000000;
	delete tmp;
    }
    if ( (tmp = ids.take("QP")) != 0 ) {
	used[*tmp] = 0x10000000;
	delete tmp;
    }

    // we speed this up a little by hand-hacking DF/MF/ND support
    if ( useFonts ) {
	if ( (tmp = ids.take("DF")) != 0 ) {
	    used[*tmp] = 0x10000000;
	    delete tmp;
	}
	if ( (tmp = ids.take("MF")) != 0 ) {
	    used[*tmp] = 0x10000000;
	    delete tmp;
	}
	if ( (tmp = ids.take("MSF")) != 0 ) {
	    used[*tmp] = 0x10000000;
	    delete tmp;
	}
	if ( (tmp = ids.take("ND")) != 0 ) {
	    used[*tmp] = 0x10000000;
	    delete tmp;
	}
	if ( (tmp = ids.take("qtfindfont")) != 0 ) {
	    used[*tmp] = 0x10000000;
	    delete tmp;
	}
	if ( (tmp = ids.take("qtdefinefont")) != 0 ) {
	    used[*tmp] = 0x10000000;
	    delete tmp;
	}
    }

    char id[10];
    i = 0;
    while( i < len ) {
	while( i < len && !isalpha( data[i] ) ) {
	    if ( data[i++] == '(' ) { // it's a string, skip it
		int level = 1;
		do {
		    if ( data[i] == '(' )
			level++;
		    else if ( data[i] == ')' )
			level--;
		    else if ( data[i] == '\\' )
			i++;
		    i++;
		} while( i < len && level > 0 );
	    }
	}
	if ( i < len && isalpha( data[i] ) ) {
	    int j=0;
	    while( isalnum( data[i+j] ) )
		j++;
	    if( j < 9 ) {
		// all identifiers we care about are <9 chars long
		strncpy( id, data + i, j );
		id[j] = '\0';
		int * offs = ids.take( id );
		if ( offs )
		    used[*offs] = 0x10000000;
		delete offs;
	    }
	    i += j;
	}
    }

    // third pass: mark the identifiers used in the used parts of the header

    i = size-1;
    while( i > 0 ) {
	// find beginning of function
	int j = i;
	while( j && used[j] < 0x10000000 )
	    j--;
	if ( used[j] == 0x10000000 ) {
	    // this function is used.
	    while( i > j ) {
		if ( used[i] )
		    used[used[i]] = 0x10000000;
		i--;
	    }
	}
	i = j-1;
    }

    // fourth pass: make the new header

    i=0;
    QString result;
    while( i < size ) {
	bool c = used[i] == 0x10000000;
	if ( c && result.length() )
	    result += '\n';
	do {
	    if ( c )
		result += header[i];
	    i++;
	} while ( i < size && used[i] < 0x10000000 );
    }
    delete[] used;
    result = wordwrap( result );
    return result;
}


void QPSPrinter::emitHeader( bool finished )
{
    QString title = printer->docName();
    QString creator = printer->creator();
    if ( !creator )				// default creator
	creator = QString::fromLatin1("Qt " QT_VERSION_STR);
    d->realDevice = new QFile();
    (void)((QFile *)d->realDevice)->open( IO_WriteOnly, d->fd );
    stream.setDevice( d->realDevice );
    stream << "%!PS-Adobe-1.0";
    QPaintDeviceMetrics m( printer );
    if ( finished && pageCount == 1 && printer->numCopies() == 1 &&
	 printer->fullPage() && qt_gen_epsf ) {
	QPaintDeviceMetrics m( printer );
	if ( !d->boundingBox.isValid() )
	    d->boundingBox.setRect( 0, 0, m.width(), m.height() );
	if ( printer->orientation() == QPrinter::Landscape )
	    stream << " EPSF-3.0\n%%BoundingBox: "
		   << m.height() - d->boundingBox.bottom() << " " // llx
		   << m.width() - d->boundingBox.right() << " " // lly
		   << m.height() - d->boundingBox.top() << " " // urx
		   << m.width() - d->boundingBox.left();// ury
	else
	    stream << " EPSF-3.0\n%%BoundingBox: "
		   << d->boundingBox.left() << " "
		   << m.height() - d->boundingBox.bottom() - 1 << " "
		   << d->boundingBox.right() + 1 << " "
		   << m.height() - d->boundingBox.top();
    } else {
	int w = m.width();
	int h = m.height();
	if ( !printer->fullPage() ) {
	    w += 2*printer->margins().width();
	    h += 2*printer->margins().height();
	}
	// set a bounding box according to the DSC
	if ( printer->orientation() == QPrinter::Landscape )	      
	    stream << "\n%%BoundingBox: 0 0 " << h << " " << w;
	else
	    stream << "\n%%BoundingBox: 0 0 " << w << " " << h;
    }
    stream << "\n%%Creator: " << creator;
    if ( !!title )
	stream << "\n%%Title: " << title;
    stream << "\n%%CreationDate: " << QDateTime::currentDateTime().toString();
    stream << "\n%%Orientation: ";
    if ( printer->orientation() == QPrinter::Landscape )
	stream << "Landscape";
    else
	stream << "Portrait";
    if ( finished )
	stream << "\n%%Pages: " << pageCount << "\n%%DocumentFonts: "
	       << fontsUsed.simplifyWhiteSpace();
    else
	stream << "\n%%Pages: (atend)"
	       << "\n%%DocumentFonts: (atend)";
    stream << "\n%%EndComments\n";

    if ( printer->numCopies() > 1 )
	stream << "/#copies " << printer->numCopies() << " def\n";

    if ( !fixed_ps_header )
	makeFixedStrings();

    const char * prologLicense = "% Prolog copyright 1994-2000 Trolltech. "
				 "You may copy this prolog in any way\n"
				 "% that is directly related to this "
				 "document. For other use of this prolog,\n"
				 "% see your licensing agreement for Qt.\n";

    stream << "%%BeginProlog\n";
    if ( finished ) {
	QString r( stripHeader( *fixed_ps_header,
				d->buffer->buffer().data(),
				d->buffer->buffer().size(),
				d->fontBuffer->buffer().size() > 0 ) );
	stream << prologLicense << r << "\n";
    } else {
	stream << prologLicense << *fixed_ps_header << "\n";
    }
    stream << "%%EndProlog\n"
	   << "%%BeginSetup\n";

    stream << "/pageinit {\n";
    if ( !printer->fullPage() ) {
	stream << "% lazy-margin hack: QPrinter::setFullPage(FALSE)\n";
	if ( printer->orientation() == QPrinter::Portrait )
	    stream << printer->margins().width() << " "
		   << printer->margins().height() << " translate\n";
	else
	    stream << printer->margins().height() << " " 
		   << printer->margins().width() << " translate\n";
    }
    if ( printer->orientation() == QPrinter::Portrait ) {
	QPaintDeviceMetrics m( printer );
	stream << "% " << m.widthMM() << "*" << m.heightMM()
	       << "mm (portrait)\n0 " << m.height()
	       << " translate 1 -1 scale/defM matrix CM d } d\n";
    } else {
	QPaintDeviceMetrics m( printer );
	stream << "% " << m.heightMM() << "*" << m.widthMM()
	       << " mm (landscape)\n 90 rotate 1 -1 scale/defM matrix CM d } d\n";
    }

    if ( d->fontBuffer->buffer().size() ) {
	if ( pageCount == 1 || finished )
	    stream << "% Fonts and encodings used\n";
	else
	    stream << "% Fonts and encodings used on pages 1-"
		   << pageCount << "\n";
	stream.writeRawBytes( d->fontBuffer->buffer().data(),
			      d->fontBuffer->buffer().size() );
    }
    stream << "%%EndSetup\n";
    stream.writeRawBytes( d->buffer->buffer().data(),
			  d->buffer->buffer().size() );

    delete d->buffer;
    d->buffer = 0;
    d->fontStream.unsetDevice();
    delete d->fontBuffer;
    d->fontBuffer = 0;
}


void QPSPrinter::newPageSetup( QPainter *paint )
{
    if ( d->buffer &&
	 ( d->pagesInBuffer++ > 32 ||
	   ( d->pagesInBuffer > 4 && d->buffer->size() > 262144 ) ) )
	emitHeader( FALSE );

    if ( !d->buffer ) {
	d->pageEncodings.clear();
	d->pageFontNames.clear();
    }

    resetDrawingTools( paint );
    dirtyNewPage      = FALSE;
    d->pageFontNumber = d->headerFontNumber;
}


/* Called whenever a restore has been done. Currently done at the top of a
  new page and whenever clipping is turned off. */
void QPSPrinter::resetDrawingTools( QPainter *paint )
{
    QPDevCmdParam param[1];
    QPen   defaultPen;			// default drawing tools
    QBrush defaultBrush;

    param[0].color = &paint->backgroundColor();
    if ( *param[0].color != Qt::white )
	cmd( PdcSetBkColor, paint, param );

    param[0].ival = paint->backgroundMode();
    if (param[0].ival != Qt::TransparentMode )
	cmd( PdcSetBkMode, paint, param );

    //d->currentUsed = d->currentSet;
    //setFont( d->currentSet );
    d->currentFontFile = 0;
    
    param[0].brush = &paint->brush();
    if (*param[0].brush != defaultBrush )
	cmd( PdcSetBrush, paint, param);

    d->dirtypen = TRUE;
    d->dirtybrush = TRUE;

    if ( paint->hasViewXForm() || paint->hasWorldXForm() )
	matrixSetup( paint );
}


static void putRect( QTextStream &stream, const QRect &r )
{
    stream << r.x() << " "
	   << r.y() << " "
	   << r.width() << " "
	   << r.height() << " ";
}


void QPSPrinter::setClippingOff( QPainter *paint )
{
	stream << "CLO\n";		// clipping off, includes a restore
	resetDrawingTools( paint );     // so drawing tools must be reset
}


void QPSPrinter::clippingSetup( QPainter *paint )
{
    if ( paint->hasClipping() ) {
	if ( !d->firstClipOnPage )
	    setClippingOff( paint );
	const QRegion rgn = paint->clipRegion();
	QArray<QRect> rects = rgn.rects();
	int i;
	stream<< "CLSTART\n";		// start clipping
	for( i = 0 ; i < (int)rects.size() ; i++ ) {
	    putRect( stream, rects[i] );
	    stream << "ACR\n";		// add clip rect
	    if ( pageCount == 1 )
		d->boundingBox = d->boundingBox.unite( rects[i] );
	}
	stream << "CLEND\n";		// end clipping
	d->firstClipOnPage = FALSE;
    } else {
	if ( !d->firstClipOnPage )	// no need to turn off if first on page
	    setClippingOff( paint );
	// if we're painting without clipping, the bounding box must
	// be everything.  NOTE: this assumes that this function is
	// only ever called when something is to be painted.
	QPaintDeviceMetrics m( printer );
	if ( !d->boundingBox.isValid() )
	    d->boundingBox.setRect( 0, 0, m.width(), m.height() );
    }
    d->dirtyClipping = FALSE;
}


#endif

