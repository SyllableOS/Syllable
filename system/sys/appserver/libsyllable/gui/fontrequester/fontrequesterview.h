#ifndef _FONTREQUESTER_VIEW_H
#define _FONTREQUESTER_VIEW_H

#include <gui/layoutview.h>
#include <gui/font.h>
#include <gui/listview.h>
#include <gui/window.h>
#include <gui/spinner.h>
#include <gui/checkbox.h>
#include <gui/stringview.h>
#include <gui/frameview.h>
#include <util/looper.h>
#include <util/message.h>

#include "fonthandle.h"

using namespace os;

/** \internal */
class FontRequesterView : public LayoutView
{
public:
	FontRequesterView(FontHandle*, bool bEnableAdvanced=false);
	~FontRequesterView();
	
	virtual void AllAttached();
	virtual void HandleMessage(Message*);
	
	void LoadFontTypes();
	void FontHasChanged();
	void FaceHasChanged();
	void SizeHasChanged();
	void AliasChanged();
	void ShearChanged();
	void RotateChanged();
	
	FontHandle* GetFontHandle();
	
private:
	void SetupFonts();
	void LayoutSystem();
	void InitialFont();
	
	enum
	{
		M_FONT_ALIAS_CHANGED,
		M_FONT_ROTATE_CHANGED,
		M_FONT_SIZE_CHANGED,
		M_FONT_FACE_CHANGED,
		M_FONT_TYPE_CHANGED,
		M_FONT_SHEAR_CHANGED,
	};
	
	ListView* pcFontListView;
	ListView* pcFontFaceListView;
	ListView* pcFontSizeListView;
	
	CheckBox* pcAntiCheck;
	
	StringView* pcTextStringViewOne;
	StringView* pcTextStringViewTwo;
	FrameView* pcTextFrameView;
	
	StringView* pcRotationString;
	StringView* pcShearString;
	
	Spinner* pcRotationSpinner;
	Spinner* pcShearSpinner;	
	
	Looper* pcParentWindowLooper;
	
	FontHandle* pcFontHandle;
	VLayoutNode* pcRoot;

	bool m_bShowAdvanced;
};

#endif



