#include "flowlayout.h"

static void RemBorders( Rect* pcRect, const Rect& cBorders )
{
    pcRect->Resize( cBorders.left, cBorders.top, -cBorders.right, -cBorders.bottom );
}

FlowLayoutNode::FlowLayoutNode( const std::string& cName, float vWeight, LayoutNode* pcParent, View* pcView )
	: LayoutNode( cName, vWeight, pcParent, pcView )
{
}

Point FlowLayoutNode::CalculatePreferredSize(bool bLargest)
{
    const std::vector<LayoutNode*> cList = GetChildList();
    Rect cBorders( GetBorders() );
    Point cSize( 0.0f, 0.0f );
    
    for ( uint i = 0 ; i < cList.size() ; ++i ) {
	Point cChildSize = cList[i]->GetPreferredSize(bLargest);
	cSize.x += cChildSize.x;
	if ( cChildSize.y > cSize.y ) {
	    cSize.y = cChildSize.y;
	}
    }
    cSize += Point( cBorders.left + cBorders.right, cBorders.top + cBorders.bottom );
    return( cSize );
}

void FlowLayoutNode::Layout()
{
	Rect cBounds = GetBounds();
	float xspace = 4.0;
	float yspace = 4.0;
	float rowheight = 0.0;
	const std::vector<LayoutNode*> cChildList = GetChildList();

	RemBorders( &cBounds, GetBorders() );

	Point	pos(cBounds.left, cBounds.top);
	Rect	cFrame;

	for(uint i = 0; i < cChildList.size(); i++ ) {
		Point size = cChildList[i]->GetPreferredSize(false);

		if(pos.x + size.x + xspace > cBounds.right) {
			if(pos.x > 0) {
				pos.y += rowheight + yspace;
				rowheight = 0.0;
				pos.x = 0.0;
			}
		}

		if(size.y > rowheight)
			rowheight = size.y;

		cFrame.left = pos.x;
		cFrame.top = pos.y;
		cFrame.right = cFrame.left + size.x;
		cFrame.bottom = cFrame.top + size.y;
		
		pos.x += size.x + xspace;

		//cout << "Frame: " << cFrame.left << " " << cFrame.top << " " << cFrame.right << " " << cFrame.bottom << endl;
		//cout << "Pos:   " << pos.x << " " << pos.y << endl;
		//cout << "Size:  " << size.x << " " << size.y << endl;

		cChildList[i]->SetFrame( cFrame );
	}
    
}






