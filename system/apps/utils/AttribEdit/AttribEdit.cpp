/*
 *  AttribEdit 0.5 (Easy to use file attributes editor)
 *  Copyright (c) 2004 Ruslan Nickolaev (nruslan@hotbox.ru)
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

#include <util/application.h>
#include <util/message.h>
#include <util/resources.h>
#include <util/invoker.h>
#include <gui/window.h>
#include <gui/view.h>
#include <gui/image.h>
#include <gui/menu.h>
#include <gui/requesters.h>
#include <gui/filerequester.h>
#include <gui/radiobutton.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/listview.h>
#include <gui/frameview.h>
#include <gui/checkbox.h>
#include <gui/dropdownmenu.h>
#include <gui/imagebutton.h>
#include <atheos/filesystem.h>
#include <atheos/fs_attribs.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include "etextview.h"

#define AE_VERSION "0.5"
#define AE_CONFIG "Settings/AttribEdit.cfg"
#define MWIND_TITLE "AttribEdit"

// main window sizes
#define MWIND_LEFT		20.0f
#define MWIND_TOP		20.0f
#define MWIND_RIGHT		400.0f
#define MWIND_BOTTOM	400.0f

// Properties window sizes
#define EWIND_X_EDIT	200.0f
#define EWIND_X_NORMAL	200.0f
#define EWIND_Y_EDIT	200.0f
#define EWIND_Y_NORMAL	130.0f

enum g_eMessages {
	ID_APPLICATION_PREFERENCES,
	ID_APPLICATION_ABOUT,
	ID_FILE_EXIT,
	ID_FILE_NEW,
	ID_FILE_OPEN,
	ID_FILE_CLOSE,
	ID_FILE_COPYALL,
	ID_FILE_REMOVEALL,
	ID_FILE_SUMMARY,
	ID_EDIT_PROPERTIES,
	ID_EDIT_COPYTO,
	ID_EDIT_ADD,
	ID_EDIT_REMOVE,
	ID_MITEMS_COUNT, // count of menu items
	ID_FILEREQ_OPEN = ID_MITEMS_COUNT,
	ID_FILEREQ_COPY,
	ID_FILEREQ_NEW,
	ID_SYMWIND_OPEN,
	ID_SYMWIND_COPYALL,
	ID_SYMWIND_COPY,
	ID_REMOVEALL_ANSWERED
};

enum g_eOpenModes {
	ID_MODE_OPEN,
	ID_MODE_COPYALL,
	ID_MODE_COPY
};

static char *g_azMenu[] = { "Application", "File", "Edit", 0 };
static struct g_sAEMenuItem {
	char *m_pzName;
	char *m_pzShortCut;
	char *m_pzIconPath;
} g_asAEMenuItem[] = {
	// Application
	{ "Preferences...", "Ctrl+P", "prefs16x16.png" },
	{ "About...", "", "about16x16.png" },
	{ "", NULL, NULL },
	{ "Quit", "Ctrl+Q", "quit16x16.png" },
	{ NULL, NULL, NULL },

	// File
	{ "New...", "Ctrl+N", "new16x16.png" },
	{ "Open...", "Ctrl+O", "open16x16.png" },
	{ "Close", "Ctrl+Z", "close16x16.png" },
	{ "", NULL, NULL },
	{ "Copy all attributes to...", "Ctrl+S", "saveas16x16.png" },
	{ "Remove all attributes", "Ctrl+D", "removeall16x16.png" },
	{ "", NULL, NULL },
	{ "Summary information", "Ctrl+I", "summary16x16.png" },
	{ NULL, NULL, NULL },

	// Edit
	{ "Properties", "Ctrl+E", "properties16x16.png" },
	{ "Copy to...", "Ctrl+Q", "copyto16x16.png" },
	{ "", NULL, NULL },
	{ "Add...", "Ctrl+A", "add16x16.png" },
	{ "Remove", "Ctrl+R", "remove16x16.png" },
	{ NULL, NULL, NULL }
};
os::MenuItem *g_apcMenuItem[ID_MITEMS_COUNT];

static struct g_sAEMenuItem g_asViewMenuItem[] = {
	{ "Properties", "Ctrl+E", "properties16x16.png" },
	{ "Copy to...", "Ctrl+Q", "copyto16x16.png" },
	{ "", NULL, NULL },
	{ "Add...", "Ctrl+A", "add16x16.png" },
	{ "Remove", "Ctrl+R", "remove16x16.png" },
	{ NULL, NULL, NULL }
};

static struct ListColumn {
	char *name;
	int width;
} g_sListColumn[] = {
	{ "Name", 100 },
	{ "Type", 50 },
	{ "Size", 50 },
	{ 0, 0 }
};

enum g_eAE_Bitmaps {
	AEBITMAP_ABOUT24X24,
	AEBITMAP_ABOUT32X32,
	AEBITMAP_SUMMARY24X24,
	AEBITMAP_QUESTION24X24,
	AEBITMAP_PREFS24X24,
	AEBITMAP_ERROR24X24,
	AEBITMAP_ERROR32X32,
	AEBITMAP_PROPERTIES24X24,
	AEBITMAP_FOLDER24X24,
	AEBITMAP_COUNT
};

os::Bitmap *g_apcBitmap[AEBITMAP_COUNT];
static char *g_apzAE_Bitmap[] = {
    "about24x24.png", "about32x32.png", "summary24x24.png", "question24x24.png",
	"prefs24x24.png", "error24x24.png", "error32x32.png", "properties24x24.png",
	"folder24x24.png"
};

enum g_eAE_BitmapImages {
	AEBITMAPIMG_LOAD22X22,
	AEBITMAPIMG_DUMP22X22,
	AEBITMAPIMG_COUNT
};

os::BitmapImage *g_apcBitmapImg[AEBITMAPIMG_COUNT];
static char *g_apzAE_BitmapImg[] = {
	"load22x22.png", "dump22x22.png"
};

static char *g_apzAttrType[] = {
	"raw",
	"int32",
	"int64",
	"float",
	"double",
	"string"
};

static char *g_apzSummaryLabels[] = {
	"Raw data count:",
	"Int32 numbers count:",
	"Int64 numbers count:",
	"Float numbers count:",
	"Double numbers count:",
	"Strings count:",
	"Unknown data count:"
};

static int g_hFd = -1;

static char *g_azMainWindowError[] = {
	"File not found!",
	"Can't stat file!",
	"Can't read file!",
	"Can't write file!",
	"File format is incorrect!",
	"Unknown error!",
	"Opening attributes directory failed!",
	"Error copying attributes!",
	"Please select attribute before!",
	"Please close all editing windows before!",
	"Please close all file requesters dialogs before!",
	"Please close all dialog windows before!",
	"The option is unvailable for attributes\nwith unknown type!",
	"Can't stat attribute!",
	"Can't read attribute!",
	"Can't write attribute!",
	"Attribute type was changed!",
	"Memory out!",
	"Attribute with the same name already exists!",
	"Attribute name can't be empty!",
	"Incorrect value!",
	"You should load attribute first!"
};

enum g_eErrors {
	ID_ERROR_FILENOTFOUND,
	ID_ERROR_FILESTAT,
	ID_ERROR_FILEREAD,
	ID_ERROR_FILEWRITE,
	ID_ERROR_FILEFORMAT,
	ID_ERROR_UNKNOWN,
	ID_ERROR_OPENATTRDIR,
	ID_ERROR_COPYING,
	ID_ERROR_SELECTATTR,
	ID_ERROR_EDIT,
	ID_ERROR_FILEREQ,
	ID_ERROR_DIALOG,
	ID_ERROR_UNKNOWNATTR,
	ID_ERROR_ATTRSTAT,
	ID_ERROR_ATTRREAD,
	ID_ERROR_ATTRWRITE,
	ID_ERROR_TYPECHANGE,
	ID_ERROR_MEMORYOUT,
	ID_ERROR_ATTRNAME,
	ID_ERROR_ATTREMPTYNAME,
	ID_ERROR_INCORNUM,
	ID_ERROR_LOADATTR
};

static char *g_apzPrefsString[] = {
	"Opening symbolic links",
	"Copying to symbolic links"
};

static char *g_apzAddButton[] = {
	"Cancel", "Next"
};

#define CANCEL_BUTTON *g_apzAddButton

static char *g_apzPrefsButton[] = {
	"Defaults", CANCEL_BUTTON, "OK"
};

#define OK_BUTTON g_apzPrefsButton[2]

static char *g_apzEditButton[] = {
	"Save", "Close"
};

static char *g_apzEditRawButton[] = {
	"Load", "Dump"
};

// calculated powers of two (0, 1, ...)
static int g_apnPower[] = { 1, 2 };

static struct g_sSummaryType {
	uint m_anAttrCount[ATTR_TYPE_COUNT + 1];
	uint m_nTotalAttrCount;
	off_t m_nTotalAttrSize;
} g_sSummary = { { 0u, 0u, 0u, 0u, 0u, 0u, 0u}, 0u, 0 };

enum g_ePrefs {
	PREFS_SAVEAS_TRUNC = 01,
	PREFS_SLNKWIND = 02,
	PREFS_SLNK_MANUAL = 04,
	PREFS_SLNK_MANUAL_COPYING = 010,
	PREFS_SLNK_MODE = 020,
	PREFS_SLNK_MODE_COPYING = 040
};

static struct {
	int m_nAttrType;
	float m_vEditX;
	float m_vEditY;
	float m_vEditXnormal;
	int m_nPrefs;
} g_sPreferences = { 0, EWIND_X_EDIT, EWIND_Y_EDIT, EWIND_X_NORMAL, PREFS_SAVEAS_TRUNC | PREFS_SLNK_MANUAL | PREFS_SLNK_MANUAL_COPYING };

// "C++"-style functions
static os::Bitmap *CopyBitmap(os::Bitmap *pcSrcIcon);
static void ShowError(os::Window *pcWnd, int nErrorCode);

// "C"-style functions
extern "C" {
	static int GetAttribType(const char *pzString);
	static void ConvertString(const char *pzBufferOld, char *pzBufferNew);
}

namespace ae
{
	class MainWindow;

	// ------------------------------------------------------- //
	// MainView class
	// ------------------------------------------------------- //
	class MainView : public os::ListView
	{
		public:
			MainView(const os::Rect &cFrame);
			virtual void HandleMessage(os::Message *pcMsg);
			virtual void MouseDown(const os::Point &cPosition, uint32 nButton);
			void InsertRow(int nPos, os::ListViewRow *pcRow, bool bUpdate = true);
			void InsertRow(os::ListViewRow *pcRow, bool bUpdate = true);
			os::ListViewRow *RemoveRow(int nIndex, bool bUpdate = true);
			void Clear();
			void MenuLock(bool bValue = true);
			uint GetSelectedRow();
			virtual ~MainView();
		private:
			os::Menu *m_pcMenu;

			enum m_eMessages {
				ID_PROPERTIES,
				ID_COPYTO,
				ID_ADD,
				ID_REMOVE,
				ID_COUNT // count of elements
			};

			os::MenuItem *m_apcMenuItem[ID_COUNT];
	};

	// ------------------------------------------------------- //
	// Add dialog window class
	// ------------------------------------------------------- //
	class AddWindow : public os::Window
	{
		private:
			MainWindow *m_pcParent;
			os::DropdownMenu *m_pcDDMenu;
			os::Button *m_pcNext, *m_pcCancel;
			bool m_nRetMainWindow;

			enum m_eMessages {
				ID_CANCEL,
				ID_NEXT,
				ID_BUTTON_COUNT
			};

			os::Button *m_apcButton[ID_BUTTON_COUNT];

		public:
			AddWindow(MainWindow *pcParent);
			virtual void HandleMessage(os::Message *pcMsg);
			virtual ~AddWindow();
	};

	// ------------------------------------------------------- //
	// Editing window class
	// ------------------------------------------------------- //
	class EditWindow : public os::Window
	{
		private:
			MainWindow *m_pcParent;
			os::TextView *m_pcName, *m_pcValue;
			os::FileRequester *m_pcLoadReq, *m_pcDumpReq;
			os::ListViewStringRow *m_pcRow;
			bool m_nIsFileReq;
			int m_nAttrType, m_nErrCode;
			char *m_pzFilePath, *m_pzName;
			uint m_nRow;

			enum m_eMessages {
				ID_OK,
				ID_CANCEL,
				ID_BUTTON_COUNT,
				ID_LOAD = ID_BUTTON_COUNT,
				ID_DUMP,
				ID_FILEREQ_LOAD,
				ID_FILEREQ_DUMP
			};

			enum m_eRawButtons {
				ID_RAWBUTTON_LOAD,
				ID_RAWBUTTON_DUMP,
				ID_RAWBUTTON_COUNT
			};

			os::Button *m_apcButton[ID_BUTTON_COUNT];
			os::ImageButton *m_apcRawButton[ID_RAWBUTTON_COUNT];

		public:
			EditWindow(MainWindow *pcParent, int nAttrType, uint nRow = UINT_MAX);
			inline int GetErrorCode();
			virtual void HandleMessage(os::Message *pcMsg);
			virtual ~EditWindow();
	};

	// ------------------------------------------------------- //
	// Symlink dialog window class
	// ------------------------------------------------------- //
	class SlnkWindow : public os::Window
	{
		private:
			os::RadioButton *m_pcRegularFile, *m_pcSymlink;
			os::Window *m_pcParent;
			os::Message *m_pcMsg;
			char *m_pzFilePath;

			enum m_eMessages {
				ID_SLNKWIND_REGFILE,
				ID_SLNKWIND_SYMLINK,
				ID_SLNKWIND_OK
			};
		public:
			SlnkWindow(os::Window *pcParent, const char *pzFilePath, os::Message *pcMsg);
			virtual void HandleMessage(os::Message *pcMsg);
			virtual ~SlnkWindow();
	};

	// ------------------------------------------------------- //
	// Preferences window class
	// ------------------------------------------------------- //
	class PrefsWindow : public os::Window
	{
		private:
			MainWindow *m_pcParent;
			os::CheckBox *m_pcOverwrite;
			os::RadioButton *m_apcFollowLinks[2], *m_apcDirectOpen[2], *m_apcAsk[2];

			enum m_eMessages {
				ID_DEFAULTS,
				ID_CANCEL,
				ID_SAVE,
				ID_BUTTON_COUNT,
				ID_OVERWRITE = ID_BUTTON_COUNT,
				ID_FOLLOW_LINKS,
				ID_FOLLOW_LINKS_COPYING,
				ID_DIRECT_OPEN,
				ID_DIRECT_OPEN_COPYING,
				ID_ASK,
				ID_ASK_COPYING
			};

			os::Button *m_apcButton[ID_BUTTON_COUNT];

		public:
			PrefsWindow(MainWindow *pcParent);
			virtual void HandleMessage(os::Message *pcMsg);
			virtual ~PrefsWindow();
	};

	// ------------------------------------------------------- //
	// Summary window class
	// ------------------------------------------------------- //
	class SummaryWindow : public os::Window
	{
		private:
			enum m_eMessages {
				ID_CLOSEBUT
			};
		public:
			SummaryWindow();
			virtual void HandleMessage(os::Message *pcMsg);
			virtual ~SummaryWindow();
	};

	// ------------------------------------------------------- //
	// MainWindow class
	// ------------------------------------------------------- //
	class MainWindow : public os::Window
	{
		private:
			os::Menu *m_pcMenu, *m_pcFileMenu;
			os::FileRequester *m_pcOpenReq, *m_pcCopyReq, *m_pcNewReq;
			bool m_nIsFileReq;
			int m_nCopyMode;
			uint m_nCopyRow;
			os::Alert *m_pcAlert;

		public:
			os::Window *m_pcPrefsWindow, *m_pcAddEditWindow;
			MainView *m_pcView;
			MainWindow(const os::Rect &cFrame);
			virtual void HandleMessage(os::Message *pcMsg);
			void Open(const char *pzFilePath, int nMode);
			void FileOpen(const char *zFilePath, int nOpenFlags, bool nUnlink = false);
			void FileCopyAll(const char *zFilePath, int nOpenFlags);
			void FileCopy(const char *zFilePath, int nOpenFlags);
			void RemoveAll();
			void CloseFile();
			void ManageLock(bool nValue = true);
			virtual bool OkToQuit();
			virtual ~MainWindow();
	};

	// ------------------------------------------------------- //
	// AttribEdit application class
	// ------------------------------------------------------- //
	class AttribEdit : public os::Application
	{
		private:
			MainWindow *m_pcWind;
		public:
			AttribEdit(const char *zFilePath);
			virtual bool OkToQuit();
			virtual ~AttribEdit();
	};

	// ------------------------------------------------------- //
	// MainView constructor
	// ------------------------------------------------------- //
	MainView::MainView(const os::Rect &cFrame)
	  : os::ListView(cFrame, "", os::ListView::F_RENDER_BORDER, os::CF_FOLLOW_ALL)
	{
		os::Resources cAEResources(get_image_id());
		os::ResStream *pcResStream;
		os::MenuItem *pcMenuItem;
		os::Rect cRect = GetBounds();
		struct g_sAEMenuItem *psMenuItem = g_asViewMenuItem;
		int i = 0;

		for (ListColumn *sColumn = g_sListColumn; sColumn->name; sColumn++) {
			i += sColumn->width;
			InsertColumn(sColumn->name, sColumn->width);
		}

		InsertColumn("Value", (int)cRect.right - i - 4);

		m_pcMenu = new os::Menu(os::Rect(0.0f, 0.0f, 1.0f, 1.0f), "", os::ITEMS_IN_COLUMN, os::CF_FOLLOW_NONE);

		for (i = 0; psMenuItem->m_pzName; psMenuItem++)
		{
			switch (*(psMenuItem->m_pzName))
			{
				case NULL:
					pcMenuItem = new os::MenuSeparator;
					break;

				default:
				{
					pcResStream = psMenuItem->m_pzIconPath ? cAEResources.GetResourceStream(psMenuItem->m_pzIconPath) : NULL;
					pcMenuItem = new os::MenuItem(psMenuItem->m_pzName, new os::Message(i), psMenuItem->m_pzShortCut, pcResStream ? new os::BitmapImage(pcResStream) : NULL);
					if (pcResStream)
						delete pcResStream;
					m_apcMenuItem[i++] = pcMenuItem;
					break;
				}
			}
			m_pcMenu->AddItem(pcMenuItem);
		}

		MenuLock();
	}

	// ------------------------------------------------------- //
	// virtual method: HandleMessage
	// ------------------------------------------------------- //
	void MainView::HandleMessage(os::Message *pcMsg)
	{
		switch (pcMsg->GetCode())
		{
			case ID_PROPERTIES:
				Invoke(new os::Message(ID_EDIT_PROPERTIES));
				break;

			case ID_ADD:
				Invoke(new os::Message(ID_EDIT_ADD));
				break;

			case ID_REMOVE:
				Invoke(new os::Message(ID_EDIT_REMOVE));
				break;

			case ID_COPYTO:
				Invoke(new os::Message(ID_EDIT_COPYTO));
				break;

			default:
				os::ListView::HandleMessage(pcMsg);
				break;
		}
	}

	// ------------------------------------------------------- //
	// virtual method: MouseDown
	// ------------------------------------------------------- //
	void MainView::MouseDown(const os::Point &cPosition, uint32 nButton)
	{
		switch (nButton)
		{
			case 2:
			{
				m_apcMenuItem[ID_ADD]->SetEnable(g_hFd == -1 ? false : true);
				m_pcMenu->Open(ConvertToScreen(cPosition));
				m_pcMenu->SetTargetForItems(this);
				break;
			}

			default:
				os::ListView::MouseDown(cPosition, nButton);
				break;
		}
	}

    void MainView::InsertRow(int nPos, os::ListViewRow *pcRow, bool bUpdate)
	{
		register uint nRowCount = GetRowCount();
		os::ListView::InsertRow(nPos, pcRow, bUpdate);
		if (nRowCount == 0)
			MenuLock(false);
	}

	void MainView::InsertRow(os::ListViewRow *pcRow, bool bUpdate)
	{
		register uint nRowCount = GetRowCount();
		os::ListView::InsertRow(pcRow, bUpdate);
		if (nRowCount == 0)
			MenuLock(false);
	}

	os::ListViewRow *MainView::RemoveRow(int nIndex, bool bUpdate)
	{
		register os::ListViewRow *pcListViewRow = os::ListView::RemoveRow(nIndex, bUpdate);
		register uint nRowCount = GetRowCount();
		if (nRowCount == 0)
			MenuLock();

		return pcListViewRow;
	}

	void MainView::Clear()
	{
		MenuLock();
		os::ListView::Clear();
	}

	void MainView::MenuLock(bool bLock)
	{
		bLock = !bLock;
		m_apcMenuItem[ID_PROPERTIES]->SetEnable(bLock);
		m_apcMenuItem[ID_REMOVE]->SetEnable(bLock);
		m_apcMenuItem[ID_COPYTO]->SetEnable(bLock);
		g_apcMenuItem[ID_FILE_COPYALL]->SetEnable(bLock);
		g_apcMenuItem[ID_FILE_REMOVEALL]->SetEnable(bLock);
		g_apcMenuItem[ID_EDIT_PROPERTIES]->SetEnable(bLock);
		g_apcMenuItem[ID_EDIT_REMOVE]->SetEnable(bLock);
		g_apcMenuItem[ID_EDIT_COPYTO]->SetEnable(bLock);
	}

	// ------------------------------------------------------- //
	// Getting current row
	// ------------------------------------------------------- //
	uint MainView::GetSelectedRow()
	{
		register uint i = 0, nRowCount = GetRowCount();

		for (; i < nRowCount; i++)
			if (IsSelected(i))
				return i;

		// row wasn't found
		return UINT_MAX;
	}

	// ------------------------------------------------------- //
	// MainView destructor
	// ------------------------------------------------------- //
	MainView::~MainView()
	{
	}

	// ------------------------------------------------------- //
	// SummaryWindow constructor
	// ------------------------------------------------------- //
	SummaryWindow::SummaryWindow()
	  : os::Window(os::Rect(0.0f, 0.0f, 230.0f, 240.0f), "summary_window", "Summary", os::WND_NO_ZOOM_BUT | os::WND_NO_DEPTH_BUT | os::WND_NOT_RESIZABLE)
	{
		char azBuffer[21];
		SetIcon(g_apcBitmap[AEBITMAP_SUMMARY24X24]);
		os::Rect cRect = GetBounds();

		register os::View *pcView = new os::View(cRect, "", os::CF_FOLLOW_NONE);
		AddChild(pcView);

		cRect.left = 10.0f;
		cRect.top = 10.0f;
		cRect.bottom = cRect.top + 20.0f;
		os::Rect cRect2(140.0f, cRect.top, cRect.right - 10.0f, cRect.bottom);
		cRect.right = 140.0f;

		pcView->AddChild(new os::StringView(cRect, "", "Total attributes size:", os::ALIGN_LEFT, os::CF_FOLLOW_NONE));
		sprintf(azBuffer, "%lli", g_sSummary.m_nTotalAttrSize);
		pcView->AddChild(new os::StringView(cRect2, "", azBuffer, os::ALIGN_LEFT, os::CF_FOLLOW_NONE));

		cRect.top += 20.0f;
		cRect.bottom += 20.0f;
		cRect2.top += 20.0f;
		cRect2.bottom += 20.0f;
		pcView->AddChild(new os::StringView(cRect, "", "Total attributes count:", os::ALIGN_LEFT, os::CF_FOLLOW_NONE));
		sprintf(azBuffer, "%u", g_sSummary.m_nTotalAttrCount);
		pcView->AddChild(new os::StringView(cRect2, "", azBuffer, os::ALIGN_LEFT, os::CF_FOLLOW_NONE));
		cRect.top += 30.0f;
		cRect.bottom += 30.0f;
		cRect2.top += 30.0f;
		cRect2.bottom += 30.0f;

		for (register int i = 0; i <= ATTR_TYPE_COUNT; i++)
		{
			pcView->AddChild(new os::StringView(cRect, "", g_apzSummaryLabels[i], os::ALIGN_LEFT, os::CF_FOLLOW_NONE));
			sprintf(azBuffer, "%u", g_sSummary.m_anAttrCount[i]);
			pcView->AddChild(new os::StringView(cRect2, "", azBuffer, os::ALIGN_LEFT, os::CF_FOLLOW_NONE));
			cRect.top += 20.0f;
			cRect.bottom += 20.0f;
			cRect2.top += 20.0f;
			cRect2.bottom += 20.0f;
		}

		cRect2.top += 10.0f;
		cRect2.bottom += 10.0f;
		cRect2.left = cRect2.right - 65.0f;
		os::Button *pcButton = new os::Button(cRect2, "", "Close", new os::Message(ID_CLOSEBUT), os::CF_FOLLOW_NONE);
		pcView->AddChild(pcButton);
		SetDefaultButton(pcButton);
	}

	// ------------------------------------------------------- //
	// HandleMessage
	// ------------------------------------------------------- //
	void SummaryWindow::HandleMessage(os::Message *pcMsg)
	{
		switch (pcMsg->GetCode())
		{
			case ID_CLOSEBUT:
				PostMessage(os::M_QUIT);
				break;

			default:
				os::Window::HandleMessage(pcMsg);
				break;
		}
	}

	// ------------------------------------------------------- //
	// SummaryWindow destructor
	// ------------------------------------------------------- //
	SummaryWindow::~SummaryWindow()
	{
	}

	// ------------------------------------------------------- //
	// SlnkWindow constructor
	// ------------------------------------------------------- //
	SlnkWindow::SlnkWindow(os::Window *pcParent, const char *pzFilePath, os::Message *pcMsg)
	  : os::Window(os::Rect(0.0f, 0.0f, 200.0f, 105.0f), "slnk_window", "Symbolic link operation", os::WND_NO_CLOSE_BUT | os::WND_NO_ZOOM_BUT | os::WND_NO_DEPTH_BUT | os::WND_NOT_RESIZABLE),
	    m_pcParent(pcParent), m_pcMsg(pcMsg), m_pzFilePath(strdup(pzFilePath))
	{
		SetIcon(g_apcBitmap[AEBITMAP_QUESTION24X24]);
		os::View *pcView = new os::View(GetBounds(), "", os::CF_FOLLOW_NONE);
		os::Rect cRect = GetBounds();
		cRect.left = 10.0f;
		cRect.top = 10.0f;
		cRect.right -= 10.0f;
		register float vXcoord = cRect.right;
		cRect.bottom = 20.0f;
		os::StringView *pcString = new os::StringView(cRect, "", "Symbolic link operation mode:");
		pcView->AddChild(pcString);
		cRect.left = 20.0f;
		cRect.top = cRect.bottom + 10.0f;
		cRect.right = 100.0f;
		cRect.bottom += 30.0f;
		m_pcSymlink = new os::RadioButton(cRect, "", "Follow link", new os::Message(ID_SLNKWIND_SYMLINK));
		pcView->AddChild(m_pcSymlink);
		cRect.top = cRect.bottom;
		cRect.bottom += 20.0f;
		m_pcRegularFile = new os::RadioButton(cRect, "", "Direct", new os::Message(ID_SLNKWIND_REGFILE));
		pcView->AddChild(m_pcRegularFile);
		if (g_sPreferences.m_nPrefs & PREFS_SLNKWIND)
			m_pcRegularFile->SetValue(true);
		else
			m_pcSymlink->SetValue(true);
		cRect.right = vXcoord;
		cRect.left = vXcoord - 65.0f;
		cRect.top = cRect.bottom + 5.0f;
		cRect.bottom += 25.0f;
		os::Button *pcOkButton = new os::Button(cRect, "", OK_BUTTON, new os::Message(ID_SLNKWIND_OK));
		pcView->AddChild(pcOkButton);
		SetDefaultButton(pcOkButton);
		AddChild(pcView);
	}

	// ------------------------------------------------------- //
	// HandleMessage
	// ------------------------------------------------------- //
	void SlnkWindow::HandleMessage(os::Message *pcMsg)
	{
		switch (pcMsg->GetCode())
		{
			case ID_SLNKWIND_OK:
			{
				register int nOpenFlags = O_RDWR;
				m_pcMsg->AddString("file/path", m_pzFilePath);
				if (m_pcRegularFile->GetValue()) {
					nOpenFlags |= O_NOTRAVERSE;
					g_sPreferences.m_nPrefs |= PREFS_SLNKWIND;
				}
				else
					g_sPreferences.m_nPrefs &= ~PREFS_SLNKWIND;
				m_pcMsg->AddInt32("file/mode", nOpenFlags);
				os::Invoker *pcInvoker = new os::Invoker(m_pcMsg, m_pcParent);
				pcInvoker->Invoke();
				PostMessage(os::M_QUIT);
				break;
			}

			case ID_SLNKWIND_SYMLINK:
			case ID_SLNKWIND_REGFILE:
				break;

			default:
				os::Window::HandleMessage(pcMsg);
				break;
		}
	}

	// ------------------------------------------------------- //
	// SlnkWindow destructor
	// ------------------------------------------------------- //
	SlnkWindow::~SlnkWindow()
	{
		free(m_pzFilePath);
	}

	// ------------------------------------------------------- //
	// PrefsWindow constructor
	// ------------------------------------------------------- //
	PrefsWindow::PrefsWindow(MainWindow *pcParent)
	 : os::Window(os::Rect(0, 0, 210, 270), "prefs_window", "Preferences", os::WND_NO_ZOOM_BUT | os::WND_NO_DEPTH_BUT | os::WND_NOT_RESIZABLE),
	   m_pcParent(pcParent)
	{
		SetIcon(g_apcBitmap[AEBITMAP_PREFS24X24]);
		os::Rect cRect = GetBounds();
		os::View *pcView = new os::View(cRect, "", os::CF_FOLLOW_NONE);
		AddChild(pcView);

		cRect.left += 10.0f;
		cRect.right -= 10.0f;
		cRect.bottom = 0.0f;

		os::FrameView *pcSlnkView;
		os::Rect cRect2;
		register int i = 0;

		for (register int nPower; i < 2; i++)
		{
			cRect.top = cRect.bottom + 10.0f;
			cRect.bottom = cRect.top + 90.0f;
			pcSlnkView = new os::FrameView(cRect, "", g_apzPrefsString[i], os::CF_FOLLOW_NONE);
			AddChild(pcSlnkView);
			cRect2 = pcSlnkView->GetBounds();
			cRect2.left = 10.0f;
			cRect2.right -= 10.0f;
			cRect2.top = 20.0f;
			cRect2.bottom = cRect2.top + 20.0f;
			m_apcFollowLinks[i] = new os::RadioButton(cRect2, "", "Follow link", new os::Message(ID_FOLLOW_LINKS + i), os::CF_FOLLOW_NONE);
			pcSlnkView->AddChild(m_apcFollowLinks[i]);
			cRect2.top = cRect2.bottom;
			cRect2.bottom += 20.0f;
			m_apcDirectOpen[i] = new os::RadioButton(cRect2, "", "Direct", new os::Message(ID_DIRECT_OPEN + i), os::CF_FOLLOW_NONE);
			pcSlnkView->AddChild(m_apcDirectOpen[i]);
			cRect2.top = cRect2.bottom;
			cRect2.bottom += 20.0f;
			m_apcAsk[i] = new os::RadioButton(cRect2, "", "Always ask about it", new os::Message(ID_ASK + i), os::CF_FOLLOW_NONE);
			pcSlnkView->AddChild(m_apcAsk[i]);

			nPower = g_apnPower[i];

			if (g_sPreferences.m_nPrefs & (PREFS_SLNK_MANUAL * nPower))
				m_apcAsk[i]->SetValue(true);
			else if (g_sPreferences.m_nPrefs & (PREFS_SLNK_MODE * nPower))
				m_apcDirectOpen[i]->SetValue(true);
			else
				m_apcFollowLinks[i]->SetValue(true);
		}

		cRect.top = cRect.bottom + 10.0f;
		cRect.bottom += 30.0f;

		m_pcOverwrite = new os::CheckBox(cRect, "", "Ovewrite mode for copying", new os::Message(ID_OVERWRITE), os::CF_FOLLOW_NONE);
		m_pcOverwrite->SetValue(g_sPreferences.m_nPrefs & PREFS_SAVEAS_TRUNC);
		pcView->AddChild(m_pcOverwrite);

		cRect.top += 30.0f;
		cRect.bottom += 30.0f;
		cRect.right = 5.0f;

		for (i = 0; i < ID_BUTTON_COUNT; i++) {
			cRect.left = cRect.right + 5.0f;
			cRect.right += 65.0f;
			m_apcButton[i] = new os::Button(cRect, "", g_apzPrefsButton[i], new os::Message(i), os::CF_FOLLOW_NONE);
			pcView->AddChild(m_apcButton[i]);
		}

		SetDefaultButton(m_apcButton[ID_SAVE]);
	}

	// ------------------------------------------------------- //
	// HandleMessage
	// ------------------------------------------------------- //
	void PrefsWindow::HandleMessage(os::Message *pcMsg)
	{
		switch (pcMsg->GetCode())
		{
			case ID_DEFAULTS:
			{
				m_pcOverwrite->SetValue(true);
				for (register int i = 0; i < 2; i++)
					m_apcAsk[i]->SetValue(true);
				break;
			}

			case ID_SAVE:
			{
				if (m_pcOverwrite->GetValue())
					g_sPreferences.m_nPrefs |= PREFS_SAVEAS_TRUNC;
				else
					g_sPreferences.m_nPrefs &= ~PREFS_SAVEAS_TRUNC;

				register int nPower;
				for (register int i = 0; i < 2; i++)
				{
					nPower = g_apnPower[i];

					if (m_apcAsk[i]->GetValue())
						g_sPreferences.m_nPrefs |= PREFS_SLNK_MANUAL * nPower;

					else
					{
						g_sPreferences.m_nPrefs &= ~(PREFS_SLNK_MANUAL * nPower);
						if (m_apcDirectOpen[i]->GetValue())
							g_sPreferences.m_nPrefs |= (PREFS_SLNK_MODE * nPower);
						else
							g_sPreferences.m_nPrefs &= ~(PREFS_SLNK_MODE * nPower);
					}
				}

				// don't add break here!
			}

			case ID_CANCEL:
				PostMessage(os::M_QUIT);
				break;

			case ID_OVERWRITE:
			case ID_FOLLOW_LINKS:
			case ID_DIRECT_OPEN:
			case ID_ASK:
				break;

			default:
				os::Window::HandleMessage(pcMsg);
				break;
		}
	}

	// ------------------------------------------------------- //
	// PrefsWindow destructor
	// ------------------------------------------------------- //
	PrefsWindow::~PrefsWindow()
	{
		m_pcParent->Lock();
		m_pcParent->m_pcPrefsWindow = NULL;
		m_pcParent->Unlock();
	}

	// ------------------------------------------------------- //
	// Add dialog window constructor
	// ------------------------------------------------------- //
	AddWindow::AddWindow(MainWindow *pcParent)
	 : os::Window(os::Rect(0.0f, 0.0f, 145.0f, 95.0f), "add_window", "Adding new attribute", os::WND_NO_ZOOM_BUT | os::WND_NO_DEPTH_BUT | os::WND_NOT_RESIZABLE),
	   m_pcParent(pcParent), m_nRetMainWindow(true)
	{
		SetIcon(g_apcBitmap[AEBITMAP_QUESTION24X24]);

		os::Rect cRect = GetBounds();

		register os::View *pcView = new os::View(cRect, "", os::CF_FOLLOW_NONE);
		AddChild(pcView);

		cRect.left = 10.0f;
		cRect.right -= 10.0f;
		cRect.top = 10.0f;
		cRect.bottom = 30.0f;

		pcView->AddChild(new os::StringView(cRect, "", "Choice attribute type:", os::ALIGN_CENTER, os::CF_FOLLOW_NONE));

		cRect.left += 10.0f;
		cRect.right -= 10.0f;
		cRect.top = cRect.bottom + 5.0f;
		cRect.bottom += 25.0f;

		m_pcDDMenu = new os::DropdownMenu(cRect, "", os::CF_FOLLOW_NONE);
		pcView->AddChild(m_pcDDMenu);

		for (register char **ppzStart = g_apzAttrType, **ppzEnd = g_apzAttrType + ATTR_TYPE_COUNT; ppzStart < ppzEnd; ppzStart++)
			m_pcDDMenu->AppendItem(*ppzStart);
		m_pcDDMenu->SetSelection(g_sPreferences.m_nAttrType);
		m_pcDDMenu->SetReadOnly();

		cRect.top = cRect.bottom + 10.0f;
		cRect.bottom += 30.0f;
		cRect.right = 5.0f;

		for (register int i = 0; i < ID_BUTTON_COUNT; i++) {
			cRect.left = cRect.right + 5.0f;
			cRect.right += 65.0f;
			m_apcButton[i] = new os::Button(cRect, "", g_apzAddButton[i], new os::Message(i), os::CF_FOLLOW_NONE);
			pcView->AddChild(m_apcButton[i]);
		}

		SetDefaultButton(m_apcButton[ID_NEXT]);
	}

	// ------------------------------------------------------- //
	// HandleMessage
	// ------------------------------------------------------- //
	void AddWindow::HandleMessage(os::Message *pcMsg)
	{
		switch (pcMsg->GetCode())
		{
			case ID_NEXT:
			{
				register int nAttrType = m_pcDDMenu->GetSelection();
				g_sPreferences.m_nAttrType = nAttrType;
				register os::Window *pcWnd = new EditWindow(m_pcParent, nAttrType);
				m_pcParent->Lock();
				m_pcParent->m_pcAddEditWindow = pcWnd;
				m_pcParent->Unlock();
				pcWnd->CenterInWindow(this);
				pcWnd->Show();
				pcWnd->MakeFocus();
				m_nRetMainWindow = false;

				// don't add break here!
			}

			case ID_CANCEL:
				PostMessage(os::M_QUIT);
				break;

			default:
				os::Window::HandleMessage(pcMsg);
				break;
		}
	}

	// ------------------------------------------------------- //
	// Add dialog window destructor
	// ------------------------------------------------------- //
	AddWindow::~AddWindow()
	{
		if (m_nRetMainWindow)
		{
			m_pcParent->Lock();
			m_pcParent->m_pcAddEditWindow = NULL;
			m_pcParent->Unlock();
		}
	}

	// ------------------------------------------------------- //
	// Edit window constructor
	// ------------------------------------------------------- //
	EditWindow::EditWindow(MainWindow *pcParent, int nAttrType, uint nRow)
	 : os::Window(nAttrType == ATTR_TYPE_STRING ? os::Rect(0.0f, 0.0f, g_sPreferences.m_vEditX, g_sPreferences.m_vEditY) : os::Rect(0.0f, 0.0f, g_sPreferences.m_vEditXnormal, EWIND_Y_NORMAL), "edit_window", "Attribute properties", os::WND_NO_DEPTH_BUT | (nAttrType == ATTR_TYPE_STRING ? 0x00 : os::WND_NOT_V_RESIZABLE)),
	   m_pcParent(pcParent), m_pcRow(NULL), m_nIsFileReq(false), m_nAttrType(nAttrType), m_nErrCode(-1), m_pzFilePath(NULL), m_pzName(NULL), m_nRow(nRow)
	{
		SetIcon(g_apcBitmap[AEBITMAP_PROPERTIES24X24]);

		os::Rect cRect = GetBounds();
		register os::View *pcView = new os::View(cRect, "", os::CF_FOLLOW_ALL);
		AddChild(pcView);

		float vBottom = cRect.bottom - 40.0f;

		cRect.top = 10.0f;
		cRect.bottom = 30.0f;
		cRect.left = 10.0f;
		register float vXcoord = cRect.right - 10.0f;
		cRect.right = 55.0f;

		pcView->AddChild(new os::StringView(cRect, "", "Name:", os::ALIGN_LEFT, os::CF_FOLLOW_NONE));
		cRect.left = cRect.right;
		cRect.right = vXcoord;

		if (m_nRow != UINT_MAX) {
			m_pcParent->Lock();
			m_pcRow = (os::ListViewStringRow *)(m_pcParent->m_pcView->GetRow(m_nRow));
			m_pzName = strdup(m_pcRow->GetString(0).c_str());
			m_pcParent->Unlock();
		}

		m_pcName = new EtextView(cRect, "", m_pzName ? m_pzName : "", os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_LEFT);
		m_pcName->SetMaxUndoSize(0);
		pcView->AddChild(m_pcName);
	
		cRect.top = cRect.bottom + 10.0f;
		cRect.bottom += 30.0f;
		cRect.left = 10.0f;
		cRect.right = 55.0f;
		pcView->AddChild(new os::StringView(cRect, "", "Type:", os::ALIGN_LEFT, os::CF_FOLLOW_NONE));
		cRect.left = cRect.right;
		cRect.right = vXcoord;
		pcView->AddChild(new os::StringView(cRect, "", g_apzAttrType[nAttrType], os::ALIGN_LEFT, os::CF_FOLLOW_NONE));
		cRect.left = 10.0f;
		cRect.top = cRect.bottom + 10.0f;

		os::Point cPointMin(EWIND_X_NORMAL, EWIND_Y_NORMAL), cPointMax(os::COORD_MAX, EWIND_Y_NORMAL);
		register int i = 0;

		if (m_nAttrType == ATTR_TYPE_NONE)
		{
			m_pcLoadReq = new os::FileRequester(os::FileRequester::LOAD_REQ, new os::Messenger(this), NULL, os::FileRequester::NODE_FILE, false, new os::Message(ID_FILEREQ_LOAD), NULL, false, true, g_apzEditRawButton[ID_RAWBUTTON_LOAD]);
			m_pcLoadReq->SetIcon(g_apcBitmap[AEBITMAP_FOLDER24X24]);
			m_pcLoadReq->Start();
			m_pcDumpReq = new os::FileRequester(os::FileRequester::SAVE_REQ, new os::Messenger(this), NULL, os::FileRequester::NODE_FILE, false, new os::Message(ID_FILEREQ_DUMP), NULL, false, true, g_apzEditRawButton[ID_RAWBUTTON_DUMP]);
			m_pcDumpReq->SetIcon(g_apcBitmap[AEBITMAP_FOLDER24X24]);
			m_pcDumpReq->Start();

			cRect.bottom += 35.0f;
			for (; i < ID_RAWBUTTON_COUNT; i++) {
				cRect.right = cRect.left + 85.0f;
				m_apcRawButton[i] = new os::ImageButton(cRect, "", g_apzEditRawButton[i], new os::Message(ID_LOAD + i), new os::BitmapImage(*g_apcBitmapImg[i]), os::ImageButton::IB_TEXT_RIGHT, true, true, true, os::CF_FOLLOW_NONE);
				pcView->AddChild(m_apcRawButton[i]);
				cRect.left = cRect.right + 10.0f;
			}

			if (!m_pcRow)
				m_apcRawButton[ID_RAWBUTTON_DUMP]->SetEnable(false);

			cRect.left = 10.0f;
			cRect.right = vXcoord;
		}

		else
		{
			cRect.right = 55.0f;
			cRect.bottom = m_nAttrType == ATTR_TYPE_STRING ? vBottom : cRect.bottom + 30.0f;
			pcView->AddChild(new os::StringView(cRect, "", "Value:", os::ALIGN_LEFT, os::CF_FOLLOW_TOP | os::CF_FOLLOW_BOTTOM));
			cRect.left = cRect.right;
			cRect.right = vXcoord;
			m_pcValue = new EtextView(cRect, "", "", os::CF_FOLLOW_ALL);

			if (m_nAttrType == ATTR_TYPE_STRING) {
				m_pcValue->SetMultiLine();
				cPointMin.x = EWIND_X_EDIT;
				cPointMin.y = EWIND_Y_EDIT;
				cPointMax.y = os::COORD_MAX;
			}

			m_pcValue->SetMaxUndoSize(0);
			pcView->AddChild(m_pcValue);

			if (m_pcRow)
			{
				if (m_nAttrType == ATTR_TYPE_STRING)
				{
					struct attr_info sAttrInfo;
					if (stat_attr(g_hFd, m_pzName, &sAttrInfo) == 0)
					{
						if (sAttrInfo.ai_type == ATTR_TYPE_STRING)
						{
							register char *pzBuffer = (char *)malloc(sAttrInfo.ai_size + 1);
							if (pzBuffer) {
								int nLen = read_attr(g_hFd, m_pzName, ATTR_TYPE_STRING, pzBuffer, 0, sAttrInfo.ai_size);
								pzBuffer[nLen < 0 ? 0 : nLen] = '\0';
								m_pcValue->Set(pzBuffer);
								free(pzBuffer);
							}
							else
								m_nErrCode = ID_ERROR_MEMORYOUT;
						}
						else
							m_nErrCode = ID_ERROR_UNKNOWN;
					}
					else
						m_nErrCode = ID_ERROR_ATTRSTAT;
				}
				else {
					m_pcParent->Lock();
					m_pcValue->Set(m_pcRow->GetString(3).c_str());
					m_pcParent->Unlock();
				}
			}
		}

		cRect.top = cRect.bottom + 10.0f;
		cRect.bottom += 30.0f;

		for (i = 0; i < ID_BUTTON_COUNT; i++) {
			cRect.left = cRect.right - 65.0f;
			m_apcButton[i] = new os::Button(cRect, "", g_apzEditButton[i], new os::Message(i), os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM);
			pcView->AddChild(m_apcButton[i]);
			cRect.right = cRect.left - 5.0f;
		}

		SetFocusChild(m_apcButton[ID_CANCEL]);
		SetSizeLimits(cPointMin, cPointMax);
	}

	// ------------------------------------------------------- //
	// Getting error code
	// ------------------------------------------------------- //
	inline int EditWindow::GetErrorCode()
	{
		return m_nErrCode;
	}

	// ------------------------------------------------------- //
	// HandleMessage
	// ------------------------------------------------------- //
	void EditWindow::HandleMessage(os::Message *pcMsg)
	{
		register int nError = -1;
		switch (pcMsg->GetCode())
		{
			case ID_OK:
			{
				struct attr_info sAttrInfo;
				register const char *pzName = m_pcName->GetBuffer()[0].c_str();
				register bool nNameEqual = m_pzName && !strcmp(m_pzName, pzName);

				if (*pzName == '\0') {
					nError = ID_ERROR_ATTREMPTYNAME;
					break;
				}

				else if (nNameEqual || stat_attr(g_hFd, pzName, &sAttrInfo) != 0)
				{
					char pzValue[256];
					off_t nSize = 0;

					switch (m_nAttrType)
					{
						case ATTR_TYPE_NONE:
						{
							*pzValue = '\0';
							if (m_pzFilePath)
							{
								struct stat sFileInfo;
								register int hFd = open(m_pzFilePath, O_RDONLY | O_NOTRAVERSE);
								if (hFd >= 0) {
									if (fstat(hFd, &sFileInfo) == 0)
									{
										nSize = sFileInfo.st_size;
										register void *pBuffer = malloc(nSize);
										if (pBuffer)
										{
											if (read(hFd, pBuffer, nSize) == nSize)
											{
												write_attr(g_hFd, pzName, O_TRUNC, m_nAttrType, pBuffer, 0, nSize);
												free(m_pzFilePath);
												m_pzFilePath = NULL;
												m_apcRawButton[ID_RAWBUTTON_DUMP]->SetEnable(true);
											}
											else
												nError = ID_ERROR_FILEREAD;
											free(pBuffer);
										}
										else
											nError = ID_ERROR_MEMORYOUT;
									}

									else
										nError = ID_ERROR_FILESTAT;
									close(hFd);
								}
								else
									nError = ID_ERROR_FILENOTFOUND;
							}
							else {
								if (!m_pcRow)
									nError = ID_ERROR_LOADATTR;
								goto err;
							}

							if (nError >= 0)
								goto err;
							break;
						}

						case ATTR_TYPE_STRING:
						{
							register const char *pzVal = m_pcValue->GetValue().AsString().c_str();
							nSize = strlen(pzVal);
							write_attr(g_hFd, pzName, O_TRUNC, ATTR_TYPE_STRING, pzVal, 0, nSize);
							ConvertString(pzVal, pzValue);
							break;
						}

						default:
						{
							char *pzEnd;
							const char *pzVal = m_pcValue->GetBuffer()[0].c_str();
							switch (m_nAttrType)
							{
								case ATTR_TYPE_INT32:
								{
									int32 nVal = strtol(pzVal, &pzEnd, 10);
									if (errno != ERANGE && *pzVal != '\0' && *pzEnd == '\0') {
										nSize = sizeof(int32);
										write_attr(g_hFd, pzName, O_TRUNC, m_nAttrType, &nVal, 0, nSize);
										sprintf(pzValue, "%li", nVal);
									}
									else {
										nError = ID_ERROR_INCORNUM;
										goto err;
									}
									break;
								}

								case ATTR_TYPE_INT64:
								{
									int64 nVal = strtoll(pzVal, &pzEnd, 10);
									if (errno != ERANGE && *pzVal != '\0' && *pzEnd == '\0') {
										nSize = sizeof(int64);
										write_attr(g_hFd, pzName, O_TRUNC, m_nAttrType, &nVal, 0, nSize);
										sprintf(pzValue, "%lli", nVal);
									}
									else {
										nError = ID_ERROR_INCORNUM;
										goto err;
									}
									break;
								}

								case ATTR_TYPE_FLOAT:
								{
									float nVal = strtof(pzVal, &pzEnd);
									if (errno != ERANGE && *pzVal != '\0' && *pzEnd == '\0') {
										nSize = sizeof(float);
										write_attr(g_hFd, pzName, O_TRUNC, m_nAttrType, &nVal, 0, nSize);
										sprintf(pzValue, "%f", nVal);
									}
									else {
										nError = ID_ERROR_INCORNUM;
										goto err;
									}
									break;
								}

								default: // ATTR_TYPE_DOUBLE
								{
									double nVal = strtod(pzVal, &pzEnd);
									if (errno != ERANGE && *pzVal != '\0' && *pzEnd == '\0') {
										nSize = sizeof(double);
										write_attr(g_hFd, pzName, O_TRUNC, m_nAttrType, &nVal, 0, nSize);
										sprintf(pzValue, "%lf", nVal);
									}
									else {
										nError = ID_ERROR_INCORNUM;
										goto err;
									}
									break;
								}
							}
							break;
						}
					}

					if (m_pzName) {
						if (!nNameEqual)
							remove_attr(g_hFd, m_pzName);
						free(m_pzName);
					}

					m_pzName = strdup(pzName);

					char pzSize[21];
					sprintf(pzSize, "%lli", nSize);
					m_pcParent->Lock();
					if (m_pcRow) {
						register char *pzString;
						nSize -= strtoll(m_pcRow->GetString(2).c_str(), &pzString, 10);
						m_pcRow->SetString(0, m_pzName);
						m_pcRow->SetString(2, pzSize);
						m_pcRow->SetString(3, pzValue);
						m_pcParent->m_pcView->InvalidateRow(m_nRow, 0x00);
					}
					else {
						m_pcRow = new os::ListViewStringRow;
						m_pcRow->AppendString(m_pzName);
						m_pcRow->AppendString(g_apzAttrType[m_nAttrType]);
						m_pcRow->AppendString(pzSize);
						m_pcRow->AppendString(pzValue);
						m_nRow = m_pcParent->m_pcView->GetRowCount();
						m_pcParent->m_pcView->InsertRow(m_nRow, m_pcRow);
						m_pcParent->m_pcView->Select(m_nRow);
						g_sSummary.m_nTotalAttrCount++;
						g_sSummary.m_anAttrCount[m_nAttrType]++;
					}
					g_sSummary.m_nTotalAttrSize += nSize;
					m_pcParent->Unlock();
				}

				else
					nError = ID_ERROR_ATTRNAME;

				err: break;
			}

			case ID_CANCEL:
			{
				PostMessage(os::M_QUIT);
				break;
			}

			case ID_LOAD:
			{
				if (!m_nIsFileReq)
				{
					m_nIsFileReq = true;
					m_pcLoadReq->CenterInWindow(this);
					m_pcLoadReq->Show();
					m_pcLoadReq->MakeFocus();
				}
				break;
			}

			case ID_DUMP:
			{
				if (!m_nIsFileReq)
				{
					m_nIsFileReq = true;
					m_pcDumpReq->CenterInWindow(this);
					m_pcDumpReq->Show();
					m_pcDumpReq->MakeFocus();
				}
				break;
			}

			case ID_FILEREQ_LOAD:
			{
				const char *pzFilePath;
				if (pcMsg->FindString("file/path", &pzFilePath) == 0) {
					if (m_pzFilePath)
						free(m_pzFilePath);
					m_pzFilePath = strdup(pzFilePath);
					m_apcRawButton[ID_RAWBUTTON_DUMP]->SetEnable(false);
				}
				else
					nError = ID_ERROR_UNKNOWN;
				m_nIsFileReq = false;
				break;
			}

			case ID_FILEREQ_DUMP:
			{
				const char *pzFilePath;
				if (pcMsg->FindString("file/path", &pzFilePath) == 0)
				{
					register int hFd = unlink(pzFilePath); // try to unlink file
					if (hFd == 0 || errno == ENOENT)
						hFd = open(pzFilePath, O_RDWR | O_CREAT);

					if (hFd >= 0)
					{
						struct attr_info sAttrInfo;
						if (stat_attr(g_hFd, m_pzName, &sAttrInfo) == 0)
						{
							register void *pBuffer = malloc(sAttrInfo.ai_size);
							if (pBuffer) {
								if (read_attr(g_hFd, m_pzName, sAttrInfo.ai_type, pBuffer, 0, sAttrInfo.ai_size) == sAttrInfo.ai_size) {
									if (write(hFd, pBuffer, sAttrInfo.ai_size) != sAttrInfo.ai_size)
										nError = ID_ERROR_FILEWRITE;
								}
								else
									nError = ID_ERROR_ATTRREAD;
								free(pBuffer);
							}
							else
								nError = ID_ERROR_MEMORYOUT;
						}
						else
							nError = ID_ERROR_ATTRSTAT;
						close(hFd);
					}
					else
						nError = ID_ERROR_FILENOTFOUND;
				}
				else
					nError = ID_ERROR_UNKNOWN;
				m_nIsFileReq = false;
				break;
			}

			case os::M_FILE_REQUESTER_CANCELED:
				m_nIsFileReq = false;
				break;

			default:
				os::Window::HandleMessage(pcMsg);
				break;
		}

		if (nError >= 0) {
			m_nErrCode = nError;
			ShowError(this, nError);
		}
	}

	// ------------------------------------------------------- //
	// EditWindow destructor
	// ------------------------------------------------------- //
	EditWindow::~EditWindow()
	{
		if (m_pzName)
			free(m_pzName);

		if (m_pzFilePath)
			free(m_pzFilePath);

		os::Rect cRect = GetBounds();
		m_pcParent->Lock();

		switch (m_nAttrType)
		{
			case ATTR_TYPE_STRING:
			{
				g_sPreferences.m_vEditX = cRect.right;
				g_sPreferences.m_vEditY = cRect.bottom;
				break;
			}

			case ATTR_TYPE_NONE:
			{
				m_pcLoadReq->Close();
				m_pcDumpReq->Close();
				// don't add break here
			}

			default:
				g_sPreferences.m_vEditXnormal = cRect.right;
		}

		m_pcParent->m_pcAddEditWindow = NULL;
		m_pcParent->Unlock();
	}

	// ------------------------------------------------------- //
	// MainWindow constructor
	// ------------------------------------------------------- //
	MainWindow::MainWindow(const os::Rect &cFrame)
	  : os::Window(cFrame, "main_window", "AttribEdit"),
	    m_nIsFileReq(false), m_pcAlert(NULL), m_pcPrefsWindow(NULL), m_pcAddEditWindow(NULL)
	{
		os::Rect cViewFrame = GetBounds();
		os::Menu *pcMenu;
		os::MenuItem *pcMenuItem;
		struct g_sAEMenuItem *psMenuItem = g_asAEMenuItem;
		char **ppzTmpMenu = g_azMenu;
		os::ResStream *pcResStream;
		os::Resources cAEResources(get_image_id());
		int i = 0;

		SetSizeLimits(os::Point(MWIND_RIGHT - MWIND_LEFT, MWIND_BOTTOM - MWIND_TOP), os::Point(os::COORD_MAX, os::COORD_MAX));

		cViewFrame.top = 18;
		m_pcMenu = new os::Menu(os::Rect(cViewFrame.left, 0, cViewFrame.right, cViewFrame.top), "", os::ITEMS_IN_ROW);

		// add menu items
		for (; *ppzTmpMenu; ppzTmpMenu++)
		{
			pcMenu = new os::Menu(os::Rect(0, 0, 1, 1), *ppzTmpMenu, os::ITEMS_IN_COLUMN);
			for (; psMenuItem->m_pzName; psMenuItem++)
			{
				switch(*(psMenuItem->m_pzName))
				{
					case NULL:
						pcMenuItem = new os::MenuSeparator;
						break;
					default:
					{
						pcResStream = psMenuItem->m_pzIconPath ? cAEResources.GetResourceStream(psMenuItem->m_pzIconPath) : NULL;
						pcMenuItem = new os::MenuItem(psMenuItem->m_pzName, new os::Message(i), psMenuItem->m_pzShortCut, pcResStream ? new os::BitmapImage(pcResStream) : NULL);
						if (pcResStream)
							delete pcResStream;
						g_apcMenuItem[i++] = pcMenuItem;
						break;
					}
				}
				pcMenu->AddItem(pcMenuItem);
			}
			psMenuItem++;
			m_pcMenu->AddItem(pcMenu);
		}

		m_pcMenu->SetTargetForItems(this);
		AddChild(m_pcMenu);

		m_pcView = new MainView(cViewFrame);
		m_pcView->SetInvokeMsg(new os::Message(ID_EDIT_PROPERTIES));
		AddChild(m_pcView);

		// set window icon
		pcResStream = cAEResources.GetResourceStream("icon24x24.png");
		os::BitmapImage *pcBitmapImage = new os::BitmapImage(pcResStream);
		delete pcResStream;
		SetIcon(pcBitmapImage->LockBitmap());
		delete pcBitmapImage;

		// getting bitmaps
		for (i = 0; i < AEBITMAP_COUNT; i++) {
			pcResStream = cAEResources.GetResourceStream(g_apzAE_Bitmap[i]);
			pcBitmapImage = new os::BitmapImage(pcResStream);
			delete pcResStream;
			g_apcBitmap[i] = CopyBitmap(pcBitmapImage->LockBitmap());
			delete pcBitmapImage;
		}

		// getting bitmap images
		for (i = 0; i < AEBITMAPIMG_COUNT; i++) {
			pcResStream = cAEResources.GetResourceStream(g_apzAE_BitmapImg[i]);
			g_apcBitmapImg[i] = new os::BitmapImage(pcResStream);
			delete pcResStream;
		}

		m_pcOpenReq = new os::FileRequester(os::FileRequester::LOAD_REQ, new os::Messenger(this), "", os::FileRequester::NODE_FILE | os::FileRequester::NODE_DIR, false, new os::Message(ID_FILEREQ_OPEN), NULL, false, true, "Select");
		m_pcOpenReq->SetIcon(g_apcBitmap[AEBITMAP_FOLDER24X24]);
		m_pcOpenReq->Start();
		m_pcCopyReq = new os::FileRequester(os::FileRequester::LOAD_REQ, new os::Messenger(this), "", os::FileRequester::NODE_FILE | os::FileRequester::NODE_DIR, false, new os::Message(ID_FILEREQ_COPY), NULL, false, true, "Copy");
		m_pcCopyReq->SetIcon(g_apcBitmap[AEBITMAP_FOLDER24X24]);
		m_pcCopyReq->Start();
		m_pcNewReq = new os::FileRequester(os::FileRequester::SAVE_REQ, new os::Messenger(this), "", os::FileRequester::NODE_FILE, false, new os::Message(ID_FILEREQ_NEW), NULL, false, true, "Create");
		m_pcNewReq->SetIcon(g_apcBitmap[AEBITMAP_FOLDER24X24]);
		m_pcNewReq->Start();

		ManageLock(); 
	}

	// ------------------------------------------------------- //
	// Removing all attributes
	// ------------------------------------------------------- //
	void MainWindow::RemoveAll()
	{
		register DIR *pDir;
		struct dirent *psDirEntry;

		if ((pDir = open_attrdir(g_hFd)) == NULL)
			ShowError(this, ID_ERROR_OPENATTRDIR);
		else
		{
			while ((psDirEntry = read_attrdir(pDir)) != NULL) {
				remove_attr(g_hFd, psDirEntry->d_name);
				rewind_attrdir(pDir);
			}
			close_attrdir(pDir);
			memset(&g_sSummary, 0, sizeof(g_sSummaryType));
			m_pcView->Clear();
		}
	}

	// ------------------------------------------------------- //
	// "Open file" function (with flags)
	// ------------------------------------------------------- //
	void MainWindow::FileOpen(const char *zFilePath, int nOpenFlags, bool nUnlink)
	{
		register DIR *pDir;
		register int hFd = 0;
		register int nError = -1;
		struct dirent *psDirEntry;
		struct attr_info sAttrInfo;
		register char *pzString;
		char zBuffer[256];
		os::ListViewStringRow *pcRow;

		std::string cWindowTitle(MWIND_TITLE ": ");

		if (nUnlink)
			hFd = unlink(zFilePath);

		if (hFd == 0 || errno == ENOENT)
			hFd = open(zFilePath, nOpenFlags);

		if (hFd >= 0)
		{
			if ((pDir = open_attrdir(hFd)) == NULL) {
				close(hFd);
				nError = ID_ERROR_OPENATTRDIR;
			}
			else
			{
				CloseFile();
				g_hFd = hFd;
				ManageLock(false);
				cWindowTitle += zFilePath;
				SetTitle(cWindowTitle);

				while ((psDirEntry = read_attrdir(pDir)) != NULL)
				{
					if (stat_attr(hFd, psDirEntry->d_name, &sAttrInfo) == 0)
					{
						pcRow = new os::ListViewStringRow;
						pcRow->AppendString(psDirEntry->d_name);

						if ((sAttrInfo.ai_type >= ATTR_TYPE_NONE) && (sAttrInfo.ai_type < ATTR_TYPE_COUNT))
						{
							pzString = g_apzAttrType[sAttrInfo.ai_type];
							g_sSummary.m_anAttrCount[sAttrInfo.ai_type]++;
						}
						else {
							sprintf(zBuffer, "unknown: %i", sAttrInfo.ai_type);
							pzString = zBuffer;
							g_sSummary.m_anAttrCount[ATTR_TYPE_COUNT]++;
						}

						pcRow->AppendString(pzString);

						g_sSummary.m_nTotalAttrSize += sAttrInfo.ai_size;
						sprintf(zBuffer, "%lli", sAttrInfo.ai_size);
						pcRow->AppendString(zBuffer);

						switch (sAttrInfo.ai_type)
						{
							case ATTR_TYPE_INT32:
							{
								int32 nVal;
								read_attr(hFd, psDirEntry->d_name, sAttrInfo.ai_type, &nVal, 0, sizeof(int32));
								sprintf(zBuffer, "%li", nVal);
								break;
							}

							case ATTR_TYPE_INT64:
							{
								int64 nVal;
								read_attr(hFd, psDirEntry->d_name, sAttrInfo.ai_type, &nVal, 0, sizeof(int64));
								sprintf(zBuffer, "%lli", nVal);
								break;
							}

							case ATTR_TYPE_FLOAT:
							{
								float vVal;
								read_attr(hFd, psDirEntry->d_name, sAttrInfo.ai_type, &vVal, 0, sizeof(float));
								sprintf(zBuffer, "%f", vVal);
								break;
							}

							case ATTR_TYPE_DOUBLE:
							{
								double vVal;
								read_attr(hFd, psDirEntry->d_name, sAttrInfo.ai_type, &vVal, 0, sizeof(double));
								sprintf(zBuffer, "%lf", vVal);
								break;
							}

							case ATTR_TYPE_STRING:
							{
								char pzTmpBuffer[256];
								register int nLen = read_attr(hFd, psDirEntry->d_name, sAttrInfo.ai_type, pzTmpBuffer, 0, 255);
								pzTmpBuffer[nLen < 0 ? 0 : nLen] = '\0';
								ConvertString(pzTmpBuffer, zBuffer);
								break;
							}

							default: // ATTR_TYPE_NONE and unknown attributes
								*zBuffer = '\0';
								break;
						}

						g_sSummary.m_nTotalAttrCount++;
						pcRow->AppendString(zBuffer);
						m_pcView->InsertRow(pcRow);
					}
				}
				close_attrdir(pDir);
			}
		}
		else
			nError = ID_ERROR_FILENOTFOUND;

		if (nError >= 0)
			ShowError(this, nError);
	}

	// ------------------------------------------------------- //
	// "Copy all attributes" function (with flags)
	// ------------------------------------------------------- //
	void MainWindow::FileCopyAll(const char *pzFilePath, int nOpenFlags)
	{
		struct dirent *psDirEntry;
		struct attr_info sAttrInfo, sAttrCopyInfo;
		register int hFd, nError = -1;
		register DIR *pDir;

		if ((hFd = open(pzFilePath, nOpenFlags)) >= 0)
		{
			if ((pDir = open_attrdir(g_hFd)) == NULL)
				nError = ID_ERROR_OPENATTRDIR;
			else
			{
				register void *pBuffer = malloc(65536);
				register void *pTmp;
				if (pBuffer) {
					while ((psDirEntry = read_attrdir(pDir)) != NULL)
					{
						if (stat_attr(g_hFd, psDirEntry->d_name, &sAttrInfo) == 0)
						{
							if (sAttrInfo.ai_size > 65536) {
								pTmp = pBuffer;
								pBuffer = realloc(pBuffer, sAttrInfo.ai_size);
								if (!pBuffer) {
									free(pTmp);
									goto mem_out;
								}
							}

							if (read_attr(g_hFd, psDirEntry->d_name, sAttrInfo.ai_type, pBuffer, 0, sAttrInfo.ai_size) == sAttrInfo.ai_size)
							{
								if ((g_sPreferences.m_nPrefs & PREFS_SAVEAS_TRUNC) || (stat_attr(hFd, psDirEntry->d_name, &sAttrCopyInfo) != 0))
									if (write_attr(hFd, psDirEntry->d_name, O_TRUNC, sAttrInfo.ai_type, pBuffer, 0, sAttrInfo.ai_size) < 0)
									{
										nError = ID_ERROR_ATTRWRITE;
										break;
									}

								if (sAttrInfo.ai_size > 65536) {
									pTmp = pBuffer;
									pBuffer = realloc(pBuffer, 65536);
									if (!pBuffer) {
										free(pTmp);
										goto mem_out;
									}
								}
							}
							else {
								nError = ID_ERROR_ATTRREAD;
								break;
							}
						}
						else {
							nError = ID_ERROR_ATTRSTAT;
							break;
						}
					}
					free(pBuffer);
				}
				else
					mem_out: nError = ID_ERROR_MEMORYOUT;
				close_attrdir(pDir);
			}
			close(hFd);
		}
		else
			nError = ID_ERROR_FILENOTFOUND;

		if (nError >= 0) {
			ShowError(this, ID_ERROR_COPYING);
			ShowError(this, nError);
		}
	}

	// ------------------------------------------------------- //
	// "Copy to" function (with flags)
	// ------------------------------------------------------- //
	void MainWindow::FileCopy(const char *pzFilePath, int nOpenFlags)
	{
		register int nError = -1;
		register os::ListViewStringRow *pcRow = (os::ListViewStringRow *)m_pcView->GetRow(m_nCopyRow);
		if (pcRow)
		{
			register int hFd = open(pzFilePath, nOpenFlags);
			if (hFd >= 0)
			{
				const char *pzName = pcRow->GetString(0).c_str();
				struct attr_info sAttrInfo, sAttrCopyInfo;
				if (stat_attr(g_hFd, pzName, &sAttrInfo) == 0)
				{
					void *pBuffer = malloc(sAttrInfo.ai_size);
					if (pBuffer) {
						if (read_attr(g_hFd, pzName, sAttrInfo.ai_type, pBuffer, 0, sAttrInfo.ai_size) == sAttrInfo.ai_size)
						{
							if ((g_sPreferences.m_nPrefs & PREFS_SAVEAS_TRUNC) || (stat_attr(hFd, pzName, &sAttrCopyInfo) != 0))
							{
								if (write_attr(hFd, pzName, O_TRUNC, sAttrInfo.ai_type, pBuffer, 0, sAttrInfo.ai_size) < 0)
									nError = ID_ERROR_ATTRWRITE;
							}
							else
								nError = ID_ERROR_ATTRNAME;
						}
						else
							nError = ID_ERROR_ATTRREAD;
						free(pBuffer);
					}
					else
						nError = ID_ERROR_MEMORYOUT;
				}
				else
					nError = ID_ERROR_ATTRSTAT;
				close(hFd);
			}
			else
				nError = ID_ERROR_FILENOTFOUND;
		}
		else
			nError = ID_ERROR_UNKNOWN;

		if (nError >= 0)
			ShowError(this, nError);
	}

	// ------------------------------------------------------- //
	// "Open file" function for all available modes
	// ------------------------------------------------------- //
	void MainWindow::Open(const char *pzFilePath, int nMode)
	{
		m_nIsFileReq = true;
		register int nOpenFlags = O_RDWR;
		register int nPower = 2, nMessage = ID_SYMWIND_OPEN, nError = -1;

		switch (nMode) {
			case ID_MODE_OPEN:
				nPower = 1;
				break;

			case ID_MODE_COPYALL:
				nMessage = ID_SYMWIND_COPYALL;
				break;

			default:
				nMessage = ID_SYMWIND_COPY;
		}

		if (g_sPreferences.m_nPrefs & (PREFS_SLNK_MANUAL * nPower))
		{
			struct stat sStat;
			int hFd;
			if ((hFd = open(pzFilePath, O_RDWR | O_NOTRAVERSE)) >= 0)
			{
				if (fstat(hFd, &sStat) == 0)
				{
					int nRes = S_ISLNK(sStat.st_mode);
					close(hFd);
					if (nRes)
					{
						SlnkWindow *pcWind = new SlnkWindow(this, pzFilePath, new os::Message(nMessage));
						pcWind->CenterInWindow(this);
						pcWind->Show();
						pcWind->MakeFocus();
						return;
					}
					else
						goto op;
				}
				else {
					close(hFd);
					nError = ID_ERROR_FILESTAT;
				}
			}
			else
				nError = ID_ERROR_FILENOTFOUND;
		}
		else {
			if (g_sPreferences.m_nPrefs & (PREFS_SLNK_MODE * nPower))
				nOpenFlags |= O_NOTRAVERSE;
			op: switch (nMode) {
					case ID_MODE_OPEN: // File->Open
						FileOpen(pzFilePath, nOpenFlags);
						break;

					case ID_MODE_COPYALL: // File->Copy all attributes
						FileCopyAll(pzFilePath, nOpenFlags);
						break;

					default: // Edit->Copy to
						FileCopy(pzFilePath, nOpenFlags);
				}
		}

		m_nIsFileReq = false;

		if (nError >= 0)
			ShowError(this, nError);
	}

	// ------------------------------------------------------- //
	// Close file
	// ------------------------------------------------------- //
	void MainWindow::CloseFile()
	{
		if (g_hFd >= 0)
		{
			close(g_hFd);
			g_hFd = -1;
			memset(&g_sSummary, 0, sizeof(g_sSummaryType));
			SetTitle(MWIND_TITLE);
			m_pcView->Clear();
			ManageLock();
		}
	}

	// ------------------------------------------------------- //
	// Lock attributes management
	// ------------------------------------------------------- //
	void MainWindow::ManageLock(bool nValue)
	{
		nValue = !nValue;
		g_apcMenuItem[ID_FILE_CLOSE]->SetEnable(nValue);
		g_apcMenuItem[ID_FILE_SUMMARY]->SetEnable(nValue);
		g_apcMenuItem[ID_EDIT_ADD]->SetEnable(nValue);
	}

	// ------------------------------------------------------- //
	// virtual method: HandleMessage
	// ------------------------------------------------------- //
	void MainWindow::HandleMessage(os::Message *pcMsg)
	{
		register int nError = -1;

		switch (pcMsg->GetCode())
		{
			case ID_EDIT_PROPERTIES:
			{
				if (m_nIsFileReq)
					nError = ID_ERROR_FILEREQ;
				else if (m_pcAlert)
					nError = ID_ERROR_DIALOG;
				else if (!m_pcAddEditWindow)
				{
					register uint nRow = m_pcView->GetSelectedRow();
					if (nRow == UINT_MAX)
						nError = ID_ERROR_SELECTATTR;
					else
					{
						register os::ListViewStringRow *pcRow = (os::ListViewStringRow *)m_pcView->GetRow(nRow);
						register int nAttrType = GetAttribType(pcRow->GetString(1).c_str());
						if (nAttrType == ATTR_TYPE_COUNT)
							nError = ID_ERROR_UNKNOWNATTR;
						else {
							m_pcAddEditWindow = new EditWindow(this, nAttrType, nRow);
							if ((nError = ((EditWindow *)m_pcAddEditWindow)->GetErrorCode()) < 0) {
								m_pcAddEditWindow->CenterInWindow(this);
								m_pcAddEditWindow->Show();
								m_pcAddEditWindow->MakeFocus();
							}
							else
								m_pcAddEditWindow->Close();
						}
					}
				}
				break;
			}

			case ID_EDIT_ADD:
			{
				if (m_nIsFileReq)
					nError = ID_ERROR_FILEREQ;
				else if (m_pcAlert)
					nError = ID_ERROR_DIALOG;
				else if (!m_pcAddEditWindow)
				{
					m_pcAddEditWindow = new AddWindow(this);
					m_pcAddEditWindow->CenterInWindow(this);
					m_pcAddEditWindow->Show();
					m_pcAddEditWindow->MakeFocus();
				}
				break;
			}

			case ID_FILE_SUMMARY:
			{
				SummaryWindow *pcWindow = new SummaryWindow;
				pcWindow->CenterInWindow(this);
				pcWindow->Show();
				pcWindow->MakeFocus();
				break;
			}

			case ID_EDIT_REMOVE:
			{
				if (m_pcAddEditWindow)
					nError = ID_ERROR_EDIT;
				else if (m_pcAlert)
					nError = ID_ERROR_DIALOG;
				else if (m_nIsFileReq)
					nError = ID_ERROR_FILEREQ;
				else
				{
					register uint nCurRow = m_pcView->GetSelectedRow();
					if (nCurRow == UINT_MAX)
						nError = ID_ERROR_SELECTATTR;
					else
					{
						RemoveChild(m_pcMenu);
						register os::ListViewStringRow *pcRow = (os::ListViewStringRow *)m_pcView->RemoveRow(nCurRow);
						g_sSummary.m_nTotalAttrCount--;
						register char *pzString;
						g_sSummary.m_nTotalAttrSize -= strtoll(pcRow->GetString(2).c_str(), &pzString, 10);
						g_sSummary.m_anAttrCount[GetAttribType(pcRow->GetString(1).c_str())]--;
						remove_attr(g_hFd, pcRow->GetString(0).c_str());
						delete pcRow;
						AddChild(m_pcMenu);
					}
				}
				break;
			}

			case ID_EDIT_COPYTO:
			{
				if (m_pcAddEditWindow)
					nError = ID_ERROR_EDIT;
				else if (m_pcAlert)
					nError = ID_ERROR_DIALOG;
				else if (m_nIsFileReq)
					nError = ID_ERROR_FILEREQ;
				else
				{
					m_nCopyRow = m_pcView->GetSelectedRow();
					if (m_nCopyRow == UINT_MAX)
						nError = ID_ERROR_SELECTATTR;
					else {
						m_nIsFileReq = true;
						m_nCopyMode = ID_MODE_COPY;
						m_pcCopyReq->CenterInWindow(this);
						m_pcCopyReq->Show();
						m_pcCopyReq->MakeFocus();
					}
				}
				break;
			}

			case ID_FILE_REMOVEALL:
			{
				if (m_pcAddEditWindow)
					nError = ID_ERROR_EDIT;
				else if (m_nIsFileReq)
					nError = ID_ERROR_FILEREQ;
				else if (!m_pcAlert)
				{
					m_pcAlert = new os::Alert("Removing all attributes", "Are you really want to remove all\nattributes from a selected file?", os::Alert::ALERT_QUESTION, 0x00, "Yes", "No", NULL);
					m_pcAlert->SetIcon(g_apcBitmap[AEBITMAP_QUESTION24X24]);
					m_pcAlert->CenterInWindow(this);
					m_pcAlert->Go(new os::Invoker(new os::Message(ID_REMOVEALL_ANSWERED), (os::Handler *)this));
					m_pcAlert->MakeFocus();
				}
				break;
			}

			case ID_REMOVEALL_ANSWERED:
			{
				int32 nRes;
				if (pcMsg->FindInt32("which", &nRes) == 0) {
					if (nRes == 0) // "Yes" was selected
						RemoveAll();
				}
				else
					nError = ID_ERROR_UNKNOWN;
				m_pcAlert = NULL;
				break;
			}

			case ID_FILE_NEW:
			{
				if (m_pcAddEditWindow)
					nError = ID_ERROR_EDIT;
				else if (m_pcAlert)
					nError = ID_ERROR_DIALOG;
				else if (!m_nIsFileReq)
				{
					m_nIsFileReq = true;
					m_pcNewReq->CenterInWindow(this);
					m_pcNewReq->Show();
					m_pcNewReq->MakeFocus();
				}
				break;
			}

			case ID_FILE_OPEN:
			{
				if (m_pcAddEditWindow)
					nError = ID_ERROR_EDIT;
				else if (m_pcAlert)
					nError = ID_ERROR_DIALOG;
				else if (!m_nIsFileReq)
				{	
					m_nIsFileReq = true;
					m_pcOpenReq->CenterInWindow(this);
					m_pcOpenReq->Show();
					m_pcOpenReq->MakeFocus();
				}
				break;
			}

			case ID_FILE_CLOSE:
			{
				if (m_pcAddEditWindow)
					nError = ID_ERROR_EDIT;
				else if (m_pcAlert)
					nError = ID_ERROR_DIALOG;
				else if (m_nIsFileReq)
					nError = ID_ERROR_FILEREQ;
				else
				{
					RemoveChild(m_pcMenu);
					CloseFile();
					AddChild(m_pcMenu);
				}
				break;
			}

			case ID_FILE_COPYALL:
			{
				if (m_pcAddEditWindow)
					nError = ID_ERROR_EDIT;
				else if (m_pcAlert)
					nError = ID_ERROR_DIALOG;
				else if (!m_nIsFileReq)
				{
					m_nIsFileReq = true;
					m_nCopyMode = ID_MODE_COPYALL;
					m_pcCopyReq->CenterInWindow(this);
					m_pcCopyReq->Show();
					m_pcCopyReq->MakeFocus();
				}
				break;
			}

			case ID_FILEREQ_OPEN:
			{
				register const char *zFilePath;
				if (pcMsg->FindString("file/path", &zFilePath) == 0)
					Open(zFilePath, ID_MODE_OPEN);
				else {
					nError = ID_ERROR_UNKNOWN;
					m_nIsFileReq = false;
				}
				break;
			}

			case ID_FILEREQ_NEW:
			{
				register const char *pzFilePath;
				if (pcMsg->FindString("file/path", &pzFilePath) == 0)
					FileOpen(pzFilePath, O_RDWR | O_CREAT, true);
				else
					nError = ID_ERROR_UNKNOWN;
				m_nIsFileReq = false;
				break;
			}

			case ID_FILEREQ_COPY:
			{
				register const char *pzFilePath;
				if (pcMsg->FindString("file/path", &pzFilePath) == 0)
					Open(pzFilePath, m_nCopyMode);
				else {
					nError = ID_ERROR_UNKNOWN;
					m_nIsFileReq = false;
				}
				break;
			}

			case os::M_FILE_REQUESTER_CANCELED:
				m_nIsFileReq = false;
				break;

			case ID_SYMWIND_OPEN:
			{
				register const char *pzFilePath;
				register int nOpenFlags;
				if (!(pcMsg->FindString("file/path", &pzFilePath)) && !(pcMsg->FindInt32("file/mode", (int32 *)(&nOpenFlags))))
					FileOpen(pzFilePath, nOpenFlags);
				else
					nError = ID_ERROR_UNKNOWN;
				m_nIsFileReq = false;
				break;
			}

			case ID_SYMWIND_COPYALL:
			{
				register const char *pzFilePath;
				register int nOpenFlags;
				if (!(pcMsg->FindString("file/path", &pzFilePath)) && !(pcMsg->FindInt32("file/mode", (int32 *)(&nOpenFlags))))
					FileCopyAll(pzFilePath, nOpenFlags);
				else
					nError = ID_ERROR_UNKNOWN;
				m_nIsFileReq = false;
				break;
			}

			case ID_SYMWIND_COPY:
			{
				register const char *pzFilePath;
				register int nOpenFlags;
				if (!(pcMsg->FindString("file/path", &pzFilePath)) && !(pcMsg->FindInt32("file/mode", (int32 *)(&nOpenFlags))))
					FileCopy(pzFilePath, nOpenFlags);
				else
					nError = ID_ERROR_UNKNOWN;
				m_nIsFileReq = false;
				break;
			}

			case ID_APPLICATION_ABOUT:
			{
				os::Alert *pcAboutWindow = new os::Alert("About AttribEdit", "AttribEdit " AE_VERSION "\n\nAttributes editor for Syllable\nCopyright (c) 2004 Ruslan Nickolaev\nE-mail: nruslan@hotbox.ru\n\nAttribEdit is released under the GNU General\nPublic License 2.0. Please see the file COPYING,\ndistributed with AttribEdit, or http://www.gnu.org\nfor more information", g_apcBitmap[AEBITMAP_ABOUT32X32], 0x00, OK_BUTTON, NULL);
				pcAboutWindow->SetIcon(g_apcBitmap[AEBITMAP_ABOUT24X24]);
				pcAboutWindow->CenterInWindow(this);
				pcAboutWindow->Go(new os::Invoker);
				break;
			}

			case ID_FILE_EXIT:
				PostMessage(os::M_QUIT);
				break;

			case ID_APPLICATION_PREFERENCES:
			{
				if (!m_pcPrefsWindow)
				{
					m_pcPrefsWindow = new PrefsWindow(this);
					m_pcPrefsWindow->CenterInWindow(this);
					m_pcPrefsWindow->Show();
					m_pcPrefsWindow->MakeFocus();
				}
				break;
			}

			default:
				os::Window::HandleMessage(pcMsg);
				break;
		}

		if (nError >= 0)
			ShowError(this, nError);
	}

	// ------------------------------------------------------- //
	// virtual method: OkToQuit
	// ------------------------------------------------------- //
	bool MainWindow::OkToQuit()
	{
		os::Application::GetInstance()->PostMessage(os::M_QUIT);
		return false;
	}

	// ------------------------------------------------------- //
	// MainWindow destructor
	// ------------------------------------------------------- //
	MainWindow::~MainWindow()
	{
		if (g_hFd >= 0)
			close(g_hFd);

		m_pcOpenReq->Close();
		m_pcCopyReq->Close();
		m_pcNewReq->Close();

		os::Rect cWindowFrame = GetFrame();

		register int hDirFd = open(getenv("HOME"), O_RDONLY);
		register int hFd = based_open(hDirFd, AE_CONFIG, O_CREAT | O_TRUNC);
		close(hDirFd);

		if (hFd >= 0) {
			write(hFd, &cWindowFrame, sizeof(cWindowFrame));
			write(hFd, &g_sPreferences, sizeof(g_sPreferences));
			close(hFd);
		}
		else
			std::cerr << "Error writing config file!" << std::endl;

		for (register os::Bitmap **ppcBitmap = g_apcBitmap + AEBITMAP_COUNT - 1; ppcBitmap >= g_apcBitmap; ppcBitmap--)
			delete *ppcBitmap;

		for (register os::BitmapImage **ppcBitmapImg = g_apcBitmapImg + AEBITMAPIMG_COUNT - 1; ppcBitmapImg >= g_apcBitmapImg; ppcBitmapImg--)
			delete *ppcBitmapImg;
	}

	// ------------------------------------------------------- //
	// AttribEdit constructor
	// ------------------------------------------------------- //
	AttribEdit::AttribEdit(const char *zFilePath)
		: os::Application("application/x-vnd.syllable-AttribEdit")
	{
		register int hDirFd, hFd;
		struct stat sFileInfo;
		os::Rect cWindowFrame(MWIND_LEFT, MWIND_TOP, MWIND_RIGHT, MWIND_BOTTOM);

		// open config file
		hDirFd = open(getenv("HOME"), O_RDONLY);
		hFd = based_open(hDirFd, AE_CONFIG, O_RDONLY);
		close(hDirFd);

		if (fstat(hFd, &sFileInfo) == 0 && sFileInfo.st_size == (sizeof(cWindowFrame) + sizeof(g_sPreferences))) {
			read(hFd, &cWindowFrame, sizeof(cWindowFrame));
			read(hFd, &g_sPreferences, sizeof(g_sPreferences));
			close(hFd);
		}

		//else
		//	std::cerr << "Config file not found or incorrect!" << std::endl;

		m_pcWind = new MainWindow(cWindowFrame);
		m_pcWind->Show();
		m_pcWind->MakeFocus();

		if (zFilePath)
			m_pcWind->Open(zFilePath, ID_MODE_OPEN);
	}

	// ------------------------------------------------------- //
	// virtual method: OkToQuit
	// ------------------------------------------------------- //
	bool AttribEdit::OkToQuit()
	{
		m_pcWind->Close();
		return true;
	}

	// ------------------------------------------------------- //
	// AttribEdit destructor
	// ------------------------------------------------------- //
	AttribEdit::~AttribEdit()
	{
	}
}

// ------------------------------------------------------ //
// Copy bitmap function
// ------------------------------------------------------ //
os::Bitmap *CopyBitmap(os::Bitmap *pcSrcIcon)
{
    os::Bitmap *pcDestIcon = new os::Bitmap((int)pcSrcIcon->GetBounds().Width() + 1,
      (int)pcSrcIcon->GetBounds().Height() + 1, pcSrcIcon->GetColorSpace(), os::Bitmap::SHARE_FRAMEBUFFER);
    memcpy(pcDestIcon->LockRaster(), pcSrcIcon->LockRaster(), pcSrcIcon->GetBytesPerRow() * (int)(pcSrcIcon->GetBounds().Height() + 1));
    return pcDestIcon;
}

// ------------------------------------------------------- //
// Show window with error message
// ------------------------------------------------------- //
void ShowError(os::Window *pcWnd, int nErrorCode)
{
	os::Alert *pcErrorWindow = new os::Alert("Error", g_azMainWindowError[nErrorCode], CopyBitmap(g_apcBitmap[AEBITMAP_ERROR32X32]), 0x00, OK_BUTTON, NULL);
	pcErrorWindow->SetIcon(g_apcBitmap[AEBITMAP_ERROR24X24]);
	pcErrorWindow->CenterInWindow(pcWnd);
	pcErrorWindow->Go(new os::Invoker);
}

// ------------------------------------------------------------- //
// Getting attribute type
// ------------------------------------------------------------- //
int GetAttribType(const char *pzString)
{
	register int i = 0;
	for (; i < ATTR_TYPE_COUNT; i++)
		if (!strcmp(pzString, g_apzAttrType[i]))
			break;
	return i;
}

// ------------------------------------------------------------- //
// Converting string: '\n' -> "\n"; '\t' -> "\n"; '\\' -> "\\";
// ------------------------------------------------------------- //
void ConvertString(const char *pzBufferOld, char *pzBufferNew)
{
	register uint i = 0;
	register char *pzBuffer = (char *)pzBufferOld;
	for (; (*pzBuffer) && (i < 255); pzBuffer++, i++)
	{
		switch (*pzBuffer)
		{
			case '\t':
			case '\n':
			case '\\':
			{
				if (i < 254)
				{
					pzBufferNew[i++] = '\\';
					switch (*pzBuffer)
					{
						case '\t':
							pzBufferNew[i] = 't';
							break;

						case '\n':
							pzBufferNew[i] = 'n';
							break;

						case '\\':
							pzBufferNew[i] = '\\';
							break;
					}
				}
				else
					pzBufferNew[i] = '\0';
				break;
			}

			default:
				pzBufferNew[i] = *pzBuffer;
				break;
		}
	}

	pzBufferNew[i] = '\0';
}

// ------------------------------------------------------- //
// Main function
// ------------------------------------------------------- //
int main(int argc, char *argv[])
{
	ae::AttribEdit *pcApp = new ae::AttribEdit(argc == 2 ? argv[1] : NULL);
	pcApp->Run();
	return 0;
}
