#include <time.h>
#include <atheos/types.h>

#include <gui/tableview.h>
#include <gui/button.h>
#include <gui/radiobutton.h>
#include <gui/bitmap.h>
#include <gui/menu.h>
#include <gui/filerequester.h>
#include <gui/spinner.h>
#include <gui/slider.h>

#include <util/application.h>
#include <util/message.h>

#include "athgui.h"

#include "defs.h"

#include "obj.h"
#include "bitmap.h"
#include "title.h"
#include "bfbm.h"

MyWindow*	g_pcWindow;
Bitmap*		g_pcBitmap;
View*		g_pcDrawWid;
View*		g_pcWndDrawWid;


#define MOUSE_PREV	4

/* Mouse recording */
typedef struct {
    int x, y;
    unsigned long t;
} mouse_info;

static mouse_info mprev[MOUSE_PREV];
static int moffset = 0;

static double cur_grav_max[BF_NUM] = { 10000000.0,  10000000.0, 10000000.0,  10000000.0   };
static double cur_grav_min[BF_NUM] = {        0.0, -10000000.0, -10000000.0, -10000000.0   };
static double cur_misc_max[BF_NUM] = {      360.0,  10000000.0, 1000.0,      1000.0   };
static double cur_misc_min[BF_NUM] = {     -360.0,         0.0, 0.0,         -0.0   };

static double GetFL( View* pcWid, double min, double max)
{
  Spinner* pcSpinner = (Spinner*) pcWid;
  char     contents[256];
  double   result;

  result = pcSpinner->GetValue();
  
//  std::string cContents = pcEditBox->GetString();

//  sscanf( cContents.c_str(), "%lf", &result );

  if ( result < min ) result = min;
  if ( result > max ) result = max;

//  SetFL(obj, result);
  return( result );
}

#define GetForceGrav( obj, which ) \
  mst.cur_grav_val[which] = GetFL( obj, cur_grav_min[ which ], cur_grav_max[ which ] )

#define GetForceMisc(obj, which) \
  mst.cur_misc_val[which] = GetFL( obj, cur_misc_min[ which ], cur_misc_max[ which ] )


#define FOREachSelString(command) for (i = 0; i < num_spring; i++) \
  if (springs[i].status & S_SELECTED) (command);

















MyWindow::MyWindow( Rect cFrame, const char* pzName, const char* pzTitle, int nFlags ) :
    Window( cFrame, pzName, pzTitle, nFlags )
{
  m_pcLoadRequester = NULL;
  m_pcSaveRequester = NULL;
}


void MyWindow::SetCheckBox( const char* pzName, bool bChecked )
{
  Control* pcChkBox;

  pcChkBox = static_cast<Control*>(FindView( pzName ));
  if ( NULL != pcChkBox ) {
    pcChkBox->SetValue( bChecked, false );
  } else {
    dbprintf( "Unable to find checkbox %s\n", pzName );
  }
}

void MyWindow::SetSlider( const char* pzName, int nValue )
{
    Slider*	pcSlider;

    pcSlider = static_cast<Slider*>(FindView( pzName ));
    if ( NULL != pcSlider ) {
	pcSlider->SetValue( nValue, false );
    } else {
	dbprintf( "Unable to find slider %s\n", pzName );
    }
}

void	MyWindow::SetFL( const char* pzName, double vValue )
{
  Spinner* pcSpinner;

  pcSpinner = static_cast<Spinner*>( FindView( pzName ) );
  if ( NULL != pcSpinner ) {
    pcSpinner->SetValue( vValue, false );
//    char zValue[128];
//    sprintf( zValue, "%lf", vValue );
//    pcEditBox->SetString( zValue );
  } else {
    printf( "Unable to find spinner %s\n", pzName );
  }
}

void	LoadFile( void )
{
}

