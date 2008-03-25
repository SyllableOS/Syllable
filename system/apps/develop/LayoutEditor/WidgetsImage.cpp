#include "Widgets.h"
#include "WidgetsImage.h"
#include "mainwindow.h"

#include <gui/imageview.h>
#include <gui/imagebutton.h>
#include <storage/path.h>

class LEImageView : public os::ImageView
{
public:
	LEImageView( os::Rect cFrame, const os::String& cName, os::Image* pcImage )
				: os::ImageView( cFrame, cName, pcImage )
	{
		m_cFile = "";
	}
	~LEImageView()
	{
	}
	void SetFile( os::String cPath )
	{
		m_cFile = cPath;
	}
	os::String GetFile()
	{
		return( m_cFile );
	}
	os::String GetFileName()
	{
		return( os::Path( m_cFile ).GetLeaf() );
	}
	os::Path GetFile( os::String cProject, os::String cFile )
	{
		os::Path cPath( cProject );
		cPath = cPath.GetDir();
		cPath.Append( cFile );
		return( cPath );
	}
private:
	os::String m_cFile;
};

/* ImageView Widget */

const std::type_info* ImageViewWidget::GetTypeID()
{
	return( &typeid( LEImageView ) );
}

const os::String ImageViewWidget::GetName()
{
	return( "ImageView" );
}

os::LayoutNode* ImageViewWidget::CreateLayoutNode( os::String zName )
{	
	LEImageView* pcView = new LEImageView( os::Rect(), zName, NULL );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcView ) );
}

std::vector<WidgetProperty> ImageViewWidget::GetProperties( os::LayoutNode* pcNode )
{
	LEImageView* pcView = static_cast<LEImageView*>(pcNode->GetView());
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// File
	WidgetProperty cProperty1( 1, PT_STRING, "File", pcView->GetFile() );
	cProperties.push_back( cProperty1 );
	return( cProperties );
}
void ImageViewWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	LEImageView* pcView = static_cast<LEImageView*>(pcNode->GetView());
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // File
			{
				if( pcProp->GetValue().AsString() == "" )
				{
					pcView->SetFile( "" );
					delete( pcView->GetImage() );
					pcView->SetImage( NULL );
					break;
				}
				/* Try to open the file */
				os::File cFile;
				if( cFile.SetTo( pcView->GetFile( MainWindow::GetFileName(), pcProp->GetValue().AsString() ) ) != 0 )
					break;
				/* Create image */
				os::BitmapImage* pcImage = new os::BitmapImage();
				if( pcImage->Load( &cFile ) != 0 ) {
					delete( pcImage );
					break;
				}
				/* Set the imageview to the new image */
				delete( pcView->GetImage() );
				pcView->SetImage( pcImage );
				pcView->SetFile( pcProp->GetValue().AsString() );				
			}
			break;
		}
	}
}

void ImageViewWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	LEImageView* pcView = static_cast<LEImageView*>(pcNode->GetView());
	char zBuffer[8192];
	sprintf( zBuffer, "os::File c%sFile( open_image_file( get_image_id() ) );\n",
						pcNode->GetName().c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );	
	sprintf( zBuffer, "os::Resources c%sResources( &c%sFile );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );						
	sprintf( zBuffer, "os::ResStream* pc%sStream = c%sResources.GetResourceStream( \"%s\" );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(), pcView->GetFileName().c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );		
	sprintf( zBuffer, "m_pc%s = new os::ImageView( os::Rect(), \"%s\", new os::BitmapImage( pc%sStream ) );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(), pcNode->GetName().c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );
	sprintf( zBuffer, "delete( pc%sStream );\n",
						pcNode->GetName().c_str() );
	pcFile->Write( zBuffer, strlen( zBuffer ) );		
	CreateAddCode( pcFile, pcNode );
}

class LEImageButton : public os::ImageButton
{
public:
	LEImageButton( os::Rect cFrame, const os::String& cName, const os::String& cLabel, os::Message* pcMessage, os::Image* pcImage )
				: os::ImageButton( cFrame, cName, cLabel, pcMessage, pcImage,
									os::ImageButton::IB_TEXT_BOTTOM, true, true, true )
	{
		m_cFile = "";
		m_zMessageCode = "-1";
		m_cRealLabel = cLabel;
	}
	~LEImageButton()
	{
	}
	void SetFile( os::String cPath )
	{
		m_cFile = cPath;
	}
	os::String GetFile()
	{
		return( m_cFile );
	}
	os::String GetFileName()
	{
		return( os::Path( m_cFile ).GetLeaf() );
	}
	os::Path GetFile( os::String cProject, os::String cFile )
	{
		os::Path cPath( cProject );
		cPath = cPath.GetDir();
		cPath.Append( cFile );
		return( cPath );
	}
	os::String m_cFile;
	os::String m_zMessageCode;
	os::String m_cRealLabel;
};

/* ImageButton Widget */

const std::type_info* ImageButtonWidget::GetTypeID()
{
	return( &typeid( LEImageButton ) );
}

const os::String ImageButtonWidget::GetName()
{
	return( "ImageButton" );
}

os::LayoutNode* ImageButtonWidget::CreateLayoutNode( os::String zName )
{	
	LEImageButton* pcView = new LEImageButton( os::Rect(), zName, "Label", new os::Message( -1 ), NULL );
	return( new os::LayoutNode( zName, 1.0f, NULL, pcView ) );
}

static os::String PositionToString( uint32 nPos )
{
	switch( nPos )
	{
		case os::ImageButton::IB_TEXT_LEFT:
			return( "Left" );
		case os::ImageButton::IB_TEXT_RIGHT:
			return( "Right" );
		case os::ImageButton::IB_TEXT_TOP:
			return( "Top" );
		case os::ImageButton::IB_TEXT_BOTTOM:
			return( "Bottom" );
		default:
			return( "Unknown" );
	}
}

static os::String PositionToCode( uint32 nPos )
{
	switch( nPos )
	{
		case os::ImageButton::IB_TEXT_LEFT:
			return( "os::ImageButton::IB_TEXT_LEFT" );
		case os::ImageButton::IB_TEXT_RIGHT:
			return( "os::ImageButton::IB_TEXT_RIGHT" );
		case os::ImageButton::IB_TEXT_TOP:
			return( "os::ImageButton::IB_TEXT_TOP" );
		case os::ImageButton::IB_TEXT_BOTTOM:
			return( "os::ImageButton::IB_TEXT_BOTTOM" );
		default:
			return( "os::ImageButton::IB_TEXT_BOTTOM" );
	}
}

static uint32 StringToPosition( os::String zString )
{
	if( zString == "Left" )
		return( os::ImageButton::IB_TEXT_LEFT );
	else if( zString == "Right" )
		return( os::ImageButton::IB_TEXT_RIGHT );
	else if( zString == "Top" )
		return( os::ImageButton::IB_TEXT_TOP );
	else if( zString == "Bottom" )
		return( os::ImageButton::IB_TEXT_BOTTOM );

	return( os::ImageButton::IB_TEXT_RIGHT );
}

std::vector<WidgetProperty> ImageButtonWidget::GetProperties( os::LayoutNode* pcNode )
{
	LEImageButton* pcView = static_cast<LEImageButton*>(pcNode->GetView());
	std::vector<WidgetProperty> cProperties;
	// Weight
	WidgetProperty cProperty0( 0, PT_FLOAT, "Weight", pcNode->GetWeight() );
	cProperties.push_back( cProperty0 );
	// Label
	WidgetProperty cProperty1( 1, PT_STRING_CATALOG, "Label", pcView->m_cRealLabel );
	cProperties.push_back( cProperty1 );
	// Message code
	WidgetProperty cProperty2( 2, PT_STRING, "Message Code", pcView->m_zMessageCode );
	cProperties.push_back( cProperty2 );
	// File
	WidgetProperty cProperty3( 3, PT_STRING, "File", pcView->GetFile() );
	cProperties.push_back( cProperty3 );
	// Text position
	WidgetProperty cProperty4( 4, PT_STRING_SELECT, "Text position", PositionToString( pcView->GetTextPosition() ) );
	cProperty4.AddSelection( "Left" );
	cProperty4.AddSelection( "Right" );
	cProperty4.AddSelection( "Top" );
	cProperty4.AddSelection( "Bottom" );
	cProperties.push_back( cProperty4 );
	return( cProperties );
}
void ImageButtonWidget::SetProperties( os::LayoutNode* pcNode, std::vector<WidgetProperty> cProperties )
{
	LEImageButton* pcView = static_cast<LEImageButton*>(pcNode->GetView());
	for( uint i = 0; i < cProperties.size(); i++ )
	{
		WidgetProperty* pcProp = &cProperties[i];
		switch( pcProp->GetCode() )
		{
			case 0: // Weight
				pcNode->SetWeight( pcProp->GetValue().AsFloat() );
			break;
			case 1: // Label
				pcView->m_cRealLabel = pcProp->GetValue().AsString();
				pcView->SetLabel( GetString( pcView->m_cRealLabel ) );
			break;
			case 2: // Message Code
				pcView->m_zMessageCode = pcProp->GetValue().AsString();
			break;
			case 3: // File
			{
				if( pcProp->GetValue().AsString() == "" )
				{
					pcView->SetFile( "" );
					pcView->ClearImage();
					break;
				}
				/* Try to open the file */
				os::File cFile;
				if( cFile.SetTo( pcView->GetFile( MainWindow::GetFileName(), pcProp->GetValue().AsString() ) ) != 0 )
					break;
				/* Create image */
				os::BitmapImage* pcImage = new os::BitmapImage();
				if( pcImage->Load( &cFile ) != 0 ) {
					delete( pcImage );
					break;
				}
				/* Set the imageview to the new image */
				pcView->SetImage( pcImage );
				pcView->Invalidate();
				pcView->SetFile( pcProp->GetValue().AsString() );				
			}
			break;
			case 4: // Text position
				pcView->SetTextPosition( StringToPosition( pcProp->GetValue().AsString() ) );
			break;
		}
	}
}

void ImageButtonWidget::CreateCode( os::StreamableIO* pcFile, os::LayoutNode* pcNode )
{
	LEImageButton* pcView = static_cast<LEImageButton*>(pcNode->GetView());
	char zBuffer[8192];
	if( pcView->GetFile() == "" )
	{
		sprintf( zBuffer, "m_pc%s = new os::ImageButton( os::Rect(), \"%s\", %s, new os::Message( %s ), NULL"
		", %s, true, true, true );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(), ConvertStringToCode( pcView->m_cRealLabel ).c_str(), 
						pcView->m_zMessageCode.c_str(), PositionToCode( pcView->GetTextPosition() ).c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
	}
	else
	{
		sprintf( zBuffer, "os::File c%sFile( open_image_file( get_image_id() ) );\n",
						pcNode->GetName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );	
		sprintf( zBuffer, "os::Resources c%sResources( &c%sFile );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );						
		sprintf( zBuffer, "os::ResStream* pc%sStream = c%sResources.GetResourceStream( \"%s\" );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(), pcView->GetFileName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );		
		sprintf( zBuffer, "m_pc%s = new os::ImageButton( os::Rect(), \"%s\", %s, new os::Message( %s ), new os::BitmapImage( pc%sStream )"
		", %s, true, true, true );\n",
						pcNode->GetName().c_str(), pcNode->GetName().c_str(), ConvertStringToCode( pcView->m_cRealLabel ).c_str(), 
						pcView->m_zMessageCode.c_str(), pcNode->GetName().c_str(), PositionToCode( pcView->GetTextPosition() ).c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );
		sprintf( zBuffer, "delete( pc%sStream );\n",
						pcNode->GetName().c_str() );
		pcFile->Write( zBuffer, strlen( zBuffer ) );		
		
	}
	CreateAddCode( pcFile, pcNode );
}









