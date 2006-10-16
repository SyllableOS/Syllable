//  AView (C)opyright 2005 Kristian Van Der Vliet
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

#include <atheos/types.h>
#include <util/string.h>
#include <util/message.h>
#include <gui/window.h>
#include <gui/rect.h>
#include <gui/point.h>
#include <gui/image.h>
#include <gui/scrollbar.h>
#include <gui/menu.h>
#include <gui/filerequester.h>
#include <gui/imagebutton.h>
#include <storage/file.h>

#include <vector>
#include <string>

#include "iview.h"
#include "toolbar.h"
#include "statusbar.h"

#ifndef __F_AVIEW_NG_APPWINDOW_H_
#define __F_AVIEW_NG_APPWINDOW_H_

enum nav_id
{
	NAV_PREV,
	NAV_NEXT
};

using namespace os;

class AViewWindow : public Window
{
	public:
		AViewWindow( const Rect &cFrame, bool bFitToImage, char **ppzArgList, int nArgCount );
		~AViewWindow();
		void HandleMessage( Message *pcMessage );
		bool OkToQuit( void );

		status_t Load( uint nIndex );

		void DisableNav( nav_id nId );

	private:
		void EnableNav( nav_id nId );
		void HideIView( void );

		BitmapImage* LoadImageFromResource( String zResource );
		void SetButtonImageFromResource( ImageButton* pcButton, String zResource );

		String m_sCurrentFilename;

		IView *m_pcIView;
		Image *m_pcImage;

		Menu *m_pcMenuBar;
		Menu *m_pcApplicationMenu;
		Menu *m_pcFileMenu;
		Menu *m_pcViewMenu;

		MenuItem *m_pcViewPrevItem;
		MenuItem *m_pcViewNextItem;
		MenuItem *m_pcViewFirstItem;
		MenuItem *m_pcViewLastItem;

		ImageButton *m_pcOpenButton;
		ImageButton *m_pcBreakerButton;
		ImageButton *m_pcFirstButton;
		ImageButton *m_pcPrevButton;
		ImageButton *m_pcNextButton;
		ImageButton *m_pcLastButton;

		ToolBar *m_pcToolBar;
		StatusBar *m_pcStatusBar;
		FileRequester *m_pcLoadRequester;

		Invoker *m_pcAlertInvoker;

		float m_vMenuHeight;
		bool m_bFitToImage;

		std::vector <std::string> m_vzFileList;
		uint m_nCurrentIndex;

		Point *m_pcMinWindowSize;
};

#define AW_TOOLBAR_HEIGHT		35
#define AW_STATUSBAR_HEIGHT		24

#endif		// __F_AVIEW_NG_APPWINDOW_H_

