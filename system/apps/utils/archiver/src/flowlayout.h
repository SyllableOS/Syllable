#ifndef FLOWLAYOUT_H__
#define FLOWLAYOUT_H__

#include <gui/layoutview.h>

#include <vector>

using namespace os;

class FlowLayoutNode : public LayoutNode
{
	public:
	FlowLayoutNode( const std::string& cName, float vWeight = 1.0f, LayoutNode* pcParent = NULL, View* pcView = NULL );

	virtual void  Layout();

	protected:
	virtual Point    CalculatePreferredSize( bool bLargest );
	private:
};

#endif /* FLOWLAYOUT_H__*/


