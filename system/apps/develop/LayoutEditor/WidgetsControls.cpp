#include "Widgets.h"
#include "WidgetsControls.h"

#include <gui/stringview.h>
#include <gui/button.h>
#include <gui/checkbox.h>
#include <gui/radiobutton.h>
#include <gui/dropdownmenu.h>
#include <gui/textview.h>
#include <util/message.h>

/* StringView Widget */

const std::type_info* StringViewWidget::GetTypeID()
{
	return( &typeid( os::StringView ) );
}

const os::String StringViewWidget::GetName()
{
	return( "StringView" );
}

os::LayoutNode* StringViewWidget::CreateLayoutNode( os::String zName )
{
	os::StringView* pcView = new os::StringView( os::Rect(), zName, "Label" );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcView ) );
}

std::vector<WidgetProperty> StringViewWidget::GetProperties( os::LayoutNode* pcNode )
{
	os::StringView* pcView = static_cast<os::StringView*>(pcNode->GetView());
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// Label
	WidgetProperty cProperty1( 1, PT_STRING, "Label", pcView->GetString() );
	cProperties.push_back( cProperty1 );
	return( cProperties );
}
void StringViewWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	os::StringView* pcView = static_cast<os::StringView*>(pcNode->GetView());
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Label
				pcView->SetString( pcProp->GetValue().AsString() );
			break;
		}
	}
}

void StringViewWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	os::StringView* pcView = static_cast<os::StringView*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::StringView( os::Rect(), \"%s\", %s );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(), ConvertString( pcView->GetString() ).c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	CreateAddCode( pcFile, pcNode );
}


/* Button Widget */

class LEButton : public os::Button
{
public:
	LEButton( os::Rect cFrame, const os::String& cName, const os::String& cLabel, os::Message* pcMessage )
				: os::Button( cFrame, cName, cLabel, pcMessage )
	{
		m_zMessageCode = "-1";
	}
	os::String m_zMessageCode;
};

const std::type_info* ButtonWidget::GetTypeID()
{
	return( &typeid( LEButton ) );
}

const os::String ButtonWidget::GetName()
{
	return( "Button" );
}

os::LayoutNode* ButtonWidget::CreateLayoutNode( os::String zName )
{
	LEButton* pcButton = new LEButton( os::Rect(), zName, "Label", new os::Message( -1 ) );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcButton ) );
}

std::vector<WidgetProperty> ButtonWidget::GetProperties( os::LayoutNode* pcNode )
{
	LEButton* pcButton = static_cast<LEButton*>(pcNode->GetView());
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// Label
	WidgetProperty cProperty1( 1, PT_STRING, "Label", pcButton->GetLabel() );
	cProperties.push_back( cProperty1 );
	// Message code
	WidgetProperty cProperty2( 2, PT_STRING, "Message Code", pcButton->m_zMessageCode );
	cProperties.push_back( cProperty2 );
	return( cProperties );
}
void ButtonWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	LEButton* pcButton = static_cast<LEButton*>(pcNode->GetView());
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Label
				pcButton->SetLabel( pcProp->GetValue().AsString() );
			break;
			case 2: // Message Code
				pcButton->m_zMessageCode = pcProp->GetValue().AsString();
			break;
		}
	}
}

void ButtonWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	LEButton* pcButton = static_cast<LEButton*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::Button( os::Rect(), \"%s\", %s, new os::Message( %s ) );\n", pcNode->GetName().c_str(),
			pcNode->GetName().c_str(), ConvertString( pcButton->GetLabel() ).c_str(), pcButton->m_zMessageCode.c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	CreateAddCode( pcFile, pcNode );
}

/* CheckBox Widget */

class LECheckBox : public os::CheckBox
{
public:
	LECheckBox( os::Rect cFrame, const os::String& cName, const os::String& cLabel, os::Message* pcMessage )
				: os::CheckBox( cFrame, cName, cLabel, pcMessage )
	{
		m_zMessageCode = "-1";
	}
	os::String m_zMessageCode;
};

