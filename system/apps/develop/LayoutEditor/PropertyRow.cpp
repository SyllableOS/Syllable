#include "PropertyRow.h"
#include "messages.h"
#include "ccatalog.h"
#include <stdio.h>
#include <util/message.h>
#include <gui/window.h>

using namespace os;

/* THIS CODE HAS BEEN TAKEN FROM CATFISH */

/* Handler to handle the messages from the edit list window */

class PropertyListHandler : public os::Handler
{
public:
	PropertyListHandler( PropertyTreeNode* pcNode, os::EditListWin* pcWindow )
		: os::Handler( "list_handler" )
	{
		m_pcNode = pcNode;
		m_pcWindow = pcWindow;
		
		m_pcWindow->AddHandler( this );
	}
	
	~PropertyListHandler()
	{
	}
	
	void HandleMessage( os::Message* pcMsg )
	{
		switch( pcMsg->GetCode() )
		{
			case M_EDIT_LIST_FINISH:
				m_pcNode->EditDone();
			break;
			default:
			break;
		}
		os::Handler::HandleMessage( pcMsg );
	}
		
private:
	PropertyTreeNode* m_pcNode;
	os::EditListWin* m_pcWindow;
};

PropertyTreeNode::PropertyTreeNode( os::LayoutNode* pcNode, WidgetProperty cProperty )
{
	m_pcEditBox = NULL;
	m_pcDropdownBox = NULL;
	m_pcEditListWin = NULL;
	m_bShowEditBox = false;
	m_pcNode = pcNode;
	m_cProperty = cProperty;
	SetTextFlags( 0 );
}

PropertyTreeNode::~PropertyTreeNode()
{
	if( m_pcEditListWin )
	{
		delete( m_pcEditListHandler );
		m_pcEditListHandler = NULL;
		m_pcEditListWin->PostMessage( os::M_TERMINATE, m_pcEditListWin );
	}	
}

void PropertyTreeNode::Paint( const Rect& cFrame, View* pcView, uint nColumn, bool bSelected, bool bHighlighted, bool bHasFocus )
{
	if( ( m_pcEditBox == NULL && m_pcDropdownBox == NULL && m_pcEditListWin == NULL )
		 && m_bShowEditBox && m_nEditCol == nColumn ) {
		m_bShowEditBox = false;

		Rect cRect;
		cRect.left = pcView->GetBounds().left;
		cRect.right = pcView->GetBounds().right - 4;
		cRect.top = cFrame.top - 3;
		cRect.bottom = cFrame.bottom + 3;
		String s;
		
		if( m_cProperty.GetType() == PT_BOOL )
		{
			/* Create dropdown box */
			m_pcDropdownBox = new PropertyDropdownView( cRect, "" );
			m_pcDropdownBox->SetReadOnly( true );
			m_pcDropdownBox->AppendItem( "false" );
			m_pcDropdownBox->AppendItem( "true" );
			m_pcDropdownBox->SetSelection( m_cProperty.GetValue().AsInt32() );
			m_pcDropdownBox->m_cTreeNode = this;
			pcView->AddChild( m_pcDropdownBox );
			m_pcDropdownBox->MakeFocus( true );
		} 
		else if( m_cProperty.GetType() == PT_STRING_SELECT || m_cProperty.GetType() == PT_STRING_CATALOG )
		{
			/* Create dropdown box */
			m_pcDropdownBox = new PropertyDropdownView( cRect, "" );
			int nIndex = 0;
			if( m_cProperty.GetType() == PT_STRING_SELECT )
			{
				os::String zString;
				while( m_cProperty.GetSelection( &zString, nIndex ) )
				{
					m_pcDropdownBox->AppendItem( zString );
					if( zString == m_cProperty.GetValue().AsString() )
						m_pcDropdownBox->SetSelection( nIndex );
					nIndex++;
				}
				m_pcDropdownBox->SetReadOnly( true );
			}
			else
			{
				/* Create dropdown box with strings from the catalog file */
				os::CCatalog* pcCatalog = GetCatalog();
				bool bFound = false;
				bool bSearchString = false;
				int nIndex = 0;
				os::String cCurrent = m_cProperty.GetValue().AsString();
				if( cCurrent.Length() > 2 && cCurrent.substr( 0, 2 ) == "\\\\" )
				{
					bSearchString = true;
					cCurrent = cCurrent.substr( 2, cCurrent.Length() - 2 );
				}

				if( pcCatalog != NULL )
				{
					for( os::CCatalog::const_iterator i = pcCatalog->begin(); i != pcCatalog->end(); i++ )
					{
						if( (*i).second.Length() > 25 )					
							m_pcDropdownBox->AppendItem( (*i).second.substr( 0, 25 ) + "... \\\\" + pcCatalog->GetMnemonic( (*i).first ) );
						else
							m_pcDropdownBox->AppendItem( (*i).second + " \\\\" + pcCatalog->GetMnemonic( (*i).first ) );
						if( bSearchString ) {
							if( cCurrent == pcCatalog->GetMnemonic( (*i).first ) ) {
								m_pcDropdownBox->SetSelection( nIndex );
								bFound = true;
							}
						}
						nIndex++;
					}
				}
				if( !bFound )
					m_pcDropdownBox->SetCurrentString( m_cProperty.GetValue().AsString() );
			}
			
			m_pcDropdownBox->m_cTreeNode = this;
			pcView->AddChild( m_pcDropdownBox );
			m_pcDropdownBox->MakeFocus( true );
		}
		else if( m_cProperty.GetType() == PT_STRING_LIST )
		{
			/* Create list edit window */
			std::vector<os::String> acList;
			int nIndex = 0;
			os::String zString;
			while( m_cProperty.GetSelection( &zString, nIndex ) )
			{
				acList.push_back( zString );
				nIndex++;
			}
			
			
			m_pcEditListWin = new os::EditListWin( os::Rect( 0, 0, 200, 200 ), "Edit list", acList );
			m_pcEditListHandler = new PropertyListHandler( this, m_pcEditListWin );
			m_pcEditListWin->SetTarget( m_pcEditListHandler );
			m_pcEditListWin->CenterInScreen();
			m_pcEditListWin->Show();
			m_pcEditListWin->MakeFocus();
		}
		else
		{
			/* Create textview */
			s = GetString( m_nEditCol );
			m_pcEditBox = new PropertyEditView( cRect, "", s.c_str() );
			m_pcEditBox->SetMultiLine(true);
			m_pcEditBox->m_cTreeNode = this;
			pcView->AddChild( m_pcEditBox );
			m_pcEditBox->MakeFocus( true );
		}
	}

	TreeViewStringNode::Paint( cFrame, pcView, nColumn, bSelected, bHighlighted, bHasFocus );
}

