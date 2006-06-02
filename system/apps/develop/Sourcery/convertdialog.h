//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef _CONVERT_DIALOG_H
#define _CONVERT_DIALOG_H

#include <gui/window.h>
#include <gui/stringview.h>
#include <gui/checkbox.h>
#include <gui/button.h>
#include <gui/layoutview.h>
#include <gui/textview.h>
#include <gui/imagebutton.h>
#include <gui/filerequester.h>
#include <util/string.h>
#include <util/shortcutkey.h>

#include <codeview/format.h>

#include "messages.h"

using namespace os;
using namespace cv;

class ConvertDialog : public Window
{
public:
	ConvertDialog(const String& cFormat, const String& cFile);
	~ConvertDialog();

	bool 			OkToQuit();
	void 			HandleMessage(Message*);
private:
	void 			Convert();
	void			Layout();
	void			SetTabs();

	LayoutView*		pcLayoutView;
	VLayoutNode*	pcMain;
	StringView*		pcFileString;
	StringView*		pcSaveToString;
	CheckBox*		pcCssCheck;
	Button*			pcConvertButton;
	Button*			pcCancelButton;
	ImageButton*	pcIBTSaveTo;
	FileRequester*	pcFileReq;
	TextView*		pcTVSaveTo;

	String			cMainFormat;
	String			cFileName;
};

#endif  //_CONVERT_DIALOG_H
