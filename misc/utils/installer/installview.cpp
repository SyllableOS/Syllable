#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <atheos/device.h>

#include <gui/button.h>
#include <gui/radiobutton.h>
#include <gui/textview.h>
#include <gui/dropdownmenu.h>
#include <gui/frameview.h>
#include <gui/filerequester.h>
#include <gui/message.h>

#include "installview.h"

using namespace os;

enum
{
    ID_INSTALL,
    ID_BROWSE_SRC,
    ID_BROWSE_DST,
    ID_START_TERM
};

InstallView::InstallView( const std::string& cName ) : LayoutView( Rect(), cName, NULL, CF_FOLLOW_NONE )
{
    VLayoutNode* pcRoot = new VLayoutNode( "root2" );
    HLayoutNode* pcButtonNode  = new HLayoutNode( "buttons" );
    HLayoutNode* pcSrcRoot     = new HLayoutNode( "src_root" );
    VLayoutNode* pcSrcRadios   = new VLayoutNode( "src_radios" );
    HLayoutNode* pcSrcPathCtrl = new HLayoutNode( "src_path_ctrl" );
    HLayoutNode* pcDstRoot     = new HLayoutNode( "dst_root" );
    HLayoutNode* pcDstPathCtrl = new HLayoutNode( "dst_path_ctrl" );


      // Setup the main controls.
    m_pcSourceFrame = new FrameView( Rect(), "src_frame", "Source" );
    m_pcDestFrame   = new FrameView( Rect(), "dst_frame", "Destination" );
    m_pcQuitBut     = new Button( Rect(), "quit_but", "Quit", new Message( M_QUIT ) );
    m_pcTerminalBut = new Button( Rect(), "term_but", "Start terminal...", new Message( ID_START_TERM ) );
    m_pcInstallBut  = new Button( Rect(), "install_but", "Install", new Message( ID_INSTALL ) );

    pcButtonNode->AddChild( m_pcTerminalBut );
    pcButtonNode->AddChild( new LayoutSpacer( "space", 1.0f, NULL, Point(0.0f,0.0f), Point( MAX_SIZE, 0.0f ) ) );
    pcButtonNode->AddChild( m_pcQuitBut );
    pcButtonNode->AddChild( m_pcInstallBut );

    pcButtonNode->SetBorders( Rect( 10.0f, 5.0f, 10.0f, 5.0f ), "quit_but", "term_but", "install_but", NULL );

      // Setup the source controls
    
    m_pcSrcFloppy    = new RadioButton( Rect(), "src_radio_floppy", "Floppy disks", NULL );
    m_pcSrcFile	     = new RadioButton( Rect(), "src_radio_file", "TAR archive", NULL );
    m_pcSrcPath      = new TextView( Rect(), "src_path", "/" );
    m_pcBrowseSrcBut = new Button( Rect(), "src_browse", "Browse...", new Message( ID_BROWSE_SRC ) );

    pcSrcRadios->AddChild( m_pcSrcFloppy );
    pcSrcRadios->AddChild( m_pcSrcFile );
    pcSrcRadios->SetHAlignments( ALIGN_LEFT, "src_radios", NULL );

    pcSrcPathCtrl->AddChild( m_pcSrcPath );
    pcSrcPathCtrl->AddChild( m_pcBrowseSrcBut );
    pcSrcPathCtrl->SetBorders( Rect( 0.0f, 0.0f, 5.0f, 0.0f ), "src_browse", NULL );
    
    pcSrcRadios->AddChild( pcSrcPathCtrl );
    pcSrcRoot->AddChild( pcSrcRadios );

    pcSrcRoot->SetBorders( Rect( 10.0f, 5.0f, 10.0f, 5.0f ), "src_path", NULL );

      // Setup destination controls

    m_pcDstPath = new DropdownMenu( Rect(), "dst_path" );
    m_pcBrowseDstBut = new Button( Rect(), "dst_browse", "Browse...", new Message( ID_BROWSE_DST ) );
    pcDstPathCtrl->AddChild( m_pcDstPath );
    pcDstPathCtrl->AddChild( m_pcBrowseDstBut );
    pcDstPathCtrl->SetBorders( Rect( 0.0f, 0.0f, 5.0f, 0.0f ), "dst_browse", NULL );

    pcDstRoot->AddChild( pcDstPathCtrl );
    pcDstRoot->SetBorders( Rect( 10.0f, 5.0f, 10.0f, 5.0f ), "dst_path", NULL );
    m_pcDstPath->SetMaxPreferredSize( 100000 );
      // Setup the root node
    pcRoot->AddChild( m_pcSourceFrame );
    pcRoot->AddChild( m_pcDestFrame );
    pcRoot->AddChild( new LayoutSpacer( "space", 1.0f, NULL, Point(0.0f,0.0f), Point( 0.0f, MAX_SIZE ) ) );
    pcRoot->AddChild( pcButtonNode );

    pcRoot->SetBorders( Rect( 5.0f, 5.0f, 5.0f, 5.0f ), "src_frame", "dst_frame", NULL );
    
    m_pcSourceFrame->SetRoot( pcSrcRoot );
    m_pcDestFrame->SetRoot( pcDstRoot );
    SetRoot( pcRoot );

    
    InitDstPath( "/dev/disk" );
}

