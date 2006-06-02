#include "finddialog.h"

FindDialog::FindDialog( Window * pcParent, const String &str, bool down, bool caseS, bool extended ):Window( Rect( 0, 0, 200, 200 ), "FindDialog", "Find text", WND_NOT_RESIZABLE )
{
	pcParentWindow = pcParent;
	Init();
	Layout();
	SetTabs();

	//set initial config
	text->Set( str.c_str() );

	if( down )
		rDown->SetValue( true );
	else
		rUp->SetValue( true );
	if( caseS )
		rCase->SetValue( true );
	else
		rNoCase->SetValue( true );
	if( extended )
		rExtended->SetValue( true );
	else
		rBasic->SetValue( true );
}

void FindDialog::Layout()
{

	os::FrameView * dirFrame = new os::FrameView( os::Rect(), "dirFrame", "_Direction" );
	os::FrameView * caseFrame = new os::FrameView( os::Rect(), "caseFrame", "C_ase" );
	os::FrameView * syntaxFrame = new os::FrameView( os::Rect(), "syntaxFrame", "_Syntax" );

	os::LayoutView * rootView = new os::LayoutView( os::Rect(), "" );

	os::HLayoutNode * root = new os::HLayoutNode( "root" );
	root->SetBorders( os::Rect( 3, 3, 3, 0 ) );
	os::VLayoutNode * l1 = new os::VLayoutNode( "l1", 1.0f, root );
	l1->SetBorders( os::Rect( 3, 3, 3, 3 ) );
	l1->AddChild( text );
	os::HLayoutNode * l2 = new os::HLayoutNode( "l2", 1.0f, l1 );
	os::VLayoutNode * l2a = new os::VLayoutNode( "l2a" );
	l2a->SetHAlignment( os::ALIGN_LEFT );
	l2a->AddChild( rUp );
	l2a->AddChild( rDown );
	l2a->SetBorders( os::Rect( 3, 3, 3, 3 ), "rUp", "rDown", NULL );
	dirFrame->SetRoot( l2a );
	l2->AddChild( dirFrame );
	os::VLayoutNode * l2b = new os::VLayoutNode( "l2b" );
	l2b->SetHAlignment( os::ALIGN_LEFT );
	l2b->AddChild( rCase );
	l2b->AddChild( rNoCase );
	l2b->SetBorders( os::Rect( 3, 3, 3, 3 ), "rCase", "rNoCase", NULL );
	caseFrame->SetRoot( l2b );
	l2->AddChild( caseFrame );
	os::VLayoutNode * l2c = new os::VLayoutNode( "l2c" );
	l2c->SetHAlignment( os::ALIGN_LEFT );
	l2c->AddChild( rBasic );
	l2c->AddChild( rExtended );
	l2c->SetBorders( os::Rect( 3, 3, 3, 3 ), "rBasic", "rExtended", NULL );
	syntaxFrame->SetRoot( l2c );
	l2->AddChild( syntaxFrame );
	os::VLayoutNode * l3 = new os::VLayoutNode( "l3", 1.0f, root );
	l3->SetBorders( os::Rect( 3, 3, 3, 3 ) );
	l3->AddChild( findButton );
	l3->AddChild( closeButton );
	new os::VLayoutSpacer( "", 0, 1000, l3 );

	l3->SameWidth( "findButton", "closeButton", NULL );
	l3->SetBorders( os::Rect( 0, 0, 0, 3 ), "findButton", NULL );
	l3->LimitMaxSize(l3->GetPreferredSize(false));
	root->SameWidth( "TextView", "l2", NULL );
	root->SetBorders( os::Rect( 0, 0, 0, 3 ), "TextView", NULL );

	rootView->SetRoot( root );
	root->Layout();

	os::Point p = rootView->GetPreferredSize( false );
	rootView->SetFrame( os::Rect( 0, 0, p.x, p.y ) );
	ResizeTo( p.x, p.y );

	AddChild( rootView );
}

void FindDialog::Init()
{
	findButton = new os::Button( os::Rect(), "findButton", "_Find", new os::Message( M_SEND_FIND ) );
	closeButton = new os::Button( os::Rect(), "closeButton", "_Close", new os::Message(M_FIND_QUIT ) );

	rUp = new os::RadioButton( os::Rect(), "rUp", "Up", NULL );
	rDown = new os::RadioButton( os::Rect(), "rDown", "Down", NULL );
	rCase = new os::RadioButton( os::Rect(), "rCase", "Sensitive", NULL );
	rNoCase = new os::RadioButton( os::Rect(), "rNoCase", "Insensitive", NULL );
	rBasic = new os::RadioButton( os::Rect(), "rBasic", "Basic", NULL );
	rExtended = new os::RadioButton( os::Rect(), "rExtended", "Extended", NULL );

	text = new os::TextView( os::Rect(), "TextView", "" );
	text->SetMultiLine( false );
	text->SetMinPreferredSize( 30, 1 );
}

void FindDialog::SetTabs()
{
	text->SetTabOrder(NEXT_TAB_ORDER);
	findButton->SetTabOrder(NEXT_TAB_ORDER);
	closeButton->SetTabOrder(NEXT_TAB_ORDER);
	SetFocusChild(text);
	SetDefaultButton(findButton);
}

void FindDialog::HandleMessage( os::Message * msg )
{
	switch ( msg->GetCode() )
	{
	case M_SEND_FIND:
	{
		os::Message * m = new Message(M_SEND_FIND);
		if( !m )
		{
			dbprintf( "Error: Invoker registered with this FindDialog requester does not have a message!\n" );
		}
		else
		{
			m->AddString( "text", text->GetBuffer()[0] );
			m->AddBool( "down", rDown->GetValue().AsBool(  ) );
			m->AddBool( "case", rCase->GetValue().AsBool(  ) );
			m->AddBool( "extended", rExtended->GetValue().AsBool(  ) );
			pcParentWindow->PostMessage( m, pcParentWindow );
		}
		break;
	}
	
	case M_UP:
		rUp->SetValue( true );
		break;
	case M_DOWN:
		rDown->SetValue( true );
		break;
	case M_CASE:
		rCase->SetValue( true );
		break;
	case M_NOCASE:
		rNoCase->SetValue( true );
		break;
	case M_BASIC:
		rBasic->SetValue( true );
		break;
	case M_EXTENDED:
		rExtended->SetValue( true );
		break;

	case M_FIND_QUIT:
	{
		PostMessage(new Message(os::M_QUIT),this);
		break;
	}

	default:
		os::Window::HandleMessage( msg );
	}
}
