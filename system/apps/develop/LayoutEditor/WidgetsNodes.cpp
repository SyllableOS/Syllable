#include "Widgets.h"
#include "WidgetsNodes.h"

#include <gui/frameview.h>
#include <gui/tabview.h>
#include <util/message.h>
#include <cassert>

/* HLayoutNode Widget */

const std::type_info* HLayoutNodeWidget::GetTypeID()
{
	return( &typeid( LEHLayoutNode ) );
}

const os::String HLayoutNodeWidget::GetName()
{
	return( "HLayoutNode" );
}

os::LayoutNode* HLayoutNodeWidget::CreateLayoutNode( os::String zName )
{
	return( new LEHLayoutNode( zName, 1.0f, NULL ) );
}

bool HLayoutNodeWidget::CanHaveChildren()
{
	return( true );
}

std::vector<WidgetProperty> HLayoutNodeWidget::GetProperties( os::LayoutNode* pcNode )
{
	LEHLayoutNode* pcLEHLayoutNode = static_cast<LEHLayoutNode*>(pcNode);
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// Left Border
	WidgetProperty cProperty1( 1, PT_FLOAT, "Left Border", pcNode->GetBorders().left );
	cProperties.push_back( cProperty1 );
	// Top Border
	WidgetProperty cProperty2( 2, PT_FLOAT, "Top Border", pcNode->GetBorders().top );
	cProperties.push_back( cProperty2 );
	// Right Border
	WidgetProperty cProperty3( 3, PT_FLOAT, "Right Border", pcNode->GetBorders().right );
	cProperties.push_back( cProperty3 );
	// Bottom Border
	WidgetProperty cProperty4( 4, PT_FLOAT, "Bottom Border", pcNode->GetBorders().bottom );
	cProperties.push_back( cProperty4 );
	// Same Width
	WidgetProperty cProperty5( 5, PT_BOOL, "Same Width", pcLEHLayoutNode->m_bSameWidth );
	cProperties.push_back( cProperty5 );
	return( cProperties );
}


void HLayoutNodeWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	LEHLayoutNode* pcLEHLayoutNode = static_cast<LEHLayoutNode*>(pcNode);
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Left Border
			{
				os::Rect cBorders = pcNode->GetBorders();
				cBorders.left = pcProp->GetValue().AsFloat();
				pcNode->SetBorders( cBorders );
			}
			break;
			case 2: // Top Border
			{
				os::Rect cBorders = pcNode->GetBorders();
				cBorders.top = pcProp->GetValue().AsFloat();
				pcNode->SetBorders( cBorders );
			}
			break;
			case 3: // Right Border
			{
				os::Rect cBorders = pcNode->GetBorders();
				cBorders.right = pcProp->GetValue().AsFloat();
				pcNode->SetBorders( cBorders );
			}
			break;
			case 4: // Bottom Border
			{
				os::Rect cBorders = pcNode->GetBorders();
				cBorders.bottom = pcProp->GetValue().AsFloat();
				pcNode->SetBorders( cBorders );
			}
			break;
			case 5: // Same Width
			{
				pcLEHLayoutNode->m_bSameWidth = pcProp->GetValue().AsBool();
				std::vector<os::LayoutNode*> apcWidgets = pcNode->GetChildList();
				if( pcLEHLayoutNode->m_bSameWidth == true )
				{
					/* We need to be careful only to set layout nodes
					   with attached views to the same size */
					int nCount = 0;
					int nFirst = -1;
					for( uint i = 0; i < apcWidgets.size(); i++ )
					{
						if( apcWidgets[i]->GetView() != NULL )
						{
							nCount++;
							if( nFirst < 0 ) nFirst = i;
						}
					}
					if( nCount < 2 || nFirst == -1 ) 
						break;
					
					for( uint i = nFirst + 1; i < apcWidgets.size(); i++ )
					{
						if( apcWidgets[i]->GetView() != NULL )
						{
							//printf( "%s %s\n", apcWidgets[nFirst]->GetName().c_str(), apcWidgets[i]->GetName().c_str() );
							pcNode->SameWidth( apcWidgets[nFirst]->GetName().c_str(), apcWidgets[i]->GetName().c_str(), NULL );
						}
					}
				} else {
					for( uint i = 0; i < apcWidgets.size(); i++ )
						apcWidgets[i]->SameWidth( NULL );
				}
			}
			break;
		}
	}
}

void HLayoutNodeWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::HLayoutNode( \"%s\", %f );\n", pcNode->GetName().c_str(), pcNode->GetName().c_str(), pcNode->GetWeight() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	sprintf( zBuffer, "m_pc%s->SetBorders( os::Rect( %f, %f, %f, %f ) );\n", pcNode->GetName().c_str(), 
			pcNode->GetBorders().left, pcNode->GetBorders().top, pcNode->GetBorders().right, pcNode->GetBorders().bottom );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	sprintf( zBuffer, "m_pc%s->AddChild( m_pc%s );\n", pcNode->GetParent()->GetName().c_str(), pcNode->GetName().c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
}


void HLayoutNodeWidget::CreateCodeEnd( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	char zBuffer[8192];
	LEHLayoutNode* pcLEHLayoutNode = static_cast<LEHLayoutNode*>(pcNode);
	
	if( pcLEHLayoutNode->m_bSameWidth == true )
	{
		std::vector<os::LayoutNode*> apcWidgets = pcNode->GetChildList();		
		int nCount = 0;
		int nFirst = -1;
		for( uint i = 0; i < apcWidgets.size(); i++ )
		{
			if( apcWidgets[i]->GetView() != NULL )
			{
				nCount++;
				if( nFirst < 0 ) nFirst = i;
			}
		}
		if( nCount < 2 || nFirst == -1 ) 
			return;

		for( uint i = nFirst + 1; i < apcWidgets.size(); i++ )
		{
			if( apcWidgets[i]->GetView() != NULL )
			{
				sprintf( zBuffer, "m_pc%s->SameWidth( \"%s\", \"%s\", NULL );\n", pcNode->GetName().c_str(), apcWidgets[nFirst]->GetName().c_str(), apcWidgets[i]->GetName().c_str() );
				pcFile->Write( zBuffer, strlen( zBuffer ) );
			}
		}
	}
}

/* VLayoutNode Widget */

const std::type_info* VLayoutNodeWidget::GetTypeID()
{
	return( &typeid( LEVLayoutNode ) );
}

const os::String VLayoutNodeWidget::GetName()
{
	return( "VLayoutNode" );
}

os::LayoutNode* VLayoutNodeWidget::CreateLayoutNode( os::String zName )
{
	return( new LEVLayoutNode( zName, 1.0f, NULL ) );
}

bool VLayoutNodeWidget::CanHaveChildren()
{
	return( true );
}

static os::String AlignmentToString( os::alignment eAlign )
{
	switch( eAlign )
	{
		case os::ALIGN_LEFT:
			return( "Left" );
		case os::ALIGN_RIGHT:
			return( "Right" );
		case os::ALIGN_CENTER:
			return( "Center" );
		default:
			return( "Unknown" );
	}
}

static os::alignment StringToAlignment( os::String zString )
{
	if( zString == "Left" )
		return( os::ALIGN_LEFT );
	else if( zString == "Right" )
		return( os::ALIGN_RIGHT );
	else if( zString == "Center" )
		return( os::ALIGN_CENTER );
	return( os::ALIGN_LEFT );
}

std::vector<WidgetProperty> VLayoutNodeWidget::GetProperties( os::LayoutNode* pcNode )
{
	LEVLayoutNode* pcLEVLayoutNode = static_cast<LEVLayoutNode*>(pcNode);
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// Left Border
	WidgetProperty cProperty1( 1, PT_FLOAT, "Left Border", pcNode->GetBorders().left );
	cProperties.push_back( cProperty1 );
	// Top Border
	WidgetProperty cProperty2( 2, PT_FLOAT, "Top Border", pcNode->GetBorders().top );
	cProperties.push_back( cProperty2 );
	// Right Border
	WidgetProperty cProperty3( 3, PT_FLOAT, "Right Border", pcNode->GetBorders().right );
	cProperties.push_back( cProperty3 );
	// Bottom Border
	WidgetProperty cProperty4( 4, PT_FLOAT, "Bottom Border", pcNode->GetBorders().bottom );
	cProperties.push_back( cProperty4 );
	// Same Height
	WidgetProperty cProperty5( 5, PT_BOOL, "Same Height", pcLEVLayoutNode->m_bSameHeight );
	cProperties.push_back( cProperty5 );
	return( cProperties );
}


void VLayoutNodeWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	LEVLayoutNode* pcLEVLayoutNode = static_cast<LEVLayoutNode*>(pcNode);
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Left Border
			{
				os::Rect cBorders = pcNode->GetBorders();
				cBorders.left = pcProp->GetValue().AsFloat();
				pcNode->SetBorders( cBorders );
			}
			break;
			case 2: // Top Border
			{
				os::Rect cBorders = pcNode->GetBorders();
				cBorders.top = pcProp->GetValue().AsFloat();
				pcNode->SetBorders( cBorders );
			}
			break;
			case 3: // Right Border
			{
				os::Rect cBorders = pcNode->GetBorders();
				cBorders.right = pcProp->GetValue().AsFloat();
				pcNode->SetBorders( cBorders );
			}
			break;
			case 4: // Bottom Border
			{
				os::Rect cBorders = pcNode->GetBorders();
				cBorders.bottom = pcProp->GetValue().AsFloat();
				pcNode->SetBorders( cBorders );
			}
			break;
			case 5: // Same Height
			{
				pcLEVLayoutNode->m_bSameHeight = pcProp->GetValue().AsBool();
				std::vector<os::LayoutNode*> apcWidgets = pcNode->GetChildList();
				if( pcLEVLayoutNode->m_bSameHeight == true )
				{
					/* We need to be careful only to set layout nodes
					   with attached views to the same size */
					int nCount = 0;
					int nFirst = -1;
					for( uint i = 0; i < apcWidgets.size(); i++ )
					{
						if( apcWidgets[i]->GetView() != NULL )
						{
							nCount++;
							if( nFirst < 0 ) nFirst = i;
						}
					}
					if( nCount < 2 || nFirst == -1 ) 
						break;
					
					for( uint i = nFirst + 1; i < apcWidgets.size(); i++ )
					{
						if( apcWidgets[i]->GetView() != NULL )
						{
							pcNode->SameHeight( apcWidgets[nFirst]->GetName().c_str(), apcWidgets[i]->GetName().c_str(), NULL );
						}
					}
				} else {
					for( uint i = 0; i < apcWidgets.size(); i++ )
						apcWidgets[i]->SameHeight( NULL );
				}
			}
			break;
		}
	}
}

void VLayoutNodeWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::VLayoutNode( \"%s\", %f );\n", pcNode->GetName().c_str(), pcNode->GetName().c_str(), pcNode->GetWeight() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	sprintf( zBuffer, "m_pc%s->SetBorders( os::Rect( %f, %f, %f, %f ) );\n", pcNode->GetName().c_str(), 
			pcNode->GetBorders().left, pcNode->GetBorders().top, pcNode->GetBorders().right, pcNode->GetBorders().bottom );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	sprintf( zBuffer, "m_pc%s->AddChild( m_pc%s );\n", pcNode->GetParent()->GetName().c_str(), pcNode->GetName().c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
}


void VLayoutNodeWidget::CreateCodeEnd( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	char zBuffer[8192];
	LEVLayoutNode* pcLEVLayoutNode = static_cast<LEVLayoutNode*>(pcNode);
	
	if( pcLEVLayoutNode->m_bSameHeight == true )
	{
		std::vector<os::LayoutNode*> apcWidgets = pcNode->GetChildList();		
		int nCount = 0;
		int nFirst = -1;
		for( uint i = 0; i < apcWidgets.size(); i++ )
		{
			if( apcWidgets[i]->GetView() != NULL )
			{
				nCount++;
				if( nFirst < 0 ) nFirst = i;
			}
		}
		if( nCount < 2 || nFirst == -1 ) 
			return;

		for( uint i = nFirst + 1; i < apcWidgets.size(); i++ )
		{
			//printf( "%i %i\n", nFirst, i );
			if( apcWidgets[i]->GetView() != NULL )
			{
				//printf( "%s %s\n", apcWidgets[nFirst]->GetName().c_str(), apcWidgets[i]->GetName().c_str() );
				sprintf( zBuffer, "m_pc%s->SameHeight( \"%s\", \"%s\", NULL );\n", pcNode->GetName().c_str(), apcWidgets[nFirst]->GetName().c_str(), apcWidgets[i]->GetName().c_str() );
				pcFile->Write( zBuffer, strlen( zBuffer ) );
			}
		}
	}
}

/* HLayoutSpacer Widget */

const std::type_info* HLayoutSpacerWidget::GetTypeID()
{
	return( &typeid( os::HLayoutSpacer ) );
}

const os::String HLayoutSpacerWidget::GetName()
{
	return( "HLayoutSpacer" );
}

os::LayoutNode* HLayoutSpacerWidget::CreateLayoutNode( os::String zName )
{
	return( new os::HLayoutSpacer( zName ) );
}

std::vector<WidgetProperty> HLayoutSpacerWidget::GetProperties( os::LayoutNode* pcNode )
{
	os::LayoutSpacer* pcSpacer = static_cast<os::LayoutSpacer*>(pcNode);
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcSpacer->GetWeight() );
	cProperties.push_back( cProperty0 );
	WidgetProperty cProperty1( 1, PT_FLOAT, "Min size", pcSpacer->GetMinSize().x );
	cProperties.push_back( cProperty1 );
	WidgetProperty cProperty2( 2, PT_FLOAT, "Max size", pcSpacer->GetMaxSize().x );
	cProperties.push_back( cProperty2 );

	return( cProperties );
}


void HLayoutSpacerWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	os::LayoutSpacer* pcSpacer = static_cast<os::LayoutSpacer*>(pcNode);
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Min size
				pcSpacer->SetMinSize( os::Point( pcProp->GetValue().AsFloat(), 0 ) );
			break;
			case 2: // Max size
				pcSpacer->SetMaxSize( os::Point( pcProp->GetValue().AsFloat(), 0 ) );
			break;
		}
	}
}

void HLayoutSpacerWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	os::LayoutSpacer* pcSpacer = static_cast<os::LayoutSpacer*>(pcNode);
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::HLayoutSpacer( \"%s\", %f, %f, NULL, %f );\n", 
		pcNode->GetName().c_str(), pcNode->GetName().c_str(), pcSpacer->GetMinSize().x, 
		pcSpacer->GetMaxSize().x, pcNode->GetWeight() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	sprintf( zBuffer, "m_pc%s->AddChild( m_pc%s );\n", pcNode->GetParent()->GetName().c_str(), pcNode->GetName().c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
}

/* VLayoutSpacer Widget */

const std::type_info* VLayoutSpacerWidget::GetTypeID()
{
	return( &typeid( os::VLayoutSpacer ) );
}

const os::String VLayoutSpacerWidget::GetName()
{
	return( "VLayoutSpacer" );
}

os::LayoutNode* VLayoutSpacerWidget::CreateLayoutNode( os::String zName )
{
	return( new os::VLayoutSpacer( zName ) );
}

std::vector<WidgetProperty> VLayoutSpacerWidget::GetProperties( os::LayoutNode* pcNode )
{
	os::LayoutSpacer* pcSpacer = static_cast<os::LayoutSpacer*>(pcNode);
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcSpacer->GetWeight() );
	cProperties.push_back( cProperty0 );
	WidgetProperty cProperty1( 1, PT_FLOAT, "Min size", pcSpacer->GetMinSize().y );
	cProperties.push_back( cProperty1 );
	WidgetProperty cProperty2( 2, PT_FLOAT, "Max size", pcSpacer->GetMaxSize().y );
	cProperties.push_back( cProperty2 );

	return( cProperties );
}


void VLayoutSpacerWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	os::LayoutSpacer* pcSpacer = static_cast<os::LayoutSpacer*>(pcNode);
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Min size
				pcSpacer->SetMinSize( os::Point( 0, pcProp->GetValue().AsFloat() ) );
			break;
			case 2: // Max size
				pcSpacer->SetMaxSize( os::Point( 0, pcProp->GetValue().AsFloat() ) );
			break;
		}
	}
}

void VLayoutSpacerWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	os::LayoutSpacer* pcSpacer = static_cast<os::LayoutSpacer*>(pcNode);
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::VLayoutSpacer( \"%s\", %f, %f, NULL, %f );\n", 
		pcNode->GetName().c_str(), pcNode->GetName().c_str(), pcSpacer->GetMinSize().y, 
		pcSpacer->GetMaxSize().y, pcNode->GetWeight() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	sprintf( zBuffer, "m_pc%s->AddChild( m_pc%s );\n", pcNode->GetParent()->GetName().c_str(), pcNode->GetName().c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
}

/* FrameView Widget */

const std::type_info* FrameViewWidget::GetTypeID()
{
	return( &typeid( os::FrameView ) );
}

const os::String FrameViewWidget::GetName()
{
	return( "FrameView" );
}

os::LayoutNode* FrameViewWidget::CreateLayoutNode( os::String zName )
{
	os::FrameView* pcFrameView = new os::FrameView( os::Rect(), zName, "Label" );
	LEVLayoutNode* pcNode = new LEVLayoutNode( zName + "Layout", 1.0f, NULL );
	pcFrameView->SetRoot( pcNode );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcFrameView ) );
}