void MyWindow::UpdateWidgets( void )
{
  switch( mode )
  {
    case M_EDIT:
      SetCheckBox( "MODE_EDIT", true );
      break;
    case M_MASS:
      SetCheckBox( "MODE_MASS", true );
      break;
    case M_SPRING:
      SetCheckBox( "MODE_SPRING", false );
      break;
  }
    
  SetCheckBox( "ACTION", 0 == action );
  SetCheckBox( "DRAW_SPRING", mst.show_spring );
  SetCheckBox( "ADAPT_TIME_STEP", mst.adaptive_step );
  SetCheckBox( "FIXED_MASS", mst.fix_mass );
  SetCheckBox( "W_LEFT", mst.w_left );
  SetCheckBox( "W_RIGHT", mst.w_right );
  SetCheckBox( "W_TOP", mst.w_top );
  SetCheckBox( "W_BOTTOM", mst.w_bottom );

  SetCheckBox( "GRAV_ACTIVE", 0 == mst.bf_mode[FR_GRAV] );
  SetCheckBox( "CENTER_ACTIVE", 0 == mst.bf_mode[FR_CMASS] );
  SetCheckBox( "ATTRA_ACTIVE", 0 == mst.bf_mode[FR_PTATTRACT] );
  SetCheckBox( "WFORCE_ACTIVE", 0 == mst.bf_mode[FR_WALL] );

  SetSlider( "GRID", mst.grid_snap );

  SetFL( "MASS", mst.cur_mass );
  SetFL( "ELAS", mst.cur_rest );
  SetFL( "KSPR", mst.cur_ks );
  SetFL( "KDMP", mst.cur_kd );
  SetFL( "VISC", mst.cur_visc );
  SetFL( "STIC", mst.cur_stick );
  SetFL( "STEP", mst.cur_dt );
  SetFL( "PREC", mst.cur_prec );

  SetFL( "GRAV_GRAV", mst.cur_grav_val[ FR_GRAV ] );
  SetFL( "GRAV_DIR", mst.cur_misc_val[ FR_GRAV ] );
  SetFL( "CENTER_MAGN", mst.cur_grav_val[ FR_CMASS ] );
  SetFL( "CENTER_DAMP", mst.cur_misc_val[ FR_CMASS ] );
  SetFL( "ATTRA_MAGN", mst.cur_grav_val[ FR_PTATTRACT ] );
  SetFL( "ATTRA_EXPO", mst.cur_misc_val[ FR_PTATTRACT ] );
  SetFL( "WFORCE_MAGN", mst.cur_grav_val[ FR_WALL ] );
  SetFL( "WFORCE_EXPO", mst.cur_misc_val[ FR_WALL ] );
}

