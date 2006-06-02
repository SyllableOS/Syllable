#include "replacedialog.h"

ReplaceDialog::ReplaceDialog( Window * pcParent, const String &str, bool down, bool caseS, bool extended ):Window( Rect( 0, 0, 200, 200 ), "ReplaceDialog", "Replace...", WND_NOT_RESIZABLE )
{
	pcParentWindow = pcParent;
	Init();
	Layout();
	SetTabs();

	//set initial config
	pcFindText->Set( str.c_str() );

	if( down )
		pcRadioDown->SetValue( true );
	else
		pcRadioUp->SetValue( true );
	if( caseS )
		pcRadioCase->SetValue( true );
	else
		pcRadioNoCase->SetValue( true );
	if( extended )
		pcRadioExtended->SetValue( true );
	else
		pcRadioBasic->SetValue( true );
}

void ReplaceDialog::Layout()
{

	os::FrameView* dirFrame = new os::FrameView( os::Rect(), "dirFrame", "_Direction" );
	os::FrameView* caseFrame = new os::FrameView( os::Rect(), "caseFrame", "C_ase" );
	os::FrameView* syntaxFrame = new os::FrameView( os::Rect(), "syntaxFrame", "_Syntax" );

	os::LayoutView* rootView = new os::LayoutView( os::Rect(), "" );

	os::HLayoutNode* root = new os::HLayoutNode( "root" );
	root->SetBorders( os::Rect( 3, 3, 3, 0 ) );
	os::VLayoutNode* l1 = new os::VLayoutNode( "l1", 1.0f, root );
	
	os::HLayoutNode* pcFindLayoutNode = new os::HLayoutNode("find_layout");
	pcFindLayoutNode->AddChild(new StringView(Rect(),"find_string_view","Find:"));
	pcFindLayoutNode->AddChild(new HLayoutSpacer("",5,5));
	pcFindLayoutNode->AddChild(pcFindText);
	
	os::HLayoutNode* pcReplaceLayoutNode = new os::HLayoutNode("replace_layout");
	pcReplaceLayoutNode->AddChild(new StringView(Rect(),"replace_string_view","Replace With:"));
	pcReplaceLayoutNode->AddChild(new HLayoutSpacer("",5,5));
	pcReplaceLayoutNode->AddChild(pcReplaceText);
	
	l1->SetBorders(os::Rect( 3,3,3,3));
	l1->AddChild(pcFindLayoutNode );
	l1->AddChild(pcReplaceLayoutNode);
	
	l1->SameWidth("find_layout","replace_layout",NULL);
	l1->SameWidth("find_string_view","replace_string_view",NULL);
	
	os::HLayoutNode* l2 = new os::HLayoutNode( "l2", 1.0f, l1 );
	os::VLayoutNode * l2a = new os::VLayoutNode( "l2a" );
	l2a->SetHAlignment( os::ALIGN_LEFT );
	l2a->AddChild( pcRadioUp );
	l2a->AddChild( pcRadioDown );
	l2a->SetBorders( os::Rect( 3, 3, 3, 3 ), "pcRadioUp", "pcRadioDown", NULL );
	dirFrame->SetRoot( l2a );
	l2->AddChild( dirFrame );
	os::VLayoutNode * l2b = new os::VLayoutNode( "l2b" );
	l2b->SetHAlignment( os::ALIGN_LEFT );
	l2b->AddChild( pcRadioCase );
	l2b->AddChild( pcRadioNoCase );
	l2b->SetBorders( os::Rect( 3, 3, 3, 3 ), "pcRadioCase", "pcRadioNoCase", NULL );
	caseFrame->SetRoot( l2b );
	l2->AddChild( caseFrame );
	os::VLayoutNode * l2c = new os::VLayoutNode( "l2c" );
	l2c->SetHAlignment( os::ALIGN_LEFT );
	l2c->AddChild( pcRadioBasic );
	l2c->AddChild( pcRadioExtended );
	l2c->SetBorders( os::Rect( 3, 3, 3, 3 ), "pcRadioBasic", "pcRadioExtended", NULL );
	syntaxFrame->SetRoot( l2c );
	l2->AddChild( syntaxFrame );
	os::VLayoutNode * l3 = new os::VLayoutNode( "l3", 1.0f, root );
	l3->SetBorders( os::Rect( 3, 3, 3, 3 ) );
	l3->AddChild( pcReplaceButton );
	l3->AddChild( pcReplaceAllButton);
	l3->AddChild( pcCloseButton );
	new os::VLayoutSpacer( "", 0, 1000, l3 );

	l3->SameWidth( "pcReplaceButton", "pcCloseButton", "pcReplaceAllButton",NULL );
	l3->SetBorders( os::Rect( 0, 0, 0, 3 ), "pcReplaceButton", NULL );
	l3->SetBorders( os::Rect( 0, 0, 0, 3 ), "pcReplaceAllButton", NULL );	
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

void ReplaceDialog::Init()
{
	pcReplaceButton = new os::Button( os::Rect(), "pcReplaceButton", "_Replace", new os::Message( M_REPLACE_DO ) );
	pcReplaceAllButton = new Button(os::Rect(), "pcReplaceAllButton", "Replace _All",new Message(M_REPLACE_ALL_DO));
	pcCloseButton = new os::Button( os::Rect(), "pcCloseButton", "_Close", new os::Message(M_FIND_QUIT ) );

	pcRadioUp = new os::RadioButton( os::Rect(), "pcRadioUp", "Up", NULL );
	pcRadioDown = new os::RadioButton( os::Rect(), "pcRadioDown", "Down", NULL );
	pcRadioCase = new os::RadioButton( os::Rect(), "pcRadioCase", "Sensitive", NULL );
	pcRadioNoCase = new os::RadioButton( os::Rect(), "pcRadioNoCase", "Insensitive", NULL );
	pcRadioBasic = new os::RadioButton( os::Rect(), "pcRadioBasic", "Basic", NULL );
	pcRadioExtended = new os::RadioButton( os::Rect(), "pcRadioExtended", "Extended", NULL );

	pcFindText = new os::TextView( os::Rect(), "TextView", "" );
	pcFindText->SetMultiLine( false );
	pcFindText->SetMinPreferredSize( 30, 1 );
	
	pcReplaceText = new os::TextView( os::Rect(), "TextView2", "" );
	pcReplaceText->SetMultiLine( false );
	pcReplaceText->SetMinPreferredSize( 30, 1 );
}

void ReplaceDialog::SetTabs()
{
	pcFindText->SetTabOrder(NEXT_TAB_ORDER);
	pcReplaceButton->SetTabOrder(NEXT_TAB_ORDER);
	pcCloseButton->SetTabOrder(NEXT_TAB_ORDER);
	SetFocusChild(pcFindText);
	SetDefaultButton(pcReplaceButton);
}

void ReplaceDialog::HandleMessage( os::Message * msg )
{
	switch ( msg->GetCode() )
	{
		case M_REPLACE_DO:
		{
			Message* pcMsg = new Message(M_REPLACE_DO);
			pcMsg->AddString( "text", pcFindText->GetBuffer()[0].c_str() );
			pcMsg->AddBool( "down", pcRadioDown->GetValue().AsBool(  ) );
			pcMsg->AddBool( "case", pcRadioCase->GetValue().AsBool(  ) );
			pcMsg->AddBool( "extended", pcRadioExtended->GetValue().AsBool());		
			pcMsg->AddString("replace_text",pcReplaceText->GetBuffer()[0]);
			pcParentWindow->PostMessage( pcMsg, pcParentWindow );
			break;
		}
		
		case M_REPLACE_ALL_DO:
		{
			Message* pcMsg = new Message(M_REPLACE_ALL_DO);
			pcMsg->AddString( "text", pcFindText->GetBuffer()[0].c_str() );
			pcMsg->AddBool( "down", pcRadioDown->GetValue().AsBool(  ) );
			pcMsg->AddBool( "case", pcRadioCase->GetValue().AsBool(  ) );
			pcMsg->AddBool( "extended", pcRadioExtended->GetValue().AsBool());		
			pcMsg->AddString("replace_text",pcReplaceText->GetBuffer()[0]);
			pcParentWindow->PostMessage( pcMsg, pcParentWindow );
			break;			
		}
		
		case M_UP:
			pcRadioUp->SetValue( true );
			break;
		case M_DOWN:
			pcRadioDown->SetValue( true );
			break;
		case M_CASE:
			pcRadioCase->SetValue( true );
			break;
		case M_NOCASE:
			pcRadioNoCase->SetValue( true );
			break;
		case M_BASIC:
			pcRadioBasic->SetValue( true );
			break;
		case M_EXTENDED:
			pcRadioExtended->SetValue( true );
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