bool FrameViewWidget::CanHaveChildren()
{
	return( true );
}

void FrameViewWidget::AddChild( os::LayoutNode* pcNode, os::LayoutNode* pcChild )
{
	os::FrameView* pcFrameView = static_cast<os::FrameView*>(pcNode->GetView());
	pcFrameView->GetRoot()->AddChild( pcChild );
}
void FrameViewWidget::RemoveChild( os::LayoutNode* pcNode, os::LayoutNode* pcChild )
{
	os::FrameView* pcFrameView = static_cast<os::FrameView*>(pcNode->GetView());
	pcFrameView->GetRoot()->RemoveChild( pcChild );
}

const std::vector<os::LayoutNode*> FrameViewWidget::GetChildList( os::LayoutNode* pcNode )
{
	os::FrameView* pcFrameView = static_cast<os::FrameView*>(pcNode->GetView());
	return( pcFrameView->GetRoot()->GetChildList() );
}

std::vector<WidgetProperty> FrameViewWidget::GetProperties( os::LayoutNode* pcNode )
{
	std::vector<WidgetProperty> cProperties;
	os::FrameView* pcFrameView = static_cast<os::FrameView*>(pcNode->GetView());
	
	cProperties = VLayoutNodeWidget::GetProperties( pcFrameView->GetRoot() );
	
	// Weight
	WidgetProperty cProperty6( 6, PT_FLOAT, "FrameView Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty6 );
	// Label
	WidgetProperty cProperty7( 7, PT_STRING, "Label", pcFrameView->GetLabelString() );
	cProperties.push_back( cProperty7 );

	return( cProperties );
}


void FrameViewWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	os::FrameView* pcFrameView = static_cast<os::FrameView*>(pcNode->GetView());
	
	VLayoutNodeWidget::SetProperties( pcFrameView->GetRoot(), cProperties );
	
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 6: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 7: // Label
				pcFrameView->SetLabel( pcProp->GetValue().AsString() );
			break;
		}
	}
}

void FrameViewWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	os::FrameView* pcFrameView = static_cast<os::FrameView*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::FrameView( os::Rect(), \"%s\", %s );\n"
					  "m_pc%s = new os::VLayoutNode( \"%s\" );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(), ConvertStringToCode( pcFrameView->GetLabelString() ).c_str(), 
						pcFrameView->GetRoot()->GetName().c_str(), pcFrameView->GetRoot()->GetName().c_str() );	
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	os::LayoutNode* pcVNode = pcFrameView->GetRoot();
	sprintf( zBuffer, "m_pc%s->SetBorders( os::Rect( %f, %f, %f, %f ) );\n", pcVNode->GetName().c_str(), 
			pcVNode->GetBorders().left, pcVNode->GetBorders().top, pcVNode->GetBorders().right, pcVNode->GetBorders().bottom );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	CreateAddCode( pcFile, pcNode );
}

void FrameViewWidget::CreateCodeEnd( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	os::FrameView* pcFrameView = static_cast<os::FrameView*>(pcNode->GetView());
	VLayoutNodeWidget::CreateCodeEnd( pcFile, pcFrameView->GetRoot() );
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s->SetRoot( m_pc%s );\n",
						pcNode->GetName().c_str(), pcFrameView->GetRoot()->GetName().c_str() );	
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	
}

void FrameViewWidget::CreateHeaderCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	char nBuffer[8192];
	sprintf( nBuffer, "os::%s* m_pc%s;\nos::VLayoutNode* m_pc%sLayout;\n", GetName().c_str(), pcNode->GetName().c_str(),
	pcNode->GetName().c_str() );
	pcFile->Write( nBuffer, strlen( nBuffer ) );
}


/* TabView Widget */

const std::type_info* TabViewWidget::GetTypeID()
{
	return( &typeid( os::TabView ) );
}

const os::String TabViewWidget::GetName()
{
	return( "TabView" );
}

os::LayoutNode* TabViewWidget::CreateLayoutNode( os::String zName )
{
	os::TabView* pcTabView = new os::TabView( os::Rect(), zName );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcTabView ) );
}

bool TabViewWidget::CanHaveChildren()
{
	return( true );
}

