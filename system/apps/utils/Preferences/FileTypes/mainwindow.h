#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <util/application.h>
#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/frameview.h>
#include <gui/treeview.h>
#include <gui/textview.h>
#include <gui/imagebutton.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/filerequester.h>
#include <util/message.h>
#include <vector>
#include <storage/registrar.h>

class MainWindow : public os::Window
{
public:
	MainWindow();
	~MainWindow();
	void UpdateList();
	void UpdateIcon( bool bUpdateList );
	void SelectType( int32 nIndex );
	void AddExtension();
	void RemoveExtension();
	void ApplyType();
	void DumpType( os::Message cMessage );
	void HandleMessage( os::Message* );
private:
	bool OkToQuit();
	os::RegistrarManager* m_pcManager;
	
	os::LayoutView* m_pcRoot;
	os::HLayoutNode* m_pcHLayout;
	os::TreeView* m_pcTypeList;
	os::VLayoutNode* m_pcVType;
	os::FrameView* m_pcGeneralFrame;
	os::StringView* m_pcMimeType;
	os::TextView* m_pcIdentifier;
	os::ImageButton* m_pcIconChooser;
	os::FileRequester* m_pcIconReq;
	
	os::FrameView* m_pcExtFrame;
	
	os::TreeView* m_pcExtensionList;
	os::TextView* m_pcExtension;
	os::Button* m_pcExtensionAdd;
	os::Button* m_pcExtensionRemove;
	
	os::FrameView* m_pcHandlerFrame;
	os::TreeView* m_pcHandlerList;
	os::FileRequester* m_pcHandlerReq;
	os::Button* m_pcAddHandler;
	os::Button* m_pcRemoveHandler;
	
	os::HLayoutNode* m_pcHButtons;
	os::Button* m_pcAddType;
	os::Button* m_pcRemoveType;
	
	os::Button* m_pcApply;
	
	int m_nCurrentType;
	bool m_bIconReqShown;
	bool m_bHandlerReqShown;
	os::String m_zIcon;
	std::vector<os::String> m_cExtensions;
	std::vector<os::String> m_cHandlers;
	
	os::BitmapImage* m_pcDummyImage;
};

#endif



























