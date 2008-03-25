#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gui/window.h>
#include <gui/treeview.h>
#include <gui/filerequester.h>
#include <util/application.h>
#include <util/message.h>
#include <storage/file.h>
#include <vector>
#include "AppWindow.h"
#include "Widgets.h"
#include "EditString.h"
#include "ccatalog.h"

class MainWindow : public os::Window
{
public:
	MainWindow();
	~MainWindow();
	void InitWidgets();
	void InitMenus();
	void DeleteWidgets();
	Widget* GetNodeWidget( os::LayoutNode* pcNode );
	void CreateWidgetTreeLevel( int nLevel, os::LayoutNode* pcNode, os::LayoutNode* pcRealParent );
	void CreateWidgetTree();
	void CreatePropertyList( os::LayoutNode* pcNode );
	void LoadCatalog();
	os::String GetString( os::String& zString );
	os::CCatalog* GetCatalog()
	{
		return( m_pcCatalog );
	}
	bool Save( os::File* pcFile, int nLevel, os::LayoutNode* pcParentNode );
	void Save();
	bool Load( os::File* pcFile, int *pnLevel, os::LayoutNode* pcParentNode );
	void Load( os::String zFileName );
	bool CreateCode( os::File* pcSourceFile, os::File* pcHeaderFile, int nLevel, os::LayoutNode* pcParentNode );
	void CreateCode();
	bool FindNode( os::String zName, os::LayoutNode* pcParentNode );
	void ResetWidthAndHeight( os::LayoutNode* pcNode );
	void HandleMessage( os::Message* );
	static os::String GetFileName()
	{
		return( m_zFileName );
	}
private:
	bool OkToQuit();
	os::LayoutView* m_pcLayoutView;
	os::VLayoutNode* m_pcRoot;
	os::TreeView* m_pcWidgetTree;
	os::TreeView* m_pcPropertyTree;
	AppWindow* m_pcAppWindow;
	static os::String m_zFileName;
	std::vector<Widget*> m_apcWidgets;
	os::EditStringWin* m_pcNewWin;
	os::LayoutNode* m_pcNewParentNode;
	Widget* m_pcNewWidget;
	os::FileRequester* m_pcLoadRequester;
	os::FileRequester* m_pcSaveRequester;
	os::CCatalog *m_pcCatalog;
};

#endif

































