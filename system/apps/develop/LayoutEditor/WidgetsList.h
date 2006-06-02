#ifndef _WIDGETLIST_H_
#define _WIDGETSLIST_H_

class ListViewWidget : public Widget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};


#endif