void MyWindow::HandleMessage( Message* pcMsg )
{
    void* pView;

    if ( pcMsg->FindPointer( "source", &pView ) == 0 )
    {
	Control* pcSource = static_cast<Control*>(static_cast<Invoker*>(pView));
	switch( pcMsg->GetCode() )
	{
	    case M_LOAD_REQUESTED:
	    {
		const char* pzPath;
		if ( pcMsg->FindString( "file/path", &pzPath ) == 0 ) {
		    char zBuffer[ 1024 ];
		    strcpy( zBuffer, pzPath );
		    file_command( zBuffer, F_LOAD );
		}
		break;
	    }
	    case M_SAVE_REQUESTED:
	    {
		const char* pzPath;
		if ( pcMsg->FindString( "file/path", &pzPath ) == 0 ) {
		    char zBuffer[ 1024 ];
		    strcpy( zBuffer, pzPath );
		    file_command( zBuffer, F_SAVE );
		}
		break;
	    }
	    case GID_MODE_EDIT:
		if ( static_cast<Control*>(pcSource)->GetValue() ) {
		    mode = M_EDIT;
		}
		break;
	    case GID_MODE_MASS:
		if ( static_cast<Control*>(pcSource)->GetValue() ) {
		    mode	= M_MASS;
		}
		break;
	    case GID_MODE_SPRING:
		if ( static_cast<Control*>(pcSource)->GetValue() ) {
		    mode	= M_SPRING;
		}
		break;


	    case GID_ADAPT_TIME_STEP:
		mst.adaptive_step = ((CheckBox*)pcSource)->GetValue().AsBool();
		break;
	    case GID_ACTION:
		action = (((CheckBox*)pcSource)->GetValue().AsBool()) ? 0 : -1;
		break;

	    case GID_FIXED_MASS:
	    {
		mst.fix_mass	=	((CheckBox*)pcSource)->GetValue().AsBool();
		for ( int i = 0 ; i < num_mass; i++ )
		{
		    if (masses[i].status & S_SELECTED) {
			if (mst.fix_mass) {
			    masses[i].status |= S_FIXED;
			    masses[i].status &= ~S_TEMPFIXED;
			} else {
			    masses[i].status &= ~(S_FIXED | S_TEMPFIXED);
			}
		    }
		}
		review_system(TRUE);
		break;
	    }

	    case GID_W_LEFT:
		mst.w_left = ((CheckBox*)pcSource)->GetValue().AsBool();
		break;
	    case GID_W_RIGHT:
		mst.w_right= ((CheckBox*)pcSource)->GetValue().AsBool();
		break;
	    case GID_W_TOP:
		mst.w_top  = ((CheckBox*)pcSource)->GetValue().AsBool();
		break;
	    case GID_W_BOTTOM:
		mst.w_bottom	=	((CheckBox*)pcSource)->GetValue().AsBool();
		break;

	    case GID_GRAV_ACTIVE:
		mst.bf_mode[FR_GRAV] = (((CheckBox*)pcSource)->GetValue().AsBool()) ? 0 : -1;
		break;
	    case GID_CENTER_ACTIVE:
		mst.bf_mode[FR_CMASS] = (((CheckBox*)pcSource)->GetValue().AsBool()) ? 0 : -1;
		break;
	    case GID_ATTRA_ACTIVE:
		mst.bf_mode[FR_PTATTRACT] = (((CheckBox*)pcSource)->GetValue().AsBool()) ? 0 : -1;
		break;
	    case GID_WFORCE_ACTIVE:
		mst.bf_mode[FR_WALL] = (((CheckBox*)pcSource)->GetValue().AsBool()) ? 0 : -1;
		break;

	    case GID_SELECT_ALL:
		select_all();
		eval_selection();
		review_system(TRUE);
		break;

	    case GID_SET_REST_LEN:
		set_sel_restlen();
		break;

	    case GID_SET_CENTER:
		set_center();
		review_system(TRUE);
		break;

	    case GID_GRID:
	    {
		NumView* pcNumView = static_cast<NumView*>( FindView( "GRID_VIEW" ) );

		int nGrid = pcSource->GetValue();

		mst.grid_snap = (mst.cur_gsnap = nGrid) != 1;

		if ( NULL != pcNumView )
		{
		    pcNumView->SetNumber( nGrid );
		}
		break;
	    }

	    case GID_MASS:
	    {
		mst.cur_mass = GetFL( pcSource, 0.01, 10000000.0);
		for ( int i = 0; i < num_mass; i++)
		{
		    if (masses[i].status & S_SELECTED) {
			masses[i].mass = mst.cur_mass;
			masses[i].radius = mass_radius(mst.cur_mass);
		    }
		}
		review_system( TRUE );
		break;
	    }
	    case GID_ELAS:
	    {
		mst.cur_rest = GetFL( pcSource, 0.0, 1.0 );
		for ( int i = 0; i < num_mass; i++) {
		    if (masses[i].status & S_SELECTED)
			masses[i].elastic = mst.cur_rest;
		}
		break;
	    }
	    case GID_KSPR :
	    {
		int	i;

		mst.cur_ks = GetFL( pcSource, 0.01, 10000000.0);
		FOREachSelString(springs[i].ks = mst.cur_ks);
		break;
	    }
	    case GID_KDMP:
	    {
		int	i;

		mst.cur_kd = GetFL( pcSource, 0.0, 10000000.0);
		FOREachSelString(springs[i].kd = mst.cur_kd);
		break;
	    }

	    case GID_VISC : mst.cur_visc  = GetFL( pcSource, 0.0,    10000000.0); break;
	    case GID_STIC : mst.cur_stick = GetFL( pcSource, 0.0,    10000000.0); break;
	    case GID_STEP : mst.cur_dt    = GetFL( pcSource, 0.0001,        1.0); break;
	    case GID_PREC : mst.cur_prec  = GetFL( pcSource, 0.0001,     1000.0); break;

	    case GID_GRAV_GRAV    : GetForceGrav( pcSource, FR_GRAV);      break;
	    case GID_GRAV_DIR     : GetForceMisc( pcSource, FR_GRAV);      break;
	    case GID_CENTER_MAGN  : GetForceGrav( pcSource, FR_CMASS);     break;
	    case GID_CENTER_DAMP  : GetForceMisc( pcSource, FR_CMASS);     break;
	    case GID_ATTRA_MAGN   : GetForceGrav( pcSource, FR_PTATTRACT); break;
	    case GID_ATTRA_EXPO   : GetForceMisc( pcSource, FR_PTATTRACT); break;
	    case GID_WFORCE_MAGN  : GetForceGrav( pcSource, FR_WALL);      break;
	    case GID_WFORCE_EXPO  : GetForceMisc( pcSource, FR_WALL);      break;

	    case GID_DRAW_SPRING:
		mst.show_spring	=	((CheckBox*)pcSource)->GetValue().AsBool();
		review_system(TRUE);
		break;

	    case MID_LOAD:
		if ( m_pcLoadRequester == NULL ) {
		    m_pcLoadRequester = new FileRequester( FileRequester::LOAD_REQ, new Messenger( this ), "/" );
		}
		m_pcLoadRequester->Show();
		break;
	    case MID_SAVE:
	    case MID_SAVE_AS:
		if ( m_pcSaveRequester == NULL ) {
		    m_pcSaveRequester = new FileRequester( FileRequester::SAVE_REQ, new Messenger( this ), "/" );
		}
		m_pcSaveRequester->Show();
		break;
	    case MID_DUP_SEL     : duplicate_selected(); review_system( true ); break;
	    case MID_DELETE_SEL  : delete_selected();    review_system( true ); break;
	    case MID_QUIT:
	    {
		Message	cMsg( M_QUIT );
		Application::GetInstance()->PostMessage( &cMsg );
		break;
	    }

	}
    }
}




MyApp::MyApp() : Application( "application/x-vnd.DMD-xspringies" )
{
}

MyApp::~MyApp()
{
}

bool	MyApp::OkToQuit()
{
  g_bQuit = true;
  wait_for_thread( g_hUpdateThread );
  g_pcWindow->Quit();
  return( true );
}



