#include <gui/checkbox.h>
#include <gui/slider.h>
#include <gui/stringview.h>
#include <gui/window.h>

#include <util/application.h>

using namespace os;

namespace os {
  class	Button;
  class FileRequester;
};

class	MyWindow;

enum	{ AnyButton, Button1, Button2, Button3 };


void reset_bbox();
void calc_bbox();
void mouse_event( int type, int mx, int my, int mbutton, int shifted );
void mouse_vel( int *mx, int *my );
void view_system();

extern	int	action;
extern	int	mode;
extern	sem_id	g_hLock;

extern	MyWindow*	g_pcWindow;
extern	Bitmap*		g_pcBitmap;
extern	View*		g_pcDrawWid;
extern	View*		g_pcWndDrawWid;

extern	thread_id	g_hUpdateThread;
extern	volatile bool			g_bQuit;

#define NAIL_SIZE	4

enum
{
  M_EDIT,
  M_MASS,
  M_SPRING
};


enum
{
    // Control
  GID_SELECT_ALL,
  GID_SET_CENTER,
  GID_SET_REST_LEN,
  GID_DRAW_SPRING,
  GID_ACTION,
  GID_GRID,

    // S&M settings
  GID_MASS,
  GID_ELAS,
  GID_KSPR,
  GID_KDMP,
  GID_FIXED_MASS,

    // Global settings
  GID_VISC,
  GID_STIC,
  GID_STEP,
  GID_PREC,
  GID_ADAPT_TIME_STEP,

    // Edit modes
  GID_MODE_EDIT,
  GID_MODE_MASS,
  GID_MODE_SPRING,

    // Walls
  GID_W_LEFT,
  GID_W_RIGHT,
  GID_W_TOP,
  GID_W_BOTTOM,

    // Gravity
  GID_GRAV_ACTIVE,
  GID_GRAV_GRAV,
  GID_GRAV_DIR,

    // Center
  GID_CENTER_ACTIVE,
  GID_CENTER_MAGN,
  GID_CENTER_DAMP,

    // PtAttract
  GID_ATTRA_ACTIVE,
  GID_ATTRA_MAGN,
  GID_ATTRA_EXPO,

    // Wall force
  GID_WFORCE_ACTIVE,
  GID_WFORCE_MAGN,
  GID_WFORCE_EXPO,

  MID_ABOUT,
  MID_HELP,

  MID_LOAD,
  MID_INSERT,
  MID_SAVE,
  MID_SAVE_AS,
  MID_DUP_SEL,
  MID_DELETE_SEL,
  MID_RESTORE,

  MID_SNAPSHOT,
  MID_RESET,
  MID_QUIT,

  GID_COUNT,
  
};


class	MyWindow : public Window
{
public:
  MyWindow( Rect cFrame, const char* pzName, const char* pzTitle, int nFlags );

  void	HandleMessage( Message* pcMessage );

  void	SetCheckBox( const char* pzName, bool bChecked );
  void	SetSlider( const char* pzName, int nValue );
  void	SetFL( const char* pzName, double vValue );
  void	UpdateWidgets( void );

private:
  FileRequester* m_pcLoadRequester;
  FileRequester* m_pcSaveRequester;
};

class	MyApp : public Application
{
public:
  MyApp();
  ~MyApp();


// From Looper
  bool					OkToQuit();


/*	void	DispatchMessage( Message* pcMessage, Handler* pcHandler );	*/

// From Application


// From MyApp
  Window*	OpenWindow( Rect cFrame, const char* pzPath );

private:
};

class	SpringView : public View
{
public:
  SpringView( const Rect& cFrame );
  ~SpringView();

  void		FrameSized( const Point& cDelta );
  Point		GetPreferredSize( bool bLargest ) const;
  void		MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
  void		MouseDown( const Point& cPosition, uint32 nButtons );
  void		MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
  void		KeyDown( int nKeyCode, uint32 nQualifiers );
  void		Paint( const Rect& cUpdateRect );

private:

};


class	RollupWidget : public View
{
public:
	RollupWidget( const Rect& cFrame, const char* pzName, View* pcChild, uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
	~RollupWidget();

	// From View:
//	void			AttachedToWindow( void );
  void	   AllAttached( void );
  void	   Paint( const Rect& cUpdateRect );
  Point GetPreferredSize( bool bLargest ) const;

  void	MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
  void	MouseDown( const Point& cPosition, uint32 nButtons );
  void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );

private:
  View*		m_pcWidget;
  Point		m_pcPrefSize;
  bool		m_bTrack;
  Point		m_cHitPos;
};

class	NumView : public StringView
{
public:
	NumView( const char* pzName, int nNum );

//	Point	GetPreferredSize( bool bLargest ) const;
	void	AttachedToWindow( void );

	void	SetNumber( int nValue );

private:
  Point	m_cPrefSize;
};
