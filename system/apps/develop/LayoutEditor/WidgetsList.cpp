#include "Widgets.h"
#include "WidgetsList.h"

#include <gui/listview.h>
#include <gui/slider.h>


/* ListView Widget */

const std::type_info* ListViewWidget::GetTypeID()
{
	return( &typeid( os::ListView ) );
}

const os::String ListViewWidget::GetName()
{
	return( "ListView" );
}

os::LayoutNode* ListViewWidget::CreateLayoutNode( os::String zName )
{
	os::ListView* pcView = new os::ListView( os::Rect(), zName );
	pcView->InsertColumn( "Column", 1000 );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcView ) );
}

std::vector<WidgetProperty> ListViewWidget::GetProperties( os::LayoutNode* pcNode )
{
	os::ListView* pcView = static_cast<os::ListView*>(pcNode->GetView());
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// MultiSelect
	WidgetProperty cProperty1( 1, PT_BOOL, "Multi Select", pcView->IsMultiSelect() );
	cProperties.push_back( cProperty1 );
	// AutoSort
	WidgetProperty cProperty2( 2, PT_BOOL, "Auto Sort", pcView->IsAutoSort() );
	cProperties.push_back( cProperty2 );
	// Render Border
	WidgetProperty cProperty3( 3, PT_BOOL, "Render Border", pcView->HasBorder() );
	cProperties.push_back( cProperty3 );
	// Column Header
	WidgetProperty cProperty4( 4, PT_BOOL, "Column Header", pcView->HasColumnHeader() );
	cProperties.push_back( cProperty4 );
	return( cProperties );
}
void ListViewWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	os::ListView* pcView = static_cast<os::ListView*>(pcNode->GetView());
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // MultiSelect
				pcView->SetMultiSelect( pcProp->GetValue().AsBool() );
			break;
			case 2: // AutoSort
				pcView->SetAutoSort( pcProp->GetValue().AsBool() );
			break;
			case 3: // RenderBorder
				pcView->SetRenderBorder( pcProp->GetValue().AsBool() );
			break;
			case 4: // ColumnHeader
				pcView->SetHasColumnHeader( pcProp->GetValue().AsBool() );
			break;
		}
	}
}

static void AddFlag( os::String& zString, os::String zAdd, bool& bFirst )
{
	if( !bFirst ) {
		zString += " | ";
	}
	bFirst = false;
	zString += zAdd;
}

void ListViewWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	os::ListView* pcView = static_cast<os::ListView*>(pcNode->GetView());
	char zBuffer[8192];
	os::String zFlags;
	bool bFirst = true;
	if( pcView->IsMultiSelect() )
		AddFlag( zFlags, "os::ListView::F_MULTI_SELECT", bFirst );
	if( !pcView->IsAutoSort() )
		AddFlag( zFlags, "os::ListView::F_NO_AUTO_SORT", bFirst );
	if( pcView->HasBorder() )
		AddFlag( zFlags, "os::ListView::F_RENDER_BORDER", bFirst );
	if( !pcView->HasColumnHeader() )
		AddFlag( zFlags, "os::ListView::F_NO_HEADER", bFirst );
	
		
	sprintf( zBuffer, "m_pc%s = new os::ListView( os::Rect(), \"%s\", %s );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(),
						zFlags.c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	CreateAddCode( pcFile, pcNode );
}