TableView* CreateEditMode()
{
  TableView*	pcTable	= new TableView( Rect( 0, 0, 1, 1 ), "em_t1", "Edit mode", 1, 3 );

  pcTable->SetColAlignment( 0, ALIGN_LEFT );

  RadioButton* pcRadio;
  
  pcRadio = new RadioButton( Rect( 0, 0, 1, 1 ), "MODE_EDIT", "Edit", new Message( GID_MODE_EDIT )  );
//  pcRadio->SetValue( true );
  pcTable->SetChild( pcRadio, 0, 0 );
  
  pcRadio = new RadioButton( Rect( 0, 0, 1, 1 ), "MODE_MASS", "Mass", new Message( GID_MODE_MASS )  );
  pcTable->SetChild( pcRadio, 0, 1 );
  
  pcRadio = new RadioButton( Rect( 0, 0, 1, 1 ), "MODE_SPRING", "Spring", new Message( GID_MODE_SPRING ) );
  pcTable->SetChild( pcRadio, 0, 2 );
  
  return( pcTable );
}

TableView* CreateWalls()
{
  const Rect	cRect( 0, 0, 1, 1 );

  TableView*	pcTable	= new TableView( cRect, "w_t1", "Walls", 3, 3 );

  pcTable->SetCellAlignment( 1, 0, ALIGN_CENTER, ALIGN_TOP );
  pcTable->SetCellAlignment( 0, 1, ALIGN_LEFT,   ALIGN_CENTER );
  pcTable->SetCellAlignment( 2, 1, ALIGN_RIGHT,  ALIGN_CENTER );
  pcTable->SetCellAlignment( 1, 2, ALIGN_CENTER, ALIGN_BOTTOM );

//	pcTable->SetCellBorders( 0, 0, 0, 0 );

  pcTable->SetChild( new CheckBox( cRect, "W_TOP", "", new Message( GID_W_TOP ) ), 	1, 0 );
  pcTable->SetChild( new CheckBox( cRect, "W_LEFT", "", new Message( GID_W_LEFT ) ), 	0, 1 );
  pcTable->SetChild( new CheckBox( cRect, "W_RIGHT", "", new Message( GID_W_RIGHT ) ), 	2, 1 );
  pcTable->SetChild( new CheckBox( cRect, "W_BOTTOM", "", new Message( GID_W_BOTTOM ) ), 	1, 2 );

  return( pcTable );
}


TableView* CreateControl()
{
  TableView*	pcTable1 = new TableView( Rect( 0, 325, 99, 500 ), "ct_1", "Control", 1, 6 );
  TableView*	pcTable2 = new TableView( Rect( 0, 325, 99, 500 ), "ct_2", NULL, 2, 2 );
  TableView*	pcTable4 = new TableView( Rect( 0, 325, 99, 500 ), "ct_3", NULL, 3, 1 );

  pcTable1->SetChild( pcTable2, 0, 3 );
  pcTable1->SetChild( pcTable4, 0, 4 );
  pcTable1->SetColAlignment( 0, ALIGN_CENTER );

  Rect	cRect( 0, 0, 1, 1 );

  pcTable1->SetChild( new Button( cRect, "ct_b1", "Select all", new Message( GID_SELECT_ALL ) ), 0, 0 );
  pcTable1->SetChild( new Button( cRect, "ct_b2", "Set center", new Message( GID_SET_CENTER ) ), 0, 1 );
  pcTable1->SetChild( new Button( cRect, "ct_b3", "Set rest length", new Message( GID_SET_REST_LEN ) ),	0, 2 );

  pcTable2->SetColAlignment( 0, ALIGN_LEFT );
  pcTable2->SetColAlignment( 1, ALIGN_RIGHT );

  pcTable2->SetChild( new StringView( cRect, "ct_s1", "Draw spring:" ), 0, 0 );
  pcTable2->SetChild( new CheckBox( cRect, "DRAW_SPRING", "", new Message( GID_DRAW_SPRING ) ), 1, 0 );

  pcTable2->SetChild( new StringView( cRect, "ct_s2", "Action:" ), 0, 1 );
  pcTable2->SetChild( new CheckBox( cRect, "ACTION", "", new Message( GID_ACTION ) ), 1, 1 );


  Slider* pcSlider = new Slider( cRect, "GRID", new Message( GID_GRID )/*, 1, 100, HORIZONTAL*/ );
  pcSlider->SetMinMax( 1, 100 );
  pcSlider->SetTarget( g_pcWindow );
  pcSlider->SetValue( 1 );

  pcTable4->SetChild( new StringView( cRect, "ct_s3", "Grid:" ), 0, 0, 1.0f );
  pcTable4->SetChild( pcSlider, 1, 0, 100.0f );
  pcTable4->SetChild( new NumView( "GRID_VIEW", 1 ), 2, 0, 1.0f );


  pcTable4->SetColAlignment( 0, ALIGN_LEFT );
  pcTable4->SetColAlignment( 1, ALIGN_CENTER );
  pcTable4->SetColAlignment( 2, ALIGN_RIGHT );



  return( pcTable1 );
}



