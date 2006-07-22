#ifndef _WIDGETSUTILS_H_
#define _WIDGETSUTILS_H_

class SeparatorWidget : public Widget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class SliderWidget : public Widget
{
public:
	virtual const std::type_info* GetTypeID() = 0;
	virtual const os::String GetName() = 0;
	virtual os::LayoutNode* CreateLayoutNode( os::String zName ) = 0;
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class HorizontalSliderWidget : public SliderWidget
{
public:
	const std::type_info* GetTypeID() ;
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	void CreateHeaderCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class VerticalSliderWidget : public SliderWidget
{
public:
	const std::type_info* GetTypeID() ;
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	void CreateHeaderCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class SpinnerWidget : public Widget
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





