/*
 * This file is part of the DOM implementation for KDE.
 *
 * (C) 1999 Lars Knoll (knoll@kde.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * $Id$
 */
#ifndef _DOM_dtd_h_
#define _DOM_dtd_h_

#include "dom_string.h"

namespace DOM
{

void addForbidden(int tagId, ushort *forbiddenTags);
void removeForbidden(int tagId, ushort *forbiddenTags);

enum tagStatus { OPTIONAL, REQUIRED, FORBIDDEN };

bool checkChild(ushort tagID, ushort childID);

DOMString findDefAttrNone(ushort);
// (TT|I|B|U|S|STRIKE|BIG|SMALL|EM|STRONG|DFN|CODE|SAMP|KBD|VAR|CITE|ABBR|ACRONYM)
#define findDefAttrTT findDefAttrNone
// (SUB|SUP)
#define findDefAttrSUB findDefAttrNone
#define findDefAttrSPAN findDefAttrNone
#define findDefAttrBDO findDefAttrNone
#define findDefAttrBASEFONT findDefAttrNone
#define findDefAttrFONT findDefAttrNone
DOMString findDefAttrBR(ushort id);
#define findDefAttrBODY findDefAttrNone
#define findDefAttrADDRESS findDefAttrNone
#define findDefAttrDIV findDefAttrNone
#define findDefAttrCENTER findDefAttrNone
DOMString findDefAttrA(ushort id);
#define findDefAttrMAP findDefAttrNone
DOMString findDefAttrAREA(ushort id);
#define findDefAttrLINK findDefAttrNone
#define findDefAttrIMG findDefAttrNone
#define findDefAttrOBJECT findDefAttrNone
DOMString findDefAttrPARAM(ushort id);
#define findDefAttrAPPLET findDefAttrNone
#define findDefAttrHR findDefAttrNone
#define findDefAttrP findDefAttrNone
// (H1|H2|H3|H4|H5|H6)
#define findDefAttrH1 findDefAttrNone
#define findDefAttrPRE findDefAttrNone
#define findDefAttrQ findDefAttrNone
#define findDefAttrBLOCKQUOTE findDefAttrNone
// (INS|DEL)
#define findDefAttrINS findDefAttrNone
#define findDefAttrDL findDefAttrNone
// (DT|DD)
#define findDefAttrDT findDefAttrNone
#define findDefAttrOL findDefAttrNone
#define findDefAttrUL findDefAttrNone
#define findDefAttrDIR findDefAttrNone
#define findDefAttrMENU findDefAttrNone
#define findDefAttrLI findDefAttrNone
DOMString findDefAttrFORM(ushort id);
#define findDefAttrLABEL findDefAttrNone
DOMString findDefAttrINPUT(ushort id);
#define findDefAttrSELECT findDefAttrNone
#define findDefAttrOPTGROUP findDefAttrNone
#define findDefAttrOPTION findDefAttrNone
#define findDefAttrTEXTAREA findDefAttrNone
#define findDefAttrFIELDSET findDefAttrNone
#define findDefAttrLEGEND findDefAttrNone
DOMString findDefAttrBUTTON(ushort id);
#define findDefAttrTABLE findDefAttrNone
#define findDefAttrCAPTION findDefAttrNone
DOMString findDefAttrCOLGROUP(ushort id);
DOMString findDefAttrCOL(ushort id);
// (THEAD|TBODY|TFOOT)
#define findDefAttrTHEAD findDefAttrNone
#define findDefAttrTR findDefAttrNone
// (TH|TD)
DOMString findDefAttrTH(ushort id);
#define findDefAttrFRAMESET findDefAttrNone
DOMString findDefAttrFRAME(ushort id);
DOMString findDefAttrIFRAME(ushort id);
#define findDefAttrNOFRAMES findDefAttrNone
#define findDefAttrHEAD findDefAttrNone
#define findDefAttrTITLE findDefAttrNone
#define findDefAttrISINDEX findDefAttrNone
#define findDefAttrBASE findDefAttrNone
#define findDefAttrMETA findDefAttrNone
#define findDefAttrSTYLE findDefAttrNone
#define findDefAttrSCRIPT findDefAttrNone
#define findDefAttrNOSCRIPT findDefAttrNone
DOMString findDefAttrHTML(ushort id);

#define IStartTag REQUIRED
#define IEndTag REQUIRED
#define BStartTag REQUIRED
#define BEndTag REQUIRED
#define UStartTag REQUIRED
#define UEndTag REQUIRED
#define SStartTag REQUIRED
#define SEndTag REQUIRED
#define STRIKEStartTag REQUIRED
#define STRIKEEndTag REQUIRED
#define BIGStartTag REQUIRED
#define BIGEndTag REQUIRED
#define SMALLStartTag REQUIRED
#define SMALLEndTag REQUIRED
#define EMStartTag REQUIRED
#define EMEndTag REQUIRED
#define STRONGStartTag REQUIRED
#define STRONGEndTag REQUIRED
#define DFNStartTag REQUIRED
#define DFNEndTag REQUIRED
#define CODEStartTag REQUIRED
#define CODEEndTag REQUIRED
#define SAMPStartTag REQUIRED
#define SAMPEndTag REQUIRED
#define KBDStartTag REQUIRED
#define KBDEndTag REQUIRED
#define VARStartTag REQUIRED
#define VAREndTag REQUIRED
#define CITEStartTag REQUIRED
#define CITEEndTag REQUIRED
#define ABBRStartTag REQUIRED
#define ABBREndTag REQUIRED
#define ACRONYMStartTag REQUIRED
#define ACRONYMEndTag REQUIRED
#define TTStartTag REQUIRED
#define TTEndTag REQUIRED
#define SUBStartTag REQUIRED
#define SUBEndTag REQUIRED
#define SUPStartTag REQUIRED
#define SUPEndTag REQUIRED
#define SPANStartTag REQUIRED
#define SPANEndTag REQUIRED
#define BDOStartTag REQUIRED
#define BDOEndTag REQUIRED
#define BASEFONTStartTag REQUIRED
#define BASEFONTEndTag FORBIDDEN
#define FONTStartTag REQUIRED
#define FONTEndTag REQUIRED
#define BRStartTag REQUIRED
#define BREndTag FORBIDDEN
#define BODYStartTag OPTIONAL
#define BODYEndTag REQUIRED           // is OPTIONAL, but since it's only 
                                      // allowed once in the document, this 
                                      // takes care of having two of them
#define ADDRESSStartTag REQUIRED
#define ADDRESSEndTag REQUIRED
#define DIVStartTag REQUIRED
#define DIVEndTag REQUIRED
#define CENTERStartTag REQUIRED
#define CENTEREndTag REQUIRED
#define AStartTag REQUIRED
#define AEndTag REQUIRED
#define MAPStartTag REQUIRED
#define MAPEndTag REQUIRED
#define AREAStartTag REQUIRED
#define AREAEndTag FORBIDDEN
#define LAYERStartTag REQUIRED
#define LAYEREndTag REQUIRED
#define LINKStartTag REQUIRED
#define LINKEndTag FORBIDDEN
#define IMGStartTag REQUIRED
#define IMGEndTag FORBIDDEN
#define OBJECTStartTag REQUIRED
#define OBJECTEndTag REQUIRED
#define PARAMStartTag REQUIRED
#define PARAMEndTag FORBIDDEN
#define APPLETStartTag REQUIRED
#define APPLETEndTag REQUIRED
#define HRStartTag REQUIRED
#define HREndTag FORBIDDEN
#define PStartTag REQUIRED
#define PEndTag OPTIONAL
#define H1StartTag REQUIRED
#define H1EndTag REQUIRED
#define H2StartTag REQUIRED
#define H2EndTag REQUIRED
#define H3StartTag REQUIRED
#define H3EndTag REQUIRED
#define H4StartTag REQUIRED
#define H4EndTag REQUIRED
#define H5StartTag REQUIRED
#define H5EndTag REQUIRED
#define H6StartTag REQUIRED
#define H6EndTag REQUIRED
#define PREStartTag REQUIRED
#define PREEndTag REQUIRED
#define QStartTag REQUIRED
#define QEndTag REQUIRED
#define BLOCKQUOTEStartTag REQUIRED
#define BLOCKQUOTEEndTag REQUIRED
#define INSStartTag REQUIRED
#define INSEndTag REQUIRED
#define DELStartTag REQUIRED
#define DELEndTag REQUIRED
#define DLStartTag REQUIRED
#define DLEndTag REQUIRED
#define DTStartTag REQUIRED
#define DTEndTag OPTIONAL
#define DDStartTag REQUIRED
#define DDEndTag OPTIONAL
#define OLStartTag REQUIRED
#define OLEndTag REQUIRED
#define ULStartTag REQUIRED
#define ULEndTag REQUIRED
#define MENUStartTag REQUIRED
#define MENUEndTag REQUIRED
#define DIRStartTag REQUIRED
#define DIREndTag REQUIRED
#define LIStartTag REQUIRED
#define LIEndTag OPTIONAL
#define FORMStartTag REQUIRED
#define FORMEndTag REQUIRED
#define LABELStartTag REQUIRED
#define LABELEndTag REQUIRED
#define INPUTStartTag REQUIRED
#define INPUTEndTag FORBIDDEN
#define SELECTStartTag REQUIRED
#define SELECTEndTag REQUIRED
#define OPTGROUPStartTag REQUIRED
#define OPTGROUPEndTag REQUIRED
#define OPTIONStartTag REQUIRED
#define OPTIONEndTag OPTIONAL
#define TEXTAREAStartTag REQUIRED
#define TEXTAREAEndTag REQUIRED
#define FIELDSETStartTag REQUIRED
#define FIELDSETEndTag REQUIRED
#define LEGENDStartTag REQUIRED
#define LEGENDEndTag REQUIRED
#define BUTTONStartTag REQUIRED
#define BUTTONEndTag REQUIRED
#define TABLEStartTag REQUIRED
#define TABLEEndTag REQUIRED
#define CAPTIONStartTag REQUIRED
#define CAPTIONEndTag REQUIRED
#define THEADStartTag REQUIRED
#define THEADEndTag OPTIONAL
#define TFOOTStartTag REQUIRED
#define TFOOTEndTag OPTIONAL
#define TBODYStartTag OPTIONAL
#define TBODYEndTag OPTIONAL
#define COLGROUPStartTag REQUIRED
#define COLGROUPEndTag OPTIONAL
#define COLStartTag REQUIRED
#define COLEndTag FORBIDDEN
#define TRStartTag REQUIRED
#define TREndTag OPTIONAL
#define THStartTag REQUIRED
#define THEndTag OPTIONAL
#define TDStartTag REQUIRED
#define TDEndTag OPTIONAL
#define FRAMESETStartTag REQUIRED
#define FRAMESETEndTag REQUIRED
#define FRAMEStartTag REQUIRED
#define FRAMEEndTag FORBIDDEN
#define IFRAMEStartTag REQUIRED
#define IFRAMEEndTag REQUIRED
#define NOFRAMESStartTag REQUIRED
#define NOFRAMESEndTag REQUIRED
#define HEADStartTag OPTIONAL
#define HEADEndTag OPTIONAL
#define TITLEStartTag REQUIRED
#define TITLEEndTag REQUIRED
#define ISINDEXStartTag REQUIRED
#define ISINDEXEndTag FORBIDDEN
#define BASEStartTag REQUIRED
#define BASEEndTag FORBIDDEN
#define METAStartTag REQUIRED
#define METAEndTag FORBIDDEN
#define STYLEStartTag REQUIRED
#define STYLEEndTag REQUIRED
#define SCRIPTStartTag REQUIRED
#define SCRIPTEndTag REQUIRED
#define NOSCRIPTStartTag REQUIRED
#define NOSCRIPTEndTag REQUIRED
#define HTMLStartTag OPTIONAL
#define HTMLEndTag REQUIRED           // is OPTIONAL, but since it's only 
                                      // allowed once in the document, this 
                                      // takes care of having two of them


} //namespace DOM
#endif
