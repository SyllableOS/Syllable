#include "Widgets.h"
#include "WidgetsMenu.h"

#include <gui/menu.h>


/* Menu Widget */

const std::type_info* MenuWidget::GetTypeID()
{
	return( &typeid( os::Menu ) );
}

const os::String MenuWidget::GetName()
{
	return( "Menu" );
}

os::LayoutNode* MenuWidget::CreateLayoutNode( os::String zName )
{
	os::Menu* pcView = new os::Menu( os::Rect(), zName, os::ITEMS_IN_ROW );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcView ) );
}

std::vector<WidgetProperty> MenuWidget::GetProperties( os::LayoutNode* pcNode )
{
	os::Menu* pcView = static_cast<os::Menu*>(pcNode->GetView());
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// Items
	WidgetProperty cProperty1( 1, PT_STRING_LIST, "Items", os::String( "Click to edit" ) );
	for( int i = 0; i < pcView->GetItemCount(); ++i )
		cProperty1.AddSelection( pcView->GetItemAt( i )->GetLabel() );
	cProperties.push_back( cProperty1 );
	return( cProperties );
}
void MenuWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	os::Menu* pcView = static_cast<os::Menu*>(pcNode->GetView());
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Items
			
				while( pcView->GetItemAt( 0 ) != NULL )
					delete( pcView->RemoveItem( 0 ) );
				int nIndex = 0;
				os::String zString;
				while( pcProp->GetSelection( &zString, nIndex ) )
				{
					pcView->AddItem( zString, new os::Message( -1 ) );
					nIndex++;
				}
				pcView->InvalidateLayout();
				pcView->Invalidate();
				pcView->Flush();
			break;
		}
	}
}

void MenuWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	os::Menu* pcView = static_cast<os::Menu*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::Menu( os::Rect(), \"%s\", os::ITEMS_IN_ROW );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	for( int i = 0; i < pcView->GetItemCount(); ++i )
	{
		sprintf( zBuffer, "os::Menu* pc%s%sMenu = new os::Menu( os::Rect(), \"%s\", os::ITEMS_IN_COLUMN );\n"
							"m_pc%s->AddItem( pc%s%sMenu );\n",
						pcNode->GetName().c_str(), ConvertStringToCode( pcView->GetItemAt( i )->GetLabel() ).c_str(),
						ConvertStringToCode( pcView->GetItemAt( i )->GetLabel() ).c_str(),
						pcNode->GetName().c_str(), pcNode->GetName().c_str(),
						ConvertStringToCode( pcView->GetItemAt( i )->GetLabel() ).c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
	}
	CreateAddCode( pcFile, pcNode );
}