bool PropertyTreeNode::HitTest( View * pcView, const Rect & cFrame, int nColumn, Point cPos )
{
	if( ( m_pcEditBox == NULL && m_pcDropdownBox == NULL && m_pcEditListWin == NULL ) && nColumn == 1 ) {
		m_nEditCol = nColumn;
		m_bShowEditBox = true;
	} else {
		return TreeViewStringNode::HitTest( pcView, cFrame, nColumn, cPos );
	}
	return true;
}

bool PropertyTreeNode::EditBegin( uint nColumn )
{
	if( m_pcEditBox == NULL && nColumn == 1 ) {
		m_nEditCol = nColumn;
		m_bShowEditBox = true;
		return true;
	} else {
		return false;
	}
}

void PropertyTreeNode::EditDone( int nEditOther )
{
	printf("Edit done!\n");
	int nSelected;
	TreeView *tv = GetOwner();
	nSelected = tv->GetFirstSelected();
	
	if( m_pcEditListWin )
	{
		/* Copy list */
		m_cProperty.ClearSelections();
		std::vector<os::String> acValues = m_pcEditListWin->GetData();
		for( uint i = 0; i < acValues.size(); i++ )
			m_cProperty.AddSelection( acValues[i] );
		
		/* Update property */	
		os::Message cMsg( M_UPDATE_PROPERTY );
		cMsg.AddPointer( "node", m_pcNode );
		cMsg.AddData( "property", T_RAW, (const void*)&m_cProperty, sizeof( m_cProperty ) );

		tv->GetWindow()->PostMessage( &cMsg, tv->GetWindow() );
		
		/* Delete list window */
		delete( m_pcEditListHandler );
		m_pcEditListHandler = NULL;
		m_pcEditListWin->PostMessage( os::M_TERMINATE, m_pcEditListWin );
		m_pcEditListWin = NULL;
	}
	
	if( m_pcDropdownBox ) {
		/* Set value */
		if( m_cProperty.GetType() == PT_STRING_CATALOG )
		{
			/* Extract catalog string */
			os::String cString = m_pcDropdownBox->GetCurrentString();
			uint nPos;
			if( ( nPos = cString.find( " \\\\" ) ) != std::string::npos )
			{
				cString = cString.substr( nPos + 1, cString.Length() - nPos - 1 );
			}
			m_cProperty.SetValue( cString );			
		}
		else if( m_cProperty.GetType() == PT_STRING_SELECT )
		{
			os::String cString = m_pcDropdownBox->GetCurrentString();
			m_cProperty.SetValue( cString );
		}
		else
			m_cProperty.SetValue( m_pcDropdownBox->GetSelection() );

		/* Update property */	
		os::Message cMsg( M_UPDATE_PROPERTY );
		cMsg.AddPointer( "node", m_pcNode );
		cMsg.AddData( "property", T_RAW, (const void*)&m_cProperty, sizeof( m_cProperty ) );
		tv->GetWindow()->PostMessage( &cMsg, tv->GetWindow() );

		SetString( m_nEditCol, m_pcDropdownBox->GetCurrentString() );

		/* Delete view */
		tv->GetWindow()->PostMessage( (uint32)0, m_pcDropdownBox );
		m_pcDropdownBox = NULL;
	}
	if( m_pcEditBox ) {
		String s = m_pcEditBox->GetValue().AsString();
		
		switch( m_cProperty.GetType() )
		{
			case PT_STRING:
			case PT_INT:
			case PT_FLOAT:
				m_cProperty.SetValue( m_pcEditBox->GetValue() );
			break;
			default:
			break;
		}
		
		os::Message cMsg( M_UPDATE_PROPERTY );
		cMsg.AddPointer( "node", m_pcNode );
		cMsg.AddData( "property", T_RAW, (const void*)&m_cProperty, sizeof( m_cProperty ) );
		tv->GetWindow()->PostMessage( &cMsg, tv->GetWindow() );
		
		SetString( m_nEditCol, s );
		if( m_pcEditBox->HasResized() ) {
			tv->ClearSelection();
			if( nSelected > -1 ) {
				tv->RemoveNode( nSelected );
				tv->InsertNode( nSelected, this );
			}
		}
		tv->GetWindow()->PostMessage( (uint32)0, m_pcEditBox );
		m_pcEditBox = NULL;
	}

	if( nEditOther && nSelected > -1 ) {
		PropertyTreeNode*	row;

		do {
			nSelected += nEditOther;
			row = (PropertyTreeNode*)tv->GetRow( nSelected );
			if( row ) {
				if( row->EditBegin( m_nEditCol ) ) {
					tv->Select( nSelected );
					tv->MakeVisible( nSelected );
					tv->InvalidateRow( nSelected, 0 );
					break;
				}
			}
		} while( row );
	}
}

