// AEdit -:-  (C)opyright 2004 Jonas Jarvoll
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

#ifndef __ICONS_H_
#define __ICONS_H_

#include <gui/image.h>
#include <util/resources.h>
#include <util/string.h>

#define STOCK_NEW "stock_new"
#define STOCK_OPEN "stock_open"
#define STOCK_SAVE "stock_save"
#define STOCK_SAVE_AS "stock_save_as"
#define STOCK_CUT "stock_cut"
#define STOCK_COPY "stock_copy"
#define STOCK_PASTE "stock_paste"
#define STOCK_SELECT_ALL "stock_select_all"
#define STOCK_UNDO "stock_undo"
#define STOCK_REDO "stock_redo"
#define STOCK_JUMP_TO "stock_jump_to"
#define STOCK_LEFT_ARROW "stock_left_arrow"
#define STOCK_RIGHT_ARROW "stock_right_arrow"
#define STOCK_EXIT "stock_exit"
#define STOCK_SEARCH "stock_search"
#define STOCK_SEARCH_REPLACE "stock_search_replace"
#define STOCK_CLOSE "stock_close"
#define STOCK_FONT "stock_font"

typedef enum
{
	STOCK_SIZE_MENUITEM,
	STOCK_SIZE_TOOLBAR
} StockSize;

os::Image* GetStockIcon(const char* zStockName, StockSize iSize);

#endif
