#ifndef _PROPERTY_ROW_H
#define _PROPERTY_ROW_H

#include <gui/treeview.h>
#include <gui/textview.h>
#include <gui/dropdownmenu.h>
#include "Widgets.h"
#include "EditList.h"

class PropertyTreeNode;


class PropertyEditView: public os::TextView {
	public:
    PropertyEditView( const os::Rect& cFrame, const os::String& cTitle, const char* pzBuffer,
	      uint32 nResizeMask = os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP,
	      uint32 nFlags = os::WID_WILL_DRAW | os::WID_FULL_UPDATE_ON_RESIZE );

	void Activated( bool bIsActive );
	virtual void	KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
	virtual void HandleMessage( os::Message* pcMsg );

	bool HasResized();

	PropertyTreeNode	*m_cTreeNode;

	private:
	bool m_bResized;
};

class PropertyDropdownView: public os::DropdownMenu {
	public:
    PropertyDropdownView( const os::Rect& cFrame, const os::String& cTitle,
	      uint32 nResizeMask = os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP,
	      uint32 nFlags = os::WID_WILL_DRAW | os::WID_FULL_UPDATE_ON_RESIZE );

	void AttachedToWindow( void );
	void Activated( bool bIsActive );
	virtual void HandleMessage( os::Message* pcMsg );
	
	PropertyTreeNode	*m_cTreeNode;
};

class PropertyTreeNode: public os::TreeViewStringNode {
	public:
	PropertyTreeNode( os::LayoutNode* pcNode, WidgetProperty cProperty );
	~PropertyTreeNode();

	virtual void Paint( const os::Rect& cFrame, os::View* pcView, uint nColumn, bool bSelected, bool bHighlighted, bool bHasFocus );
	virtual bool HitTest( os::View * pcView, const os::Rect & cFrame, int nColumn, os::Point cPos );
	void EditDone( int nEditOther = 0 );
	bool EditBegin( uint nColumn );
	os::LayoutNode* GetNode() { return( m_pcNode ); }
	WidgetProperty GetProperty() { return( m_cProperty ); }

	private:
	PropertyEditView*	m_pcEditBox;
	os::EditListWin*	m_pcEditListWin;
	os::Handler*		m_pcEditListHandler;
	PropertyDropdownView* m_pcDropdownBox;
	WidgetProperty		m_cProperty;
	uint32				m_nEditCol;
	bool				m_bShowEditBox;
	os::LayoutNode*		m_pcNode;
	
};

#endif




