const std::type_info* CheckBoxWidget::GetTypeID()
{
	return( &typeid( LECheckBox ) );
}

const os::String CheckBoxWidget::GetName()
{
	return( "CheckBox" );
}

os::LayoutNode* CheckBoxWidget::CreateLayoutNode( os::String zName )
{
	LECheckBox* pcCheckBox = new LECheckBox( os::Rect(), zName, "Label", new os::Message( -1 ) );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcCheckBox ) );
}

std::vector<WidgetProperty> CheckBoxWidget::GetProperties( os::LayoutNode* pcNode )
{
	LECheckBox* pcCheckBox = static_cast<LECheckBox*>(pcNode->GetView());
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// Label
	WidgetProperty cProperty1( 1, PT_STRING, "Label", pcCheckBox->GetLabel() );
	cProperties.push_back( cProperty1 );
	// Checked
	WidgetProperty cProperty2( 2, PT_BOOL, "Checked", pcCheckBox->GetValue() );
	cProperties.push_back( cProperty2 );
	// Message code
	WidgetProperty cProperty3( 3, PT_STRING, "Message Code", pcCheckBox->m_zMessageCode );
	cProperties.push_back( cProperty3 );
	return( cProperties );
}
void CheckBoxWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	LECheckBox* pcCheckBox = static_cast<LECheckBox*>(pcNode->GetView());
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Label
				pcCheckBox->SetLabel( pcProp->GetValue().AsString() );
			break;
			case 2: // Checked
				pcCheckBox->SetValue( pcProp->GetValue() );
			break;
			case 3: // Message Code
				pcCheckBox->m_zMessageCode = pcProp->GetValue().AsString();
			break;
		}
	}
}

void CheckBoxWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	LECheckBox* pcCheckBox = static_cast<LECheckBox*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::CheckBox( os::Rect(), \"%s\", %s, new os::Message( %s ) );\n"
						"m_pc%s->SetValue( %s );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(), ConvertString( pcCheckBox->GetLabel() ).c_str(), 
						pcCheckBox->m_zMessageCode.c_str(), pcNode->GetName().c_str(), 
						pcCheckBox->GetValue().AsBool() == true ? "true" : "false" );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	CreateAddCode( pcFile, pcNode );
}

/* RadioButton Widget */

class LERadioButton : public os::RadioButton
{
public:
	LERadioButton( os::Rect cFrame, const os::String& cName, const os::String& cLabel, os::Message* pcMessage )
				: os::RadioButton( cFrame, cName, cLabel, pcMessage )
	{
		m_zMessageCode = "-1";
	}
	os::String m_zMessageCode;
};

const std::type_info* RadioButtonWidget::GetTypeID()
{
	return( &typeid( LERadioButton ) );
}

const os::String RadioButtonWidget::GetName()
{
	return( "RadioButton" );
}

os::LayoutNode* RadioButtonWidget::CreateLayoutNode( os::String zName )
{
	LERadioButton* pcRadioButton = new LERadioButton( os::Rect(), zName, "Label", new os::Message( -1 ) );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcRadioButton ) );
}

std::vector<WidgetProperty> RadioButtonWidget::GetProperties( os::LayoutNode* pcNode )
{
	LERadioButton* pcRadioButton = static_cast<LERadioButton*>(pcNode->GetView());
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// Label
	WidgetProperty cProperty1( 1, PT_STRING, "Label", pcRadioButton->GetLabel() );
	cProperties.push_back( cProperty1 );
	// Set
	WidgetProperty cProperty2( 2, PT_BOOL, "Set", pcRadioButton->GetValue() );
	cProperties.push_back( cProperty2 );
	// Message code
	WidgetProperty cProperty3( 3, PT_STRING, "Message Code", pcRadioButton->m_zMessageCode );
	cProperties.push_back( cProperty3 );
	return( cProperties );
}
void RadioButtonWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	LERadioButton* pcRadioButton = static_cast<LERadioButton*>(pcNode->GetView());
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Label
				pcRadioButton->SetLabel( pcProp->GetValue().AsString() );
			break;
			case 2: // Set
				pcRadioButton->SetValue( pcProp->GetValue() );
			break;
			case 3: // Message Code
				pcRadioButton->m_zMessageCode = pcProp->GetValue().AsString();
			break;
		}
	}
}

void RadioButtonWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	LERadioButton* pcRadioButton = static_cast<LERadioButton*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::RadioButton( os::Rect(), \"%s\", %s, new os::Message( %s ) );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(), ConvertString( pcRadioButton->GetLabel() ).c_str(), 
						pcRadioButton->m_zMessageCode.c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	if( pcRadioButton->GetValue().AsBool() )
	{
		sprintf( zBuffer, "m_pc%s->SetValue( true );\n",	pcNode->GetName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
	}
	CreateAddCode( pcFile, pcNode );
}

/* DropdownMenu Widget */

class LEDropdownMenu : public os::DropdownMenu
{
public:
	LEDropdownMenu( os::Rect cFrame, const os::String& cName )
				: os::DropdownMenu( cFrame, cName )
	{
		m_zMessageCode = "-1";
	}
	os::String m_zMessageCode;
};

const std::type_info* DropdownMenuWidget::GetTypeID()
{
	return( &typeid( LEDropdownMenu ) );
}

const os::String DropdownMenuWidget::GetName()
{
	return( "DropdownMenu" );
}

os::LayoutNode* DropdownMenuWidget::CreateLayoutNode( os::String zName )
{
	LEDropdownMenu* pcView = new LEDropdownMenu( os::Rect(), zName );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcView ) );
}

std::vector<WidgetProperty> DropdownMenuWidget::GetProperties( os::LayoutNode* pcNode )
{
	LEDropdownMenu* pcDropdownMenu = static_cast<LEDropdownMenu*>(pcNode->GetView());
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// Minimum Size
	WidgetProperty cProperty1( 1, PT_INT, "Minimum Chars", pcDropdownMenu->GetMinPreferredSize() );
	cProperties.push_back( cProperty1 );
	// Maximum Size
	WidgetProperty cProperty2( 2, PT_INT, "Maximum Chars", pcDropdownMenu->GetMaxPreferredSize() );
	cProperties.push_back( cProperty2 );
	// Message code
	WidgetProperty cProperty3( 3, PT_STRING, "Message Code", pcDropdownMenu->m_zMessageCode );
	cProperties.push_back( cProperty3 );
	// Content list
	WidgetProperty cProperty4( 4, PT_STRING_LIST, "Content", os::String( "Click to edit" ) );
	for( int i = 0; i < pcDropdownMenu->GetItemCount(); i++ )
		cProperty4.AddSelection( pcDropdownMenu->GetItem( i ) );
	cProperties.push_back( cProperty4 );
	// ReadOnly
	WidgetProperty cProperty5( 5, PT_BOOL, "ReadOnly", pcDropdownMenu->GetReadOnly() );
	cProperties.push_back( cProperty5 );
	return( cProperties );
}
void DropdownMenuWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	LEDropdownMenu* pcDropdownMenu = static_cast<LEDropdownMenu*>(pcNode->GetView());
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Minimum Size
				pcDropdownMenu->SetMinPreferredSize( pcProp->GetValue().AsInt32() );
			break;
			case 2: // Maximum Size
				pcDropdownMenu->SetMaxPreferredSize( pcProp->GetValue().AsInt32() );
			break;
			case 3: // Message Code
				pcDropdownMenu->m_zMessageCode = pcProp->GetValue().AsString();
			break;
			case 4:
			{
				pcDropdownMenu->Clear();
				int nIndex = 0;
				os::String zString;
				while( pcProp->GetSelection( &zString, nIndex ) )
				{
					pcDropdownMenu->AppendItem( zString );
					nIndex++;
				}
			}
			break;
			case 5: // Read Only
				pcDropdownMenu->SetReadOnly( pcProp->GetValue().AsBool() );
			break;
		}
	}
}

void DropdownMenuWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	LEDropdownMenu* pcDropdownMenu = static_cast<LEDropdownMenu*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::DropdownMenu( os::Rect(), \"%s\" );\n"
					  "m_pc%s->SetSelectionMessage( new os::Message( %s ) );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(),
						pcNode->GetName().c_str(), pcDropdownMenu->m_zMessageCode.c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	if( pcDropdownMenu->GetReadOnly() )
	{
		sprintf( zBuffer, "m_pc%s->SetReadOnly( true );\n",
						pcNode->GetName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
	}
	for( int i = 0; i < pcDropdownMenu->GetItemCount(); i++ )
	{
		sprintf( zBuffer, "m_pc%s->AppendItem( %s );\n",
						pcNode->GetName().c_str(), ConvertString( pcDropdownMenu->GetItem( i ) ).c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
	}
	CreateAddCode( pcFile, pcNode );
}


/* TextView Widget */

const std::type_info* TextViewWidget::GetTypeID()
{
	return( &typeid( os::TextView ) );
}

const os::String TextViewWidget::GetName()
{
	return( "TextView" );
}

os::LayoutNode* TextViewWidget::CreateLayoutNode( os::String zName )
{
	os::TextView* pcView = new os::TextView( os::Rect(), zName, "" );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcView ) );
}

std::vector<WidgetProperty> TextViewWidget::GetProperties( os::LayoutNode* pcNode )
{
	os::TextView* pcView = static_cast<os::TextView*>(pcNode->GetView());
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// Text
	WidgetProperty cProperty1( 1, PT_STRING, "Text", pcView->GetBuffer()[0] );
	cProperties.push_back( cProperty1 );
	// MultiLine
	WidgetProperty cProperty2( 2, PT_BOOL, "MultiLine", pcView->GetMultiLine() );
	cProperties.push_back( cProperty2 );
	// Password
	WidgetProperty cProperty3( 3, PT_BOOL, "Password", pcView->GetPasswordMode() );
	cProperties.push_back( cProperty3 );
	// Numeric
	WidgetProperty cProperty4( 4, PT_BOOL, "Numeric", pcView->GetNumeric() );
	cProperties.push_back( cProperty4 );
	// ReadOnly
	WidgetProperty cProperty5( 5, PT_BOOL, "ReadOnly", pcView->GetReadOnly() );
	cProperties.push_back( cProperty5 );
	return( cProperties );
}
void TextViewWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	os::TextView* pcView = static_cast<os::TextView*>(pcNode->GetView());
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Text
				pcView->SetValue( pcProp->GetValue() );
			break;
			case 2: // MultiLine
				pcView->SetMultiLine( pcProp->GetValue().AsBool() );
			break;
			case 3: // Password
				pcView->SetPasswordMode( pcProp->GetValue().AsBool() );
			break;
			case 4: // Numeric
				pcView->SetNumeric( pcProp->GetValue().AsBool() );
			break;
			case 5: // ReadOnly
				pcView->SetReadOnly( pcProp->GetValue().AsBool() );
			break;
		}
	}
}

void TextViewWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	os::TextView* pcView = static_cast<os::TextView*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::TextView( os::Rect(), \"%s\", %s );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(),
						ConvertString( pcView->GetBuffer()[0] ).c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	if( pcView->GetMultiLine() ) {
		sprintf( zBuffer, "m_pc%s->SetMultiLine( true );\n", pcNode->GetName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
	}
	if( pcView->GetPasswordMode() ) {
		sprintf( zBuffer, "m_pc%s->SetPasswordMode( true );\n", pcNode->GetName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
	}
	if( pcView->GetNumeric() ) {
		sprintf( zBuffer, "m_pc%s->SetNumeric( true );\n", pcNode->GetName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
	}
	if( pcView->GetReadOnly() ) {
		sprintf( zBuffer, "m_pc%s->SetReadOnly( true );\n", pcNode->GetName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
	}
	CreateAddCode( pcFile, pcNode );
}



















