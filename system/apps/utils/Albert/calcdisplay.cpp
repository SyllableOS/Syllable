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

#include "calcdisplay.h"

CalcDisplay::CalcDisplay(const Rect& cFrame)
	:View(cFrame, "Display",CF_FOLLOW_LEFT|CF_FOLLOW_TOP,WID_CLEAR_BACKGROUND)
{
	m_Contents = NULL;
	m_Mem = false;
	m_Inv = false;
	m_Hyp = false;
	m_Par = 0;
}

CalcDisplay::~CalcDisplay()
{
}

void CalcDisplay::Paint(const Rect &update)
{
	Rect cBounds = GetBounds();

	DrawFrame(cBounds, FRAME_RECESSED);

	SetFgColor(0,0,0,0);
	SetBgColor(get_default_color(COL_NORMAL));

	char *text = m_Contents;
	if(!text) text = "0";

	font_height fh;
	GetFontHeight(&fh);
	float fontheight = fh.ascender + fh.descender;
	float baseline = cBounds.top + (cBounds.Height() + 1)/2 - fontheight/2 + fh.ascender;
	float tw = GetStringWidth(text);

	MovePenTo(cBounds.right - 3.0f - tw, baseline);
	DrawString(text);

	float flagpos = cBounds.left + 5.0f;

	if(m_Mem) {
		MovePenTo(flagpos, baseline);
		DrawString("M");
		flagpos += GetStringWidth("M ");
	}

	if(m_Inv) {
		MovePenTo(flagpos, baseline);
		DrawString("INV");
		flagpos += GetStringWidth("INV ");
	}

	if(m_Hyp) {
		MovePenTo(flagpos, baseline);
		DrawString("HYP");
		flagpos += GetStringWidth("HYP ");
	}

	if(m_Par != 0) {
		char bfr[10];
		sprintf(bfr, "(%d ", m_Par);
		MovePenTo(flagpos, baseline);
		DrawString(bfr);
		flagpos += GetStringWidth(bfr);
	}
}

void CalcDisplay::Set(char *p)
{
	m_Contents = p;
	Paint(Rect(0,0,0,0));
	Sync();
}

void CalcDisplay::SetMem(bool mem)
{
	m_Mem = mem;
	Paint(Rect(0,0,0,0));
}

void CalcDisplay::SetInv(bool inv)
{
	m_Inv = inv;
	Paint(Rect(0,0,0,0));
}

void CalcDisplay::SetHyp(bool hyp)
{
	m_Hyp = hyp;
	Paint(Rect(0,0,0,0));
}

void CalcDisplay::SetPar(int par)
{
	m_Par = par;
	Paint(Rect(0,0,0,0));
}