TableView* CreateSMSettings()
{
  const Rect	cRect( 0, 0, 1, 1 );

  TableView* pcTable1 = new TableView( Rect( 0, 325, 99, 500 ), "sm_t1", "S&M Settings", 1, 2 );
  TableView* pcTable2 = new TableView( Rect( 0, 325, 99, 500 ), "sm_t2", NULL, 2, 4 );
  TableView* pcTable3 = new TableView( Rect( 0, 325, 99, 500 ), "sm_t3", NULL, 2, 1 );

  pcTable1->SetChild( pcTable2, 0, 0 );
  pcTable1->SetChild( pcTable3, 0, 1 );

  pcTable2->SetChild( new StringView( cRect, "sm_s1", "Mass:" ), 0, 0 );
  pcTable2->SetChild( new Spinner( cRect, "MASS", 1.0f, new Message( GID_MASS ) ), 1, 0 );

  pcTable2->SetChild( new StringView( cRect, "sm_s2", "Elas:" ), 0, 1 );
  pcTable2->SetChild( new Spinner( cRect, "ELAS", 1.0f, new Message( GID_ELAS ) ), 1, 1 );

  pcTable2->SetChild( new StringView( cRect, "sm_s3", "KSpr:" ), 0, 2 );
  pcTable2->SetChild( new Spinner( cRect, "KSPR", 1.0f, new Message( GID_KSPR ) ), 1, 2 );

  pcTable2->SetChild( new StringView( cRect, "sm_s4", "KDmp:" ), 0, 3 );
  pcTable2->SetChild( new Spinner( cRect, "KDMP", 1.0f, new Message( GID_KDMP ) ), 1, 3 );

  pcTable3->SetChild( new StringView( cRect, "sm_s5", "Fixed Mass:" ), 0, 0 );
  pcTable3->SetChild( new CheckBox( cRect, "FIXED_MASS", "", new Message( GID_FIXED_MASS ) ), 1, 0 );

  return( pcTable1 );
}

TableView* CreateGlobalSettings()
{
  const Rect	cRect( 0, 0, 1, 1 );

  TableView* pcTable1 = new TableView( Rect( 0, 325, 99, 500 ), "gs_1", "Global settings", 1, 3 );
  TableView* pcTable2 = new TableView( Rect( 0, 325, 99, 500 ), "gs_2", NULL, 2, 4 );
  TableView* pcTable3 = new TableView( Rect( 0, 325, 99, 500 ), "gs_3", NULL, 2, 1 );
  TableView* pcTable4 = new TableView( Rect( 0, 325, 99, 500 ), "gs_4", NULL, 2, 1 );

  pcTable1->SetChild( pcTable2, 0, 0 );
  pcTable1->SetChild( pcTable3, 0, 1 );
  pcTable1->SetChild( pcTable4, 0, 2 );

  pcTable2->SetChild( new StringView( cRect, "gs_s1", "Visc:" ), 0, 0 );
  pcTable2->SetChild( new Spinner( cRect, "VISC", 1.0f, new Message( GID_VISC ) ), 1, 0 );

  pcTable2->SetChild( new StringView( cRect, "gs_s2", "Stic:" ), 0, 1 );
  pcTable2->SetChild( new Spinner( cRect, "STIC", 1.0f, new Message( GID_STIC ) ), 1, 1 );

  pcTable2->SetChild( new StringView( cRect, "gs_s3", "Step:" ), 0, 2 );
  pcTable2->SetChild( new Spinner( cRect, "STEP", 1.0f, new Message( GID_STEP ) ), 1, 2 );

  pcTable2->SetChild( new StringView( cRect, "gs_s4", "Prec:" ), 0, 3 );
  pcTable2->SetChild( new Spinner( cRect, "PREC", 1.0f, new Message( GID_PREC ) ), 1, 3 );

  pcTable3->SetChild( new StringView( cRect, "gs_s5", "AdaptTimeStep:" ), 0, 0 );
  pcTable3->SetChild( new CheckBox( cRect, "ADAPT_TIME_STEP", "", new Message( GID_ADAPT_TIME_STEP ) ), 1, 0 );

  pcTable4->SetChild( new StringView( cRect, "gs_s6", "Frames/sec:" ), 0, 0 );
  pcTable4->SetChild( new NumView( "FRAME_RATE_VIEW", 60 ), 1, 0 );

  return( pcTable1 );
}


