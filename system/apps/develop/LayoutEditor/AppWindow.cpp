#include "AppWindow.h"
#include "Widgets.h"
#include "WidgetsNodes.h"
#include "messages.h"
#include "mainwindow.h"

#include <gui/button.h>
#include <gui/image.h>
#include <util/resources.h>

AppWindow::AppWindow( MainWindow* pcMain ) : os::Window( os::Rect( 0, 0, 300, 300 ), "app_wnd", "LayoutEditor : Preview" )
{
	m_pcMain = pcMain;
	
	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
	
	/* Create layout view and the root node */
	m_pcRoot = new os::VLayoutNode( "Root" );
	m_pcLayoutView = new os::LayoutView( GetBounds(), "layout_view" );
	

	m_pcLayoutView->SetRoot( m_pcRoot );
	
	AddChild( m_pcLayoutView );

}

void AppWindow::ReLayout( os::LayoutNode* pcParentNode )
{
	/* Relayout the nodes and all integrated layoutviews */
	std::vector<os::LayoutNode*> apcWidgets = pcParentNode->GetChildList();
	for( uint i = 0; i < apcWidgets.size(); i++ )
	{
		os::LayoutNode* pcNode = apcWidgets[i];
		
		/* Get the widget belonging to this node */
		Widget* pcWidget = m_pcMain->GetNodeWidget( pcNode );
		
		if( pcWidget == NULL )
		{
			continue;
		}
		
		if( pcNode->GetView() != NULL )
		{
			os::LayoutView* pcView = dynamic_cast<os::LayoutView*>( pcNode->GetView() );
			if( pcView != NULL )
			{
				pcView->InvalidateLayout();
			}
		}
		ReLayout( pcWidget->GetSubNode( pcNode ) );
	}
}

void AppWindow::ReLayout()
{
	GetRootNode()->Layout();
	m_pcLayoutView->Invalidate();
	ReLayout( m_pcRoot );
	Flush();
}

void AppWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		break;
	}
}

bool AppWindow::OkToQuit()
{
	return( false );
}





