// --------------------------------------------------------------------------------------

PropertyEditView::PropertyEditView( const Rect& cFrame, const String& cTitle, const char* pzBuffer, uint32 nResizeMask, uint32 nFlags )
:TextView( cFrame, cTitle, pzBuffer, nResizeMask, nFlags)
{
	m_bResized = false;
}


void PropertyEditView::Activated( bool bIsActive )
{
	if( bIsActive == false ) {
		m_cTreeNode->EditDone();
	}
}

void PropertyEditView::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
	IPoint cpos;
	bool bNotShift = ( (nQualifiers & QUAL_SHIFT) == 0 );

	if( *pzString == '\n' && bNotShift ) {
		m_cTreeNode->EditDone( 1 );
		return;
	}

	switch( *pzString )
	{
		default:
			TextView::KeyDown( pzString, pzRawString, nQualifiers );
			break;
		case VK_UP_ARROW:
			cpos = GetCursor();
			TextView::KeyDown( pzString, pzRawString, nQualifiers );
			if( cpos == GetCursor() && bNotShift ) {
				m_cTreeNode->EditDone( -1 );
			}
			break;
		case VK_DOWN_ARROW:
			cpos = GetCursor();
			TextView::KeyDown( pzString, pzRawString, nQualifiers );
			if( cpos == GetCursor() && bNotShift ) {
				m_cTreeNode->EditDone( 1 );
			}
			break;
		case '\n':
		case '\b':
			TextView::KeyDown( pzString, pzRawString, nQualifiers );
			Point cSize = GetTextExtent( GetValue().AsString() );
			Rect cFrame = GetFrame();
			cFrame.bottom = cFrame.top + cSize.y + 6 + 2 + 1;
			SetFrame( cFrame );
			m_bResized = true;
			break;
	}
}

bool PropertyEditView::HasResized()
{
	return m_bResized;
}


void PropertyEditView::HandleMessage( os::Message* pcMsg )
{
	switch( pcMsg->GetCode() )
	{
		case 0:
			delete( this );
			return;
		break;
	}
	os::TextView::HandleMessage( pcMsg );
}


// --------------------------------------------------------------------------------------

PropertyDropdownView::PropertyDropdownView( const Rect& cFrame, const String& cTitle, uint32 nResizeMask, uint32 nFlags )
:DropdownMenu( cFrame, cTitle,  nResizeMask, nFlags)
{
	
}

void PropertyDropdownView::AttachedToWindow()
{
	/* FIXME: This is a hack! */
	os::TextView* pcView = static_cast<os::TextView*>( GetChildAt( 0 ) );
	pcView->SetTarget( this );
	
}

void PropertyDropdownView::Activated( bool bIsActive )
{
	if( bIsActive == false ) {
		if( GetWindow()->GetFocusChild() != NULL )
			if( GetWindow()->GetFocusChild()->GetParent() == this )
				return;
		m_cTreeNode->EditDone();
	}
}

void PropertyDropdownView::HandleMessage( os::Message* pcMsg )
{
	switch( pcMsg->GetCode() )
	{
		case 0:
			delete( this );
			return;
		break;
		case 3: // os::DropdownMenu::ID_STRING_CHANGED
			int32 nEvents;
			pcMsg->FindInt32( "events", &nEvents );
			if( nEvents & ( os::TextView::EI_FOCUS_LOST | os::TextView::EI_ENTER_PRESSED ) )
				m_cTreeNode->EditDone();
		break;
	}
	os::DropdownMenu::HandleMessage( pcMsg );
}





