void TabViewWidget::AddChild( os::LayoutNode* pcNode, os::LayoutNode* pcChild )
{
	os::TabView* pcTabView = static_cast<os::TabView*>(pcNode->GetView());
	
	os::LayoutView* pcView = new os::LayoutView( os::Rect(), pcChild->GetName() + "Tab" );
	os::VLayoutNode* pcVNode = new os::VLayoutNode( pcChild->GetName() + "Layout" );
	
	pcTabView->AppendTab( pcChild->GetName(), pcView );
	pcVNode->AddChild( pcChild );
	pcView->SetRoot( pcVNode );
}
void TabViewWidget::RemoveChild( os::LayoutNode* pcNode, os::LayoutNode* pcChild )
{
	os::TabView* pcTabView = static_cast<os::TabView*>(pcNode->GetView());
	
	for( int i = 0; i < pcTabView->GetTabCount(); i++ )
	{
		os::LayoutView* pcView = static_cast<os::LayoutView*>(pcTabView->GetTabView( i ) );
		if( pcView->GetRoot()->GetChildList()[0] == pcChild )
		{
			pcView->GetRoot()->RemoveChild( pcChild );
			delete( pcTabView->DeleteTab( i ) );
			pcTabView->Invalidate( true );
			pcTabView->Flush();
			return;
		}
	}
	pcTabView->Invalidate( true );
	pcTabView->Flush();
	printf( "Error: Tab not found!\n" );
}

const std::vector<os::LayoutNode*> TabViewWidget::GetChildList( os::LayoutNode* pcNode )
{
	os::TabView* pcTabView = static_cast<os::TabView*>(pcNode->GetView());
	std::vector<os::LayoutNode*> cList;
	
	for( int i = 0; i < pcTabView->GetTabCount(); i++ )
	{
		os::LayoutView* pcView = static_cast<os::LayoutView*>(pcTabView->GetTabView( i ) );
		cList.push_back( pcView->GetRoot()->GetChildList()[0] );
	}
	return( cList );
}

std::vector<WidgetProperty> TabViewWidget::GetProperties( os::LayoutNode* pcNode )
{
	std::vector<WidgetProperty> cProperties;
	
	//cProperties = VLayoutNodeWidget::GetProperties( pcFrameView->GetRoot() );
	
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "TabView Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );

	return( cProperties );
}


void TabViewWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	
	//VLayoutNodeWidget::SetProperties( pcFrameView->GetRoot(), cProperties );
	
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
		}
	}
}

void TabViewWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	os::TabView* pcTabView = static_cast<os::TabView*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::TabView( os::Rect(), \"%s\" );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	
	for( int i = 0; i < pcTabView->GetTabCount(); i++ )
	{
		os::LayoutView* pcView = static_cast<os::LayoutView*>(pcTabView->GetTabView( i ) );
		
		sprintf( zBuffer, "m_pc%s = new os::LayoutView( os::Rect(), \"%s\" );\n"
						  "m_pc%s = new os::VLayoutNode( \"%s\" );\n"
						  "m_pc%s->AppendTab(  \"%s\", m_pc%s );\n",
						pcView->GetName().c_str(), pcView->GetName().c_str(),
						pcView->GetRoot()->GetName().c_str(), pcView->GetRoot()->GetName().c_str(),
						pcNode->GetName().c_str(), 
						pcView->GetRoot()->GetChildList()[0]->GetName().c_str(),
						pcView->GetName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
	}
	CreateAddCode( pcFile, pcNode );
}

void TabViewWidget::CreateCodeEnd( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	os::TabView* pcTabView = static_cast<os::TabView*>(pcNode->GetView());
	char zBuffer[8192];
	for( int i = 0; i < pcTabView->GetTabCount(); i++ )
	{
		os::LayoutView* pcView = static_cast<os::LayoutView*>(pcTabView->GetTabView( i ) );
		
		sprintf( zBuffer, "m_pc%s->SetRoot( m_pc%s );\n",
						pcView->GetName().c_str(), pcView->GetRoot()->GetName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
	}
}

void TabViewWidget::CreateHeaderCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	os::TabView* pcTabView = static_cast<os::TabView*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "os::%s* m_pc%s;\n", GetName().c_str(), pcNode->GetName().c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	for( int i = 0; i < pcTabView->GetTabCount(); i++ )
	{
		os::LayoutView* pcView = static_cast<os::LayoutView*>(pcTabView->GetTabView( i ) );
		
		sprintf( zBuffer, "os::LayoutView* m_pc%s;\nos::VLayoutNode* m_pc%s;\n",
						pcView->GetName().c_str(), pcView->GetRoot()->GetName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
	}
}



























