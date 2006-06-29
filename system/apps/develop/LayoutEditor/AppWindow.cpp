#include "AppWindow.h"
#include "Widgets.h"
#include "WidgetsNodes.h"
#include "messages.h"
#include "mainwindow.h"

#include <gui/button.h>
#include <gui/image.h>
#include <gui/tabview.h>
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

void AppWindow::ReLayout( os::LayoutNode* pcNode )
{
	/* Get the widget belonging to this node */
	Widget* pcWidget = m_pcMain->GetNodeWidget( pcNode );
	if( pcNode->GetView() != NULL )
	{
		/* Special handling for embedded layoutviews and tabviews */
		os::LayoutView* pcView = dynamic_cast<os::LayoutView*>( pcNode->GetView() );
		if( pcView != NULL )
		{
			pcView->InvalidateLayout();
		}
		os::TabView* pcTabView = dynamic_cast<os::TabView*>( pcNode->GetView() );
		if( pcTabView != NULL )
		{
			for( int i = 0; i < pcTabView->GetTabCount(); i++ )
			{	
				pcView = dynamic_cast<os::LayoutView*>( pcTabView->GetTabView( i ) );
				pcView->InvalidateLayout();
			}
			return;
		}
	}
	
	/* Relayout the nodes and all integrated layoutviews */
	std::vector<os::LayoutNode*> apcWidgets;
	if( pcWidget )
		apcWidgets = pcWidget->GetChildList( pcNode );
	else
		apcWidgets = pcNode->GetChildList();
	for( uint i = 0; i < apcWidgets.size(); i++ )
	{

		ReLayout( apcWidgets[i] );
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





















