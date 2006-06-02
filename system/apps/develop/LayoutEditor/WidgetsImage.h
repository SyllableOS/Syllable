#ifndef _WIDGETSIMAGE_H_
#define _WIDGETSIMAGE_H_

class ImageViewWidget : public Widget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class ImageButtonWidget : public Widget
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


