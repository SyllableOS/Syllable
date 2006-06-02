#include "Widgets.h"
#include "WidgetsUtils.h"

#include <gui/separator.h>
#include <gui/slider.h>


/* Separator Widget */

const std::type_info* SeparatorWidget::GetTypeID()
{
	return( &typeid( os::Separator ) );
}

const os::String SeparatorWidget::GetName()
{
	return( "Separator" );
}

os::LayoutNode* SeparatorWidget::CreateLayoutNode( os::String zName )
{
	os::Separator* pcView = new os::Separator( os::Rect(), zName );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcView ) );
}

std::vector<WidgetProperty> SeparatorWidget::GetProperties( os::LayoutNode* pcNode )
{
	os::Separator* pcView = static_cast<os::Separator*>(pcNode->GetView());
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// Orientation
	WidgetProperty cProperty1( 1, PT_STRING_SELECT, "Orientation", pcView->GetOrientation() == os::VERTICAL ? 
							os::String( "Vertical" ) : os::String( "Horizontal" ) );
	cProperty1.AddSelection( "Horizontal" );
	cProperty1.AddSelection( "Vertical" );
	cProperties.push_back( cProperty1 );
	return( cProperties );
}
void SeparatorWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	os::Separator* pcView = static_cast<os::Separator*>(pcNode->GetView());
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Orientation
				pcView->SetOrientation( pcProp->GetValue().AsString() == "Vertical" ?
						os::VERTICAL : os::HORIZONTAL );
			break;
		}
	}
}

void SeparatorWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	os::Separator* pcView = static_cast<os::Separator*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::Separator( os::Rect(), \"%s\", %s );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(),
						pcView->GetOrientation() == os::VERTICAL ? "os::VERTICAL" : "os::HORIZONTAL" );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	CreateAddCode( pcFile, pcNode );
}

/* Slider Widget */


static os::String PositionToString( uint32 nPos )
{
	switch( nPos )
	{
		case os::Slider::TICKS_ABOVE:
			return( "Above/Left" );
		case os::Slider::TICKS_BELOW:
			return( "Below/Right" );
		default:
			return( "Unknown" );
	}
}

static os::String PositionToCode( uint32 nPos )
{
	switch( nPos )
	{
		case os::Slider::TICKS_ABOVE:
			return( "os::Slider::TICKS_ABOVE" );
		case os::Slider::TICKS_BELOW:
			return( "os::Slider::TICKS_BELOW" );
		default:
			return( "Unknown" );
	}
}

static uint32 StringToPosition( os::String zString )
{
	if( zString == "Above/Left" )
		return( os::Slider::TICKS_ABOVE );
	else if( zString == "Below/Right" )
		return( os::Slider::TICKS_BELOW );

	return( os::Slider::TICKS_BELOW );
}

class LESlider : public os::Slider
{
public:
	LESlider( os::Rect cFrame, const os::String& cName, os::Message* pcMsg, os::orientation eOrientation )
				: os::Slider( cFrame, cName, pcMsg, os::Slider::TICKS_BELOW, 10,
					os::Slider::KNOB_SQUARE, eOrientation )
	{
		m_zMessageCode = "-1";
		m_vMin = 0.0f;
		m_vMax = 1.0f;
	}
	os::String m_zMessageCode;
	float m_vMin;
	float m_vMax;
};

class LEHSlider : public LESlider
{
public:
	LEHSlider( os::Rect cFrame, const os::String& cName, os::Message* pcMsg )
				: LESlider( cFrame, cName, pcMsg, os::HORIZONTAL )
	{
	}
};

class LEVSlider : public LESlider
{
public:
	LEVSlider( os::Rect cFrame, const os::String& cName, os::Message* pcMsg )
				: LESlider( cFrame, cName, pcMsg, os::VERTICAL )
	{
	}
};


