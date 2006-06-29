#ifndef _WIDGETSNODES_H_
#define _WIDGETSNODES_H_

class LEHLayoutNode : public os::HLayoutNode
{
public:
	LEHLayoutNode( const os::String& cName, float vWeight = 1.0f, os::LayoutNode* pcParent = NULL, os::View* pcView = NULL )
				: HLayoutNode( cName, vWeight, pcParent, pcView )
	{
		m_bSameWidth = false;
	}
	bool m_bSameWidth;
};

class LEVLayoutNode : public os::VLayoutNode
{
public:
	LEVLayoutNode( const os::String& cName, float vWeight = 1.0f, os::LayoutNode* pcParent = NULL, os::View* pcView = NULL )
				: VLayoutNode( cName, vWeight, pcParent, pcView )
	{
		m_bSameHeight= false;
	}
	bool m_bSameHeight;
};


class HLayoutNodeWidget : public Widget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	bool CanHaveChildren();
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
	void CreateCodeEnd( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class VLayoutNodeWidget : public Widget
{
public:
	virtual const std::type_info* GetTypeID();
	virtual const os::String GetName();
	virtual os::LayoutNode* CreateLayoutNode( os::String zName );
	bool CanHaveChildren();
	virtual std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	virtual void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	virtual void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
	virtual void CreateCodeEnd( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class HLayoutSpacerWidget : public Widget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};


class VLayoutSpacerWidget : public Widget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class FrameViewWidget : public VLayoutNodeWidget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	bool CanHaveChildren();
	void AddChild( os::LayoutNode* pcNode, os::LayoutNode* pcChild );
	void RemoveChild( os::LayoutNode* pcNode, os::LayoutNode* pcChild );
	const std::vector<os::LayoutNode*> GetChildList( os::LayoutNode* pcNode );
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
	void CreateCodeEnd( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
	void CreateHeaderCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

class TabViewWidget : public Widget
{
public:
	const std::type_info* GetTypeID();
	const os::String GetName();
	os::LayoutNode* CreateLayoutNode( os::String zName );
	bool CanHaveChildren();
	void AddChild( os::LayoutNode* pcNode, os::LayoutNode* pcChild );
	void RemoveChild( os::LayoutNode* pcNode, os::LayoutNode* pcChild );
	const std::vector<os::LayoutNode*> GetChildList( os::LayoutNode* pcNode );
	std::vector<WidgetProperty> GetProperties( os::LayoutNode* pcNode );
	void SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties );
	void CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
	void CreateCodeEnd( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
	void CreateHeaderCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode );
};

#endif







