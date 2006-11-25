/*
 * Albert 0.4 * Copyright (C)2002 Henrik Isaksson
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "base-n.h"
#include <gui/stringview.h>
#include <gui/tableview.h>
#include "postoffice.h"
#include <iostream>
#include "resources/Calculator.h"

void BaseN::HandleMessage(Message *msg)
{
	int64	i;
	Rect	r;

	switch(msg->GetCode()) {
		case 1:
			msg->FindInt64("value", &i);
			Update(i);
			break;
	}
}

BaseN::BaseN(const Rect & r)
	:Window(r, "BaseN", MSG_BASENWND_TITLE, 0, CURRENT_DESKTOP)
{
	Rect bounds = GetBounds();

	m_Visible = false;

	TableView	*table = new TableView(bounds, "tv", "", 2, 6, CF_FOLLOW_ALL);

	table->SetChild(new StringView(bounds, "s1", MSG_BASENWND_BINARY), 0, 0, 0.0f);
	table->SetChild(new StringView(bounds, "s2", MSG_BASENWND_OCTAL), 0, 1, 0.0f);
	table->SetChild(new StringView(bounds, "s3", MSG_BASENWND_DECIMAL), 0, 2, 0.0f);
	table->SetChild(new StringView(bounds, "s4", MSG_BASENWND_HEXADECIMAL), 0, 3, 0.0f);
	table->SetChild(new StringView(bounds, "s5", MSG_BASENWND_BASE64), 0, 4, 0.0f);
	table->SetChild(new StringView(bounds, "s6", MSG_BASENWND_ROMAN), 0, 5, 0.0f);

	m_Bin = new TextView(bounds, "bin", "");
	m_Bin->SetMinPreferredSize( 10, 1 );
	m_Bin->SetReadOnly(true);
	table->SetChild(m_Bin, 1, 0);

	m_Oct = new TextView(bounds, "oct", "");
	m_Oct->SetReadOnly(true);
	table->SetChild(m_Oct, 1, 1);

	m_Dec = new TextView(bounds, "dec", "");
	m_Dec->SetReadOnly(true);
	table->SetChild(m_Dec, 1, 2);

	m_Hex = new TextView(bounds, "hex", "");
	m_Hex->SetReadOnly(true);
	table->SetChild(m_Hex, 1, 3);

	m_Base64 = new TextView(bounds, "base64", "");
	m_Base64->SetReadOnly(true);
	table->SetChild(m_Base64, 1, 4);

	m_Roman = new TextView(bounds, "roman", "");
	m_Roman->SetReadOnly(true);
	table->SetChild(m_Roman, 1, 5);

	AddChild(table);

	AddMailbox("Base-N");
}

BaseN::~BaseN()
{
	RemMailbox("Base-N");
}

bool BaseN::OkToQuit(void)
{
	return true;
}

static char encodingTable [64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
    'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
};

void IToBase64(int64 val, char *bfr)
{
	int i;
	bool start = false;
	char v;

	for(i=10;i>-1;i--) {
		v = encodingTable[(val >> (6*i)) & 0x3F];
		if(v!='A' || start) {
			*bfr++ = v;
			start = true;
		}
	}
	*bfr = 0;
}

void IToBin(int64 val, char *bfr)
{
	int i;
	char *p=bfr;

	for(i=0;i<32;i++, val<<=1)
		if(val&((int64)0x80000000/*00000000L*/)) break;

	for(;i<32;val<<=1,i++) {
		if(val&((int64)0x80000000/*00000000L*/)) {
			*p++ = '1';
		} else {
			*p++ = '0';
		}
		if(((i+1)%4) == 0)
			*p++ = ' ';
	}
	*p = 0;
}

int64 BinToI(char *bfr)
{
	int64 i;
	for(i=0;*bfr;bfr++) {
		if(*bfr == '0') {
			i <<= 1;
		}
		if(*bfr == '1') {
			i <<= 1;
			i |= 1;
		}
	}

	return i;
}

char *AddRomNum(int i, char *p, char low, char mid, char high)
{
	switch(i) {
		case 0:
			break;
		case 1:
			*p++ = low;
			break;
		case 2:
			*p++ = low;
			*p++ = low;
			break;
		case 3:
			*p++ = low;
			*p++ = low;
			*p++ = low;
			break;
		case 4:
			*p++ = low;
			*p++ = mid;
			break;
		case 5:
			*p++ = mid;
			break;
		case 6:
			*p++ = mid;
			*p++ = low;
			break;
		case 7:
			*p++ = mid;
			*p++ = low;
			*p++ = low;
			break;
		case 8:
			*p++ = low;
			*p++ = low;
			*p++ = high;
			break;
		case 9:
			*p++ = low;
			*p++ = high;
			break;
	}

	return p;
}

void IToRom(int64 val, char *bfr)
{
	char *p = bfr;
	int x;

	/*
		I = 1
		X = 10
		C = 100
		M = 1000
		V = 5
		L = 50
		D = 500
					*/

	if(val>=10000) {
		sprintf(bfr, "-E-");
	} else {
		int ental = val % 10;
		int tiotal = (val % 100)/10;
		int hundratal = (val % 1000)/100;
		int tusental = (val % 10000)/1000;

		for(x=0; x<tusental && x<10; x++) {
			*p++ = 'M';
		}

		p = AddRomNum(hundratal, p, 'C', 'D', 'M');
		p = AddRomNum(tiotal, p, 'X', 'L', 'C');
		p = AddRomNum(ental, p, 'I', 'V', 'X');

		*p = 0;
	}
}

void BaseN::Update(int64 i)
{
	char txt[128];

	sprintf(txt, "%Ld", i);
	m_Dec->Set(txt);

	sprintf(txt, "%Lx", i);
	m_Hex->Set(txt);

	sprintf(txt, "%Lo", i);
	m_Oct->Set(txt);

	IToBin(i, txt);
	m_Bin->Set(txt);

	IToBase64(i, txt);
	m_Base64->Set(txt);

	IToRom(i, txt);
	m_Roman->Set(txt);
}