TableView* CreateGravity()
{
  const Rect	cRect( 0, 0, 1, 1 );

  TableView* pcTable1 = new TableView( Rect( 0, 325, 99, 500 ), "cg_t1", "Gravity", 1, 2 );
  TableView* pcTable2 = new TableView( Rect( 0, 325, 99, 500 ), "cg_t2", NULL, 2, 1 );
  TableView* pcTable3 = new TableView( Rect( 0, 325, 99, 500 ), "cg_t3", NULL, 2, 2 );

  pcTable1->SetChild( pcTable2, 0, 0 );
  pcTable1->SetChild( pcTable3, 0, 1 );

  pcTable2->SetChild( new StringView( cRect, "cg_s1", "Active(g):" ), 0, 0 );
  pcTable2->SetChild( new CheckBox( cRect, "GRAV_ACTIVE", "", new Message( GID_GRAV_ACTIVE ) ), 1, 0 );

  pcTable3->SetChild( new StringView( cRect, "cg_s2", "Grav:" ), 0, 0 );
  pcTable3->SetChild( new Spinner( cRect, "GRAV_GRAV", 10.0f, new Message( GID_GRAV_GRAV ) ), 1, 0 );

  pcTable3->SetChild( new StringView( cRect, "cg_s3", "Dir:" ), 0, 1 );
  pcTable3->SetChild( new Spinner( cRect, "GRAV_DIR", 0.0f, new Message( GID_GRAV_DIR ) ), 1, 1 );

  return( pcTable1 );
}

TableView* CreateCenter()
{
  const Rect	cRect( 0, 0, 1, 1 );

  TableView* pcTable1 = new TableView( Rect( 0, 325, 99, 500 ), "cc_t1", "Center", 1, 2 );
  TableView* pcTable2 = new TableView( Rect( 0, 325, 99, 500 ), "cc_t2", NULL, 2, 1 );
  TableView* pcTable3 = new TableView( Rect( 0, 325, 99, 500 ), "cc_t3", NULL, 2, 2 );

  pcTable1->SetChild( pcTable2, 0, 0 );
  pcTable1->SetChild( pcTable3, 0, 1 );

  pcTable2->SetChild( new StringView( cRect, "cc_s1", "Active(X):" ), 0, 0 );
  pcTable2->SetChild( new CheckBox( cRect, "CENTER_ACTIVE", "", new Message( GID_CENTER_ACTIVE ) ), 1, 0 );

  pcTable3->SetChild( new StringView( cRect, "cc_s2", "Magn:" ), 0, 0 );
  pcTable3->SetChild( new Spinner( cRect, "CENTER_MAGN", 10.0f, new Message( GID_CENTER_MAGN ) ), 1, 0 );

  pcTable3->SetChild( new StringView( cRect, "cc_s3", "Damp:" ), 0, 1 );
  pcTable3->SetChild( new Spinner( cRect, "CENTER_DAMP", 0.0f, new Message( GID_CENTER_DAMP ) ), 1, 1 );

  return( pcTable1 );
}

TableView* CreatePtAttract()
{
  const Rect	cRect( 0, 0, 1, 1 );

  TableView* pcTable1 = new TableView( Rect( 0, 325, 99, 500 ), "cp_t1", "PtAttract", 1, 2 );
  TableView* pcTable2 = new TableView( Rect( 0, 325, 99, 500 ), "cp_t2", NULL, 2, 1 );
  TableView* pcTable3 = new TableView( Rect( 0, 325, 99, 500 ), "cp_t3", NULL, 2, 2 );

  pcTable1->SetChild( pcTable2, 0, 0 );
  pcTable1->SetChild( pcTable3, 0, 1 );

  pcTable2->SetChild( new StringView( cRect, "cp_s1", "Active(p):" ), 0, 0 );
  pcTable2->SetChild( new CheckBox( cRect, "ATTRA_ACTIVE", "", new Message( GID_ATTRA_ACTIVE ) ), 1, 0 );

  pcTable3->SetChild( new StringView( cRect, "cp_s2", "Magn:" ), 0, 0 );
  pcTable3->SetChild( new Spinner( cRect, "ATTRA_MAGN", 10.0f, new Message( GID_ATTRA_MAGN ) ), 1, 0 );

  pcTable3->SetChild( new StringView( cRect, "cp_s3", "Expo:" ), 0, 1 );
  pcTable3->SetChild( new Spinner( cRect, "ATTRA_EXPO", 0.0f, new Message( GID_ATTRA_EXPO ) ), 1, 1 );

  return( pcTable1 );
}

TableView* CreateWallForce()
{
  const Rect	cRect( 0, 0, 1, 1 );

  TableView* pcTable1 = new TableView( Rect( 0, 325, 99, 500 ), "wf_t1", "WallForce", 1, 2 );
  TableView* pcTable2 = new TableView( Rect( 0, 325, 99, 500 ), "wf_t2", NULL, 2, 1 );
  TableView* pcTable3 = new TableView( Rect( 0, 325, 99, 500 ), "wf_t3", NULL, 2, 2 );

  pcTable1->SetChild( pcTable2, 0, 0 );
  pcTable1->SetChild( pcTable3, 0, 1 );

  pcTable2->SetChild( new StringView( cRect, "wf_s1", "Active(w):" ), 0, 0 );
  pcTable2->SetChild( new CheckBox( cRect, "WFORCE_ACTIVE", "", new Message( GID_WFORCE_ACTIVE ) ), 1, 0 );

  pcTable3->SetChild( new StringView( cRect, "wf_s2", "Magn:" ), 0, 0 );
  pcTable3->SetChild( new Spinner( cRect, "WFORCE_MAGN", 10.0f, new Message( GID_WFORCE_MAGN ) ), 1, 0 );

  pcTable3->SetChild( new StringView( cRect, "wf_s3", "Expo:" ), 0, 1 );
  pcTable3->SetChild( new Spinner( cRect, "WFORCE_EXPO", 0.0f, new Message( GID_WFORCE_EXPO ) ), 1, 1 );

  return( pcTable1 );
}