InstallView::~InstallView()
{
}


void InstallView::InitDstPath( const char* pzPath )
{
    DIR* hDir = opendir( pzPath );
    if ( hDir == NULL ) {
	return;
    }
    struct dirent* psEntry;
    while( (psEntry = readdir( hDir )) != NULL ) {
	if ( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 ) {
	    continue;
	}
	char zFullPath[PATH_MAX];
	strcpy( zFullPath, pzPath );
	strcat( zFullPath, "/" );
	strcat( zFullPath, psEntry->d_name );
	
	struct stat sStat;
	if ( stat( zFullPath, &sStat ) < 0 ) {
	    continue;
	}
	if ( S_ISDIR( sStat.st_mode ) ) {
	    InitDstPath( zFullPath );
	    continue;
	}
	int nDevice = open( zFullPath, O_RDONLY );
	if ( nDevice < 0 ) {
	    continue;
	}
	device_geometry sGeom;
	if ( ioctl( nDevice, IOCTL_GET_DEVICE_GEOMETRY, &sGeom ) < 0 ) {
	    close( nDevice );
	    continue;
	}
	close( nDevice );
	m_pcDstPath->AppendItem( zFullPath );
    }
    closedir( hDir );
}

void InstallView::AttachedToWindow()
{
    m_pcInstallBut->SetTarget( this );
    m_pcTerminalBut->SetTarget( this );
    m_pcBrowseSrcBut->SetTarget( this );
    m_pcBrowseDstBut->SetTarget( this );

    m_pcSrcPathRequester = new FileRequester( FileRequester::LOAD_REQ, new Messenger( this ), "/", FileRequester::NODE_FILE,
					      false, NULL, NULL, false, true, "Install", "Cancel" );
    m_pcDstPathRequester = new FileRequester( FileRequester::LOAD_REQ, new Messenger( this ), "/", FileRequester::NODE_FILE,
					      false, NULL, NULL, false, true, "Install", "Cancel" );
    
}

void InstallView::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_INSTALL:
	    printf( "Install\n" );
	    break;
	case ID_BROWSE_SRC:
//	    m_pcSrcPathRequester->Lock();
//	    m_pcSrcPathRequester->SetPath( m_cSrcPaths );
//	    m_pcSrcPathRequester->Unlock();
	    m_pcSrcPathRequester->Show();
	    break;
	case ID_BROWSE_DST:
	    m_pcDstPathRequester->Show();
	    break;
	case M_LOAD_REQUESTED:
	{
	    const char*    pzPath;
	    FileRequester* pcSource;
	    if ( pcMessage->FindPointer( "source", (void**)&pcSource ) != 0 ) {
		break;
	    }
	    if ( pcMessage->FindString( "file/path", &pzPath ) != 0 ) {
		break;
	    }
	    if ( pcSource == m_pcSrcPathRequester ) {
		m_pcSrcPath->Set( pzPath );
	    } else if ( pcSource == m_pcDstPathRequester ) {
		m_pcDstPath->SetCurrentString( pzPath );
	    }
	    break;
	}
	case ID_START_TERM:
	    if ( fork() == 0 ) {
		execl( "/bin/aterm", "aterm", NULL );
		exit( 1 );
	    }
	    break;
	default:
	    LayoutView::HandleMessage( pcMessage );
	    break;
    }
}