std::vector<WidgetProperty> SliderWidget::GetProperties( os::LayoutNode* pcNode )
{
	LESlider* pcView = static_cast<LESlider*>(pcNode->GetView());
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// Step count
	WidgetProperty cProperty1( 1, PT_INT, "Step count", pcView->GetStepCount() );
	cProperties.push_back( cProperty1 );
	// Tick count
	WidgetProperty cProperty2( 2, PT_INT, "Tick count", pcView->GetTickCount() );
	cProperties.push_back( cProperty2 );
	// Ticks position
	WidgetProperty cProperty3( 3, PT_STRING_SELECT, "Ticks position", PositionToString( pcView->GetTickFlags() ) );
	cProperty3.AddSelection( "Above/Left" );
	cProperty3.AddSelection( "Below/Right" );
	cProperties.push_back( cProperty3 );
	// Minimal value
	WidgetProperty cProperty4( 4, PT_FLOAT, "Minimal value", pcView->m_vMin );
	cProperties.push_back( cProperty4 );
	// Maximal value
	WidgetProperty cProperty5( 5, PT_FLOAT, "Maximal value", pcView->m_vMax );
	cProperties.push_back( cProperty5 );
	// Minimal Label
	os::String cMinLabel, cMaxLabel;
	pcView->GetLimitLabels( &cMinLabel, &cMaxLabel );
	WidgetProperty cProperty6( 6, PT_STRING, "Minimal label", cMinLabel );
	cProperties.push_back( cProperty6 );
	WidgetProperty cProperty7( 7, PT_STRING, "Maximal label", cMaxLabel );
	cProperties.push_back( cProperty7 );
	// Message code
	WidgetProperty cProperty8( 8, PT_STRING, "Message Code", pcView->m_zMessageCode );
	cProperties.push_back( cProperty8 );
	return( cProperties );
}
void SliderWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	LESlider* pcView = static_cast<LESlider*>(pcNode->GetView());
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Step count
				pcView->SetStepCount( pcProp->GetValue().AsInt32() );
			break;
			case 2: // Tick count
				pcView->SetTickCount( pcProp->GetValue().AsInt32() );
			break;
			case 3: // Ticks position
				pcView->SetTickFlags( StringToPosition( pcProp->GetValue().AsString() ) );
			break;
			case 4: // Minimal value
				pcView->m_vMin = pcProp->GetValue().AsFloat();
				pcView->SetMinMax( pcView->m_vMin, pcView->m_vMax );
			break;
			case 5: // Maximal value
				pcView->m_vMax = pcProp->GetValue().AsFloat();
				pcView->SetMinMax( pcView->m_vMin, pcView->m_vMax );
			break;
			case 6: // Minimal label
			{
				os::String cMinLabel, cMaxLabel;
				pcView->GetLimitLabels( &cMinLabel, &cMaxLabel );
				pcView->SetLimitLabels( pcProp->GetValue().AsString(), cMaxLabel );
			}
			break;
			case 7: // Maximal label
			{
				os::String cMinLabel, cMaxLabel;
				pcView->GetLimitLabels( &cMinLabel, &cMaxLabel );
				pcView->SetLimitLabels( cMinLabel, pcProp->GetValue().AsString() );
			}
			break;
			case 8: // Message Code
				pcView->m_zMessageCode = pcProp->GetValue().AsString();
			break;
		}
	}
}

void SliderWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	const std::type_info* psInfo;
	psInfo = &typeid( *(pcNode->GetView()) );
	LESlider* pcView = static_cast<LESlider*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "m_pc%s = new os::Slider( os::Rect(), \"%s\", new os::Message( %s ), %s, 10, os::Slider::KNOB_SQUARE, %s );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(), pcView->m_zMessageCode.c_str(),
						PositionToCode( pcView->GetTickFlags() ).c_str(), 
						psInfo == &typeid( LEHSlider ) ? "os::HORIZONTAL" : "os::VERTICAL" );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	sprintf( zBuffer, "m_pc%s->SetStepCount( %i );\n", pcNode->GetName().c_str(), pcView->GetStepCount() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	sprintf( zBuffer, "m_pc%s->SetTickCount( %i );\n", pcNode->GetName().c_str(), pcView->GetTickCount() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	os::String cMinLabel, cMaxLabel;
	pcView->GetLimitLabels( &cMinLabel, &cMaxLabel );
	sprintf( zBuffer, "m_pc%s->SetLimitLabels( %s, %s );\n", pcNode->GetName().c_str(), ConvertString( cMinLabel ).c_str(), ConvertString( cMaxLabel ).c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	sprintf( zBuffer, "m_pc%s->SetMinMax( %f, %f );\n", pcNode->GetName().c_str(), pcView->m_vMin, pcView->m_vMax );
	pcFile->Write( zBuffer, strlen( zBuffer ) );

	CreateAddCode( pcFile, pcNode );
}


const std::type_info* HorizontalSliderWidget::GetTypeID()
{
	return( &typeid( LEHSlider ) );
}

const os::String HorizontalSliderWidget::GetName()
{
	return( "Horizontal Slider" );
}

const os::String HorizontalSliderWidget::GetCodeName()
{
	return( "Slider" );
}

os::LayoutNode* HorizontalSliderWidget::CreateLayoutNode( os::String zName )
{
	LESlider* pcView = new LEHSlider( os::Rect(), zName, new os::Message( 0 ) );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcView ) );
}

const std::type_info* VerticalSliderWidget::GetTypeID()
{
	return( &typeid( LEVSlider ) );
}

const os::String VerticalSliderWidget::GetName()
{
	return( "Vertical Slider" );
}

const os::String VerticalSliderWidget::GetCodeName()
{
	return( "Slider" );
}


os::LayoutNode* VerticalSliderWidget::CreateLayoutNode( os::String zName )
{
	LESlider* pcView = new LEVSlider( os::Rect(), zName, new os::Message( 0 ) );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcView ) );
}





