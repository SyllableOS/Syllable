/***********************************************************
 *	Copyright (C) 1997, Be Inc.  All rights reserved.
 *
 *  FILE:	glutMenu.h
 *
 *	DESCRIPTION:	the GlutMenu class is a simple popup menu
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include <gui/menu.h>
//#include <MenuItem.h>

/***********************************************************
 *	Definitions
 ***********************************************************/
const int32 MENU_ITEM = 'menu';

/***********************************************************
 *	CLASS:	GlutMenu
 ***********************************************************/
class GlutMenuItem;
class GlutPopUp;
class GlutMenu {
public:
	GlutMenu(int n_id, GLUTselectCB n_select) {
		id = n_id;
		select = n_select;
		list = 0;
		num = 0;
	}
	~GlutMenu();
	os::Menu *CreateMenu(bool toplevel=true);	// construct BPopUpMenu

	// public data
	int id;
	GLUTselectCB select;	// callback function
	GlutMenuItem *list;		// list of menu items
	int num;				// number of items
};

/***********************************************************
 *	CLASS:	GlutMenuItem
 ***********************************************************/
class GlutMenuItem {
public:
	GlutMenuItem(GlutMenu *n_menu, bool n_trig, int n_value, const char *n_label);

	// public data
	GlutMenu *menu;		// parent menu
	bool isTrigger;		// are we a submenu?
	int value;			// value to send, or submenu id if isTrigger
	char *label;		// strdup'ed label string
	GlutMenuItem *next; // next menu entry on list
};

/***********************************************************
 *	CLASS:	GlutPopUp
 ***********************************************************/
class GlutPopUp : public os::PopUpMenu {
public:
	GlutPopUp(int m) : os::PopUpMenu(0, false, false) { menu = m;}
	
	os::Point point;	// point to start menu
	GlutWindow *win;	// pointer to my window
	int menu;		// my menu number
};

/***********************************************************
 *	CLASS:	GlutBMenuItem
 ***********************************************************/
class GlutBMenuItem : public os::MenuItem {
public:
	GlutBMenuItem(const char *name) : os::MenuItem(name, 0) {}
	GlutBMenuItem(BMenu* menu) : os::MenuItem(menu) {}
	int menu, value;
};
