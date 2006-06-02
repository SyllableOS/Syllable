#ifndef NP_H
#define NP_H

#include <gui/window.h>
#include <gui/textview.h>
#include <gui/button.h>
#include <gui/treeview.h>
#include <gui/stringview.h>
#include <util/resources.h>
#include <gui/image.h>
#include <gui/view.h>
#include <gui/imagebutton.h>
#include <gui/filerequester.h>
#include <gui/frameview.h>
#include <storage/fsnode.h>
#include <gui/requesters.h>
#include <storage/path.h>
#include <storage/directory.h>

#include "project.h"

using namespace os;

class NewProjectDialog : public Window
{
public:
	NewProjectDialog(Window*);
	~NewProjectDialog();
	
private:
	TreeView* pcListView;
	Button* pcOk;
	Button* pcCancel;
	StringView* pcSVPrjName;
	StringView* pcSVPrjLocation;
	TextView* pcTVPrjLocation;
	TextView* pcTVPrjName;
	Button* pcButLocation;
	ImageButton* pcFileReqBut;
	FileRequester* pcFileRequester;
	void SetItems();
	FrameView* pcFrameView;
	void AddNode( int nIndent, os::String z1, BitmapImage*); 
	BitmapImage* LoadImage(const std::string& zResource);
	void HandleMessage(Message*);
	FSNode* pcFileNode;
	Alert* pcErrorAlert;
	Path* cFilePath;
	project* cProject;
	Window* pcParentWindow;
	void SetTabs();
};

#endif















