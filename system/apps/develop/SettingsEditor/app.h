
#ifndef APP_H
#define APP_H

#define APP_VERSION	"0.1"
#define APP_NAME	"Application Settings Editor"
#define APP_ID		"application/x-vnd.digitaliz-ASE"

enum IDs {
	ID_NOP = 10,
	ID_QUIT,
	ID_ABOUT,
	ID_NEW,
	ID_OPEN,
	ID_SAVE,
	ID_SAVE_AS,
	ID_EDIT_ITEM,
	ID_DELETE_ITEM,
	ID_ADD_VARIABLE,
	ID_ADD_VARIABLE_NAME,
	ID_EDIT_CANCEL,
	ID_EDIT_SAVE,
	ID_EDIT_ADD,
	ID_EDIT_DELETE,
	ID_SAVE_INDEX,
	ID_REFRESH,
	ID_UNLOCK,		// Unlock an item in the list (items are locked when edit win opens)
	
	ID_LAST_ID
};

#endif
