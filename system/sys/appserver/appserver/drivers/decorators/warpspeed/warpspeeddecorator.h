#include <windowdecorator.h>
#include <gui/font.h>

#include <string>

using namespace os;

class WarpSpeedDecorator : public os::WindowDecorator
{
public:
	WarpSpeedDecorator(Layer* pcLayer, uint32 nWndFlags);
	virtual ~WarpSpeedDecorator();

	virtual hit_item HitTest(const Point& cPosition);
	virtual void FrameSized(const Rect& cNewFrame);
	virtual Rect GetBorderSize();
	virtual Point GetMinimumSize();
	virtual void SetTitle(const char* pzTitle);
	virtual void SetFlags(uint32 nFlags);
	virtual void FontChanged();
	virtual void SetWindowFlags(uint32 nFlags);
	virtual void SetFocusState(bool bHasFocus);
	virtual void SetButtonState(uint32 nButton, bool bPushed);
	virtual void Render(const Rect& cUpdateRect);
private:
	void CalculateBorderSizes();
	void Layout();
	void DrawZoomButton();
	void DrawMinimizeButton();
	void DrawCloseButton();
	void DrawTitle();

	os::font_height m_sFontHeight;
	std::string m_cTitle;
	uint32 m_nFlags;

	float m_vLeftBorder;
	float m_vTopBorder;
	float m_vRightBorder;
	float m_vBottomBorder;

	bool m_bHasFocus;
	bool m_bCloseState;
	bool m_bMinimizeState;
	bool m_bZoomState;
	Rect m_cBounds;
	Rect m_cZoomRect;
	Rect m_cMinimizeRect;
	Rect m_cCloseRect;
	Rect m_cTitleRect;
};
