#ifndef _WIDGETSCONTROLS_H_
#define _WIDGETSCONTROLS_H_

class StringViewWidget : public Widget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class ButtonWidget : public Widget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class CheckBoxWidget : public Widget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class RadioButtonWidget : public Widget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class DropdownMenuWidget : public Widget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class TextViewWidget : public Widget
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












