#include "loginimageview.h"

LoginImageView::LoginImageView(const Rect& cRect) : View(cRect,"login_image_view")
{
	pcImage = LoadImageFromResource("background.png");
	ResizeBackground();
	SetTabOrder( NO_TAB_ORDER );
}

LoginImageView::~LoginImageView()
{
	delete( pcImage );
}

void LoginImageView::ResizeBackground()
{
	os::Desktop cDesktop;
	os::Point cPoint(cDesktop.GetResolution());
	if (cPoint != pcImage->GetSize())
	{
		pcImage->SetSize(cPoint);
	}
}

void LoginImageView::Paint(const Rect& cRect)
{
	FillRect(GetBounds(),get_default_color(COL_NORMAL));
	SetBgColor(get_default_color(COL_NORMAL));
	SetFgColor(255,255,255);
	SetDrawingMode(DM_OVER);
	GetFont()->SetSize(25);
	pcImage->Draw(Point(0,0),this);
	DrawText(Rect(0,0,100,100),GetSystemInfo(),DTF_ALIGN_CENTER);
}