Window* MyApp::OpenWindow( Rect cFrame, const char* pzPath )
{
  g_pcBitmap = new Bitmap( 800, 600, CS_RGB16, Bitmap::ACCEPT_VIEWS );
  g_pcDrawWid = new View( g_pcBitmap->GetBounds(), "xs_bm_wid" );
  g_pcBitmap->AddChild( g_pcDrawWid );

  g_pcWindow = new MyWindow( cFrame, "aspringies_main_wnd", "ASpringies V1.1", 0 );

  Rect cWndBounds = g_pcWindow->GetBounds();
  
  Rect cMenuFrame = cWndBounds;
  Rect cMainFrame = cWndBounds;

  cMainFrame.top += 16;

  Menu*	pcMenu	= new Menu( cMenuFrame, "Menu", ITEMS_IN_ROW,
			    CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT | CF_FOLLOW_TOP, WID_FULL_UPDATE_ON_H_RESIZE );

  Menu*	pcItem1	= new Menu( Rect( 0, 0, 100, 20 ), "File", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
  Menu*	pcItem2	= new Menu( Rect( 0, 0, 100, 20 ), "Edit", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
  Menu*	pcItem3	= new Menu( Rect( 0, 0, 100, 20 ), "State", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
  Menu*	pcItem4	= new Menu( Rect( 0, 0, 100, 20 ), "Sub", ITEMS_IN_COLUMN, CF_FOLLOW_LEFT | CF_FOLLOW_TOP );

  pcItem1->AddItem( "About...", new Message( MID_ABOUT ) );
  pcItem1->AddItem( "Help...", new Message( MID_HELP ) );
  pcItem1->AddItem( "Load...", new Message( MID_LOAD ) );
  pcItem1->AddItem( "Insert...", new Message( MID_INSERT ) );
  pcItem1->AddItem( "Save", new Message( MID_SAVE ) );

  pcItem1->AddItem( "Save as...", new Message( MID_SAVE_AS ) );
  pcItem1->AddItem( "Quit...", new Message( MID_QUIT ) );

  pcItem2->AddItem( "Duplicate", new Message( MID_DUP_SEL ) );
  pcItem2->AddItem( "Delete", new Message( MID_DELETE_SEL ) );

  pcItem3->AddItem( "Restore...", new Message( MID_RESTORE ) );
  pcItem3->AddItem( "Snapshot", new Message( MID_SNAPSHOT ) );
  pcItem3->AddItem( "Reset...", new Message( MID_RESET ) );

  pcItem4->AddItem( "Restore...", new Message( MID_RESTORE ) );
  pcItem4->AddItem( "Snapshot", new Message( MID_SNAPSHOT ) );
  pcItem4->AddItem( "Reset...", new Message( MID_RESET ) );

  pcItem3->AddItem( pcItem4 );

  pcMenu->AddItem( pcItem1 );
  pcMenu->AddItem( pcItem2 );
  pcMenu->AddItem( pcItem3 );

  cMenuFrame.bottom = pcMenu->GetPreferredSize( false ).y - 1;
  cMainFrame.top = cMenuFrame.bottom + 1;
  pcMenu->SetFrame( cMenuFrame );
  
//  g_pcWindow->SetMenuBar( pcMenu );
	
  pcMenu->SetTargetForItems( g_pcWindow );

  TableView*	pcTable1 = new TableView( cMainFrame, "", NULL, 2, 1, CF_FOLLOW_ALL );
  TableView*	pcTable2 = new TableView( Rect( 0, 0, 1, 1 ), "", NULL, 1, 9 );
  RollupWidget*	pcRollup = new RollupWidget( Rect( 0, 0, 1, 1 ), "", pcTable2 );

  pcTable1->SetCellBorders( 0, 0, 5, 0, 5, 0 );
  pcTable1->SetCellBorders( 1, 0, 0, 0, 0, 0 );

  pcTable1->SetChild( pcRollup, 0, 0 );

  pcTable2->SetChild( CreateEditMode(), 0, 0 );
  pcTable2->SetChild( CreateControl(), 0, 1 );
  pcTable2->SetChild( CreateSMSettings(), 0, 2 );
  pcTable2->SetChild( CreateGlobalSettings(), 0, 3 );
  pcTable2->SetChild( CreateWalls(), 0, 4 );
  pcTable2->SetChild( CreateGravity(), 0, 5 );
  pcTable2->SetChild( CreateCenter(), 0, 6 );
  pcTable2->SetChild( CreatePtAttract(), 0, 7 );
  pcTable2->SetChild( CreateWallForce(), 0, 8 );

  pcTable2->SetColAlignment( 0, ALIGN_CENTER );

  g_pcWndDrawWid = new SpringView( Rect( 0, 0, 1, 1 ) );

  pcTable1->SetChild( g_pcWndDrawWid, 1, 0 );

  g_pcWindow->AddChild( pcMenu );
  g_pcWindow->AddChild( pcTable1 );

  g_pcWindow->Show();

  return( g_pcWindow );
}








SpringView::SpringView( const Rect& cFrame ) :
    View( cFrame, "XSpringiInterface", CF_FOLLOW_ALL, WID_WILL_DRAW )
{
}

SpringView::~SpringView()
{
}


void SpringView::FrameSized( const Point& cDelta )
{
    Rect cBounds = GetBounds();

    draw_wid = int(cBounds.Width()) + 1;
    draw_ht  = int(cBounds.Height()) + 1;

    if ( g_pcBitmap != NULL ) {
	g_pcBitmap->RemoveChild( g_pcDrawWid );
	g_pcBitmap->Sync();
	Sync();
	delete g_pcBitmap;
	g_pcBitmap = new Bitmap( draw_wid, draw_ht, CS_RGB16, Bitmap::ACCEPT_VIEWS );
	g_pcDrawWid->SetFrame( g_pcBitmap->GetBounds() );
	g_pcBitmap->AddChild( g_pcDrawWid );
    }
    
    reset_bbox();

    review_system(TRUE);
}

void mouse_vel( int *mx, int *my )
{
  int i, totalx = 0, totaly = 0, scale = 0, dx, dy, dt;
  int fudge = 256;

  for (i = 0; i < MOUSE_PREV - 1; i++) {
    dx = mprev[(moffset + 2 + i) % MOUSE_PREV].x - mprev[(moffset + 1 + i) % MOUSE_PREV].x;
    dy = mprev[(moffset + 2 + i) % MOUSE_PREV].y - mprev[(moffset + 1 + i) % MOUSE_PREV].y;
    dt = mprev[(moffset + 2 + i) % MOUSE_PREV].t - mprev[(moffset + 1 + i) % MOUSE_PREV].t;

    if (dt) {
      scale += 64 * i * i;
      totalx += 64 * i * i * fudge * dx / dt;
      totaly += 64 * i * i * fudge * dy / dt;
    }
  }

  if (scale) {
    totalx /= scale;
    totaly /= scale;
  }

  *mx = totalx;
  *my = totaly;
}


void SpringView::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
  static int	nTime = 0;
  if ( lock_semaphore( g_hLock ) == 0 )
  {
    mprev[moffset].x = int(cNewPos.x);
    mprev[moffset].y = int(cNewPos.y);
    mprev[moffset].t = (unsigned long) nTime++;
    moffset = (moffset + 1) % MOUSE_PREV;

      /* Process motion notify event */
    mouse_event( M_DRAG, int(cNewPos.x), int(cNewPos.y), AnyButton, false );
    unlock_semaphore( g_hLock );
  }
}

void SpringView::MouseDown( const Point& cPosition, uint32 nButtons )
{
  MakeFocus( true );

  int	nButton = AnyButton;

  if (nButtons & MOUSE_BUT_LEFT && nButtons & MOUSE_BUT_RIGHT ) {
    nButton = Button2;
  } else {
    if (nButtons & MOUSE_BUT_LEFT) {
      nButton = Button1;
    } else {
      if (nButtons & MOUSE_BUT_RIGHT) {
	nButton = Button3;
      }
    }
  }

  if ( lock_semaphore( g_hLock ) == 0 )
  {
    memset( mprev, 0, sizeof(mprev));
    mouse_event(M_DOWN, int(cPosition.x), int(cPosition.y), nButton /*event.xbutton.button*/, /*event.xbutton.state & ShiftMask */ 0 );
    unlock_semaphore( g_hLock );
  }
}

void SpringView::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
  int	nButton = AnyButton;

  if (nButtons & MOUSE_BUT_LEFT && nButtons & MOUSE_BUT_RIGHT ) {
    nButton = Button2;
  } else {
    if (nButtons & MOUSE_BUT_LEFT) {
      nButton = Button1;
    } else {
      if (nButtons & MOUSE_BUT_RIGHT) {
	nButton = Button3;
      }
    }
  }

  if ( lock_semaphore( g_hLock ) == 0 )
  {
    mouse_event( M_UP, int(cPosition.x), int(cPosition.y), nButton /* event.xbutton.button */ , FALSE );
    unlock_semaphore( g_hLock );
  }
}

Point SpringView::GetPreferredSize( bool bLargest ) const
{
  if ( bLargest ) {
    return( Point( 100000, 100000 ) );
  } else {
    return( Point( 20, 20 ) );
  }
}

void SpringView::KeyDown( int lKeyCode, uint32 lQualifier )
{
}

void SpringView::Paint( const Rect& cUpdateRect )
{
  view_system();
}
