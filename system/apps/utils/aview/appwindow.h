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
#include <gui/checkmenu.h>
#include <gui/filerequester.h>
#include <gui/imagebutton.h>
#include <gui/toolbar.h>
#include <storage/file.h>

#include <vector>
#include <string>

#include "iview.h"
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
		AViewWindow( const Rect &cFrame, bool bFitToImage, bool bFitToWindow, char **ppzArgList, int nArgCount );
		~AViewWindow();
		void HandleMessage( Message *pcMessage );
		bool OkToQuit( void );
		void FrameSized( const Point &cDelta );

		status_t Load( uint nIndex );

		void DisableNav( nav_id nId );
	private:
		void EnableNav( nav_id nId );
		void HideIView( void );
		void ResetWindow( void );
		void DoResize( int nScaleBy );
		void Scale( void );
		void ResetResizeMenu( void );

		BitmapImage* LoadImageFromResource( String zResource );
		void SetButtonImageFromResource( ImageButton* pcButton, String zResource );

		String m_sCurrentFilename;

		IView *m_pcIView;
		Image *m_pcImage;

		Menu *m_pcMenuBar;
		Menu *m_pcApplicationMenu;
		Menu *m_pcFileMenu;
		Menu *m_pcViewMenu;

		CheckMenu *m_pcViewFitImageItem;
		CheckMenu *m_pcViewFitWindowItem;

		MenuItem *m_pcViewPrevItem;
		MenuItem *m_pcViewNextItem;
		MenuItem *m_pcViewFirstItem;
		MenuItem *m_pcViewLastItem;

		Menu *m_pcResizeMenu;

		CheckMenu *m_pcResize10pcItem;
		CheckMenu *m_pcResize25pcItem;
		CheckMenu *m_pcResize50pcItem;
		CheckMenu *m_pcResize75pcItem;
		CheckMenu *m_pcResize100pcItem;
		CheckMenu *m_pcResize150pcItem;
		CheckMenu *m_pcResize200pcItem;
		CheckMenu *m_pcResize400pcItem;

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
		bool m_bFitToWindow;

		Point m_cOriginalSize;
		int m_nScaleBy;

		std::vector <std::string> m_vzFileList;
		uint m_nCurrentIndex;

		Point *m_pcMinWindowSize;
};

#define AW_TOOLBAR_HEIGHT		48
#define AW_STATUSBAR_HEIGHT		30

#endif		// __F_AVIEW_NG_APPWINDOW_H_

