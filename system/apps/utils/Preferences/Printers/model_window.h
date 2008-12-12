/*
 *  Syllable Printer configuration preferences
 *
 *  (C)opyright 2006 - Kristian Van Der Vliet
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef PRINTERS_MODEL_WINDOW_H
#define PRINTERS_MODEL_WINDOW_H

#include <gui/rect.h>
#include <gui/window.h>
#include <gui/treeview.h>
#include <gui/layoutview.h>
#include <gui/button.h>

#define MODELS_LIST	"/system/indexes/share/cups/model/models.list"

enum model_window_messages
{
	M_MODEL_WINDOW_CLOSED = 100,
	M_MODEL_WINDOW_SELECT,
	M_MODEL_WINDOW_CANCEL
};

class ModelWindow : public os::Window
{
	public:
		ModelWindow( const os::Rect &cFrame, os::Handler *pcParent );
		~ModelWindow();
		void HandleMessage( os::Message *pcMessage );
		bool OkToQuit( void );

	private:
		status_t  LoadModels( void );

		os::Handler *m_pcParent;

		os::TreeView *m_pcTreeView;
		os::LayoutView *m_pcLayoutView;
};

#endif	/* PRINTERS_MODEL_WINDOW_H */
