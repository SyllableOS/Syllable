#include "gbar.h"
#include "blendablecolor.h"


GradientBar::GradientBar(const Rect &cFrame, const char *cName)
	: View(cFrame, cName, CF_FOLLOW_ALL, WID_WILL_DRAW | WID_CLEAR_BACKGROUND | WID_FULL_UPDATE_ON_RESIZE)
{
	m_Value = 0;
	m_OldValue = 0;
	m_Start.red = 0;
	m_Start.green = 0;
	m_Start.blue = 255;
	m_End.red = 255;
	m_End.green = 0;
	m_End.blue = 0;
}

void GradientBar::SetValue(float v)
{
	m_OldValue = m_Value;
	m_Value = v;
	DeltaRedraw();
	Sync();
}

void GradientBar::SetStartCol(const Color32_s &c)
{
	m_Start = c;
	Paint(GetBounds());
}

void GradientBar::SetEndCol(const Color32_s &c)
{
	m_End = c;
	Paint(GetBounds());
}

void GradientBar::Paint(const Rect &cUpdateRect)
{
	BlendableColor	col(m_Start);
	Rect		bounds(GetBounds());
	int		x, xstop;

	SetEraseColor(get_default_color(COL_NORMAL));
	DrawFrame(bounds, 0/*FRAME_RECESSED*/);

	bounds.left+=2;
	bounds.top+=2;
	bounds.bottom-=2;
	bounds.right-=2;

	DrawFrame(bounds, FRAME_RECESSED | FRAME_TRANSPARENT | FRAME_THIN);

	bounds.left+=2;
	bounds.top+=2;
	bounds.bottom-=2;
	bounds.right-=2;
	x = (int)(cUpdateRect.left > bounds.left ? cUpdateRect.left : bounds.left);
	xstop = (int)(cUpdateRect.right < bounds.right ? cUpdateRect.right : bounds.right);

	while(x <= xstop && (((float)(x - bounds.left) / bounds.Width()) <= m_Value)) {
		float alpha = (float)(x - bounds.left) / bounds.Width();
		SetFgColor(col.BlendAndCopy(m_End, alpha));
		DrawLine(Point(x, bounds.top), Point(x, bounds.bottom));
		x++;
	}

	Flush();
}

void GradientBar::DeltaRedraw()
{
	Rect		bounds(GetBounds());

	bounds.left+=4;
	bounds.top+=4;
	bounds.bottom-=4;
	bounds.right-=4;

	if(m_Value > m_OldValue) {
		BlendableColor	col(m_Start);
		int		x;

		x = (int)bounds.left + (int)(m_OldValue * bounds.Width());

		while(x <= bounds.right && (((float)(x - bounds.left) / bounds.Width()) <= m_Value)) {
			float alpha = (float)(x - bounds.left) / bounds.Width();
			SetFgColor(col.BlendAndCopy(m_End, alpha));
			DrawLine(Point(x, bounds.top), Point(x, bounds.bottom));
			x++;
		}
	} else if(m_Value < m_OldValue) {
		float width = bounds.Width();
		bounds.right = bounds.left + m_OldValue * width;
		bounds.left += m_Value * width;
		//bounds.right -= (m_OldValue - m_Value) * bounds.Width();
		if(bounds.Width()) EraseRect(bounds);
	}
	Sync();
	Flush();
}

































