#include <time.h>
#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/kernel.h>
#include <gui/bitmap.h>

#include "athgui.h"

#include "defs.h"

#include "obj.h"
#include "bitmap.h"
#include "title.h"
#include "bfbm.h"

char*	g_pzPath;


//#define M_ACTION	3

int	mode	=	M_EDIT;


int	action = -1;

boolean active = FALSE;

/* Number of previous mouse state saves */

sem_id	g_hLock = -1;
thread_id	g_hUpdateThread = -1;
volatile bool	g_bQuit = false;

/* Current and previous bounding box */
int bb_ulx, bb_uly, bb_lrx, bb_lry;
int bbo_ulx, bbo_uly, bbo_lrx, bbo_lry;
extern boolean bboxing;


/* Current mouse grab for spring */
int static_spring = FALSE;
int startx = 0, prevx = 0, starty = 0, prevy = 0;

int selection = -1;



void view_system()
{
  g_pcBitmap->Sync();
  Rect cRect = g_pcWndDrawWid->GetNormalizedBounds();
  g_pcWndDrawWid->DrawBitmap( g_pcBitmap, cRect, cRect );
  g_pcWindow->Sync();
}

void view_subsystem( int ulx, int uly, int wid, int ht )
{
  if (ulx < 0) {
    wid += ulx;
    ulx = 0;
  }
  if (uly < 0) {
    ht += uly;
    uly = 0;
  }
  if (wid < 0 || ht < 0)
    return;

  if (ulx + wid >= draw_wid)
    wid -= (ulx + wid - draw_wid);
  if (uly + ht >= draw_ht)
    ht -= (uly + ht - draw_ht);

  if (wid < 0 || ht < 0)
    return;

  g_pcBitmap->Sync();
  g_pcWndDrawWid->DrawBitmap( g_pcBitmap, Rect( ulx, uly, ulx + wid - 1, uly + ht - 1 ), Rect( ulx, uly, ulx + wid - 1, uly + ht - 1 ) );
  g_pcWindow->Sync();

//	XCopyPlane( dpy, draw_pm, draw_win, fggc, ulx, uly, wid, ht, ulx, uly, 0x1 );
}

static void draw_spring( View* pcWid, int x1, int y1, int x2, int y2)
{
  pcWid->SetFgColor( 0, 0, 0, 0 );
  pcWid->DrawLine( Point( x1, y1 ), Point( x2, y2 ) );
  pcWid->Flush();
}


static void draw_mass( View* pcWid, int mx, int my, double m )
{
  int rad;

  rad = mass_radius(m);

  pcWid->SetFgColor( 0, 0, 0, 0 );
  pcWid->FillRect( Rect( mx - rad, my - rad, mx + rad, my + rad ) );
}

void reset_bbox()
{
  bb_ulx = bbo_ulx = bb_uly = bbo_uly = 0;
  bb_lrx = bbo_lrx = draw_wid - 1;
  bb_lry = bbo_lry = draw_ht - 1;
}

void calc_bbox()
{
	int i, rad, bb_temp;
	boolean first = TRUE, fixed;

	if (bboxing)
	{
		if (static_spring)
		{
	    first = FALSE;
	    bb_ulx = MIN(startx, prevx);
	    bb_uly = MIN(starty, prevy);
	    bb_lrx = MAX(startx, prevx);
	    bb_lry = MAX(starty, prevy);
		}
		for (i = 0; i < num_mass; i++)
		{
	  	if ((masses[i].status & S_ALIVE) || (i == 0 && springs[0].status & S_ALIVE))
			{
				fixed = (masses[i].status & S_FIXED) && !(masses[i].status & S_TEMPFIXED);
				rad = fixed ? NAIL_SIZE : masses[i].radius;

				if (i == mst.center_id)
		  		rad += 1;

				/* Make sure bounding box includes mass */
				if (first)
				{
			    bb_ulx = COORD_X(masses[i].x - rad);
			    bb_uly = COORD_Y(masses[i].y + rad);
			    bb_lrx = COORD_X(masses[i].x + rad);
			    bb_lry = COORD_Y(masses[i].y - rad);
			    first = FALSE;
				}
				else
				{
			    if ((bb_temp = COORD_X(masses[i].x - rad)) < bb_ulx)
			      bb_ulx = bb_temp;
			    if ((bb_temp = COORD_Y(masses[i].y + rad)) < bb_uly)
			      bb_uly = bb_temp;
			    if ((bb_temp = COORD_X(masses[i].x + rad)) > bb_lrx)
			      bb_lrx = bb_temp;
			    if ((bb_temp = COORD_Y(masses[i].y - rad)) > bb_lry)
			      bb_lry = bb_temp;
				}
	    }
		}

		/* Add bounding strip */
		bb_ulx -= spthick + 1;
		bb_uly -= spthick + 1;
		bb_lrx += spthick + 1;
		bb_lry += spthick + 1;

		/* Bound within screen */
		if (bb_ulx < 0)
		  bb_ulx = 0;
		if (bb_lrx < 0)
		  bb_lrx = 0;

		if (bb_uly < 0)
		  bb_uly = 0;
		if (bb_lry < 0)
		  bb_lry = 0;

		if (bb_ulx >= draw_wid)
		  bb_ulx = draw_wid - 1;
		if (bb_lrx >= draw_wid)
		  bb_lrx = draw_wid - 1;

		if (bb_uly >= draw_ht)
		  bb_uly = draw_ht - 1;
		if (bb_lry >= draw_ht)
		  bb_lry = draw_ht - 1;

		/* Intersect with previous bbox, and set old box to new */
		if (bbo_ulx < (bb_temp = bb_ulx))
		  bb_ulx = bbo_ulx;
		bbo_ulx = bb_temp;

		if (bbo_uly < (bb_temp = bb_uly))
		  bb_uly = bbo_uly;
		bbo_uly = bb_temp;

		if (bbo_lrx > (bb_temp = bb_lrx))
		  bb_lrx = bbo_lrx;
		bbo_lrx = bb_temp;

		if (bbo_lry > (bb_temp = bb_lry))
		  bb_lry = bbo_lry;
		bbo_lry = bb_temp;
	}
}



void mouse_event( int type, int mx, int my, int mbutton, int shifted )
{
  static int selection = -1, cur_button = 0;
  static boolean active = FALSE, cur_shift = FALSE;
    /* Skip restarts when active or continuations when inactive */
  if ((type != M_DOWN && !active) || (type == M_DOWN && active))
    return;

  
    /* Do grid snapping operation */
  if (mst.grid_snap && mode != M_EDIT) {
    mx = ((mx + (int)mst.cur_gsnap / 2) / (int)mst.cur_gsnap) * (int)mst.cur_gsnap;
    my = ((my + (int)mst.cur_gsnap / 2) / (int)mst.cur_gsnap) * (int)mst.cur_gsnap;
  }

  printf( "Got mouse down %d (%d)\n", type, mode );
  switch (type)
  {
    case M_REDISPLAY:
      switch (mode)
      {
	case M_EDIT:
	  switch (cur_button)
	  {
	    case Button1:
	      if (selection < 0) {
//------------------------------------		    		XDrawRectangle(dpy, draw_win, selectboxgc, MIN(startx, prevx), MIN(starty, prevy), ABS(prevx - startx), ABS(prevy - starty));
	      }
	      break;
	    case Button2:
	      break;
	    case Button3:
	      break;
	  }
	  break;
	case M_MASS:
	  draw_mass( g_pcWndDrawWid, prevx, prevy, mst.cur_mass);
	  break;
	case M_SPRING:
	  if (static_spring) {
	    startx = COORD_X(masses[selection].x);
	    starty = COORD_Y(masses[selection].y);
	    draw_spring( g_pcWndDrawWid, startx, starty, prevx, prevy);
	  }
	  break;
      }
      break;

    case M_DOWN:
	printf( "Got mouse down: %d\n", mode );
      review_system(TRUE);
      startx = prevx = mx;
      starty = prevy = my;
      cur_button = mbutton;
      active = TRUE;
      cur_shift = shifted;

      switch (mode)
      {
	case M_EDIT:
	  switch (cur_button)
	  {
	    case Button1:
	    {
	      boolean is_mass = FALSE;
	      selection = nearest_object(COORD_X(mx), COORD_Y(my), &is_mass);

		/* If not shift clicking, unselect all currently selected items */
	      if (!shifted) {
		unselect_all();
		review_system(TRUE);
	      }
	    }
	    break;
	    case Button2:
	    case Button3:
	      tempfixed_obj(TRUE);
	      break;
	  }
	  break;
	case M_MASS:
	  draw_mass( g_pcWndDrawWid, mx, my, mst.cur_mass);
	  break;
	case M_SPRING:
	{
	    printf( "Down in spring\n" );
	  boolean is_mass = TRUE;

	  static_spring = (action == -1 || cur_button == Button3);

	  selection = nearest_object(COORD_X(mx), COORD_Y(my), &is_mass);
	  if (selection >= 0 && is_mass) {
	    startx = COORD_X(masses[selection].x);
	    starty = COORD_Y(masses[selection].y);
	    if (static_spring) {
	      draw_spring( g_pcWndDrawWid, startx, starty, prevx, prevy);
	    } else {
	      attach_fake_spring(selection);
	      move_fake_mass(COORD_X(mx), COORD_Y(my));
	      review_system(TRUE);
	    }
	  } else {
	    active = FALSE;
	  }
	    printf( "Down in spring DONE\n" );
	}
	break;
      }
      break;

    case M_DRAG:
      switch (mode)
      {
	case M_EDIT:
	  switch (cur_button)
	  {
	    case Button1:
	      if (selection < 0) {
		view_subsystem(MIN(startx, prevx), MIN(starty, prevy), ABS(prevx - startx) + 1, ABS(prevy - starty) + 1);
		prevx = mx;
		prevy = my;
//--------------------------------				    XDrawRectangle(dpy, draw_win, selectboxgc, MIN(startx, prevx), MIN(starty, prevy),   ABS(prevx - startx), ABS(prevy - starty));
	      }
	      break;
	    case Button2:
	    case Button3:
		/* Move objects relative to mouse */
	      translate_selobj(COORD_DX(mx - prevx), COORD_DY(my - prevy));
	      review_system(TRUE);
	      prevx = mx;
	      prevy = my;
	      break;
	  }
	  break;
	case M_MASS:
	{
	  int rad = mass_radius(mst.cur_mass);
	  view_subsystem(prevx - rad, prevy - rad, rad * 2 + 1, rad * 2 + 1);
	}
	prevx = mx;
	prevy = my;
	draw_mass( g_pcWndDrawWid, prevx, prevy, mst.cur_mass);
	break;
	case M_SPRING:
	  if (static_spring)
	  {
	    view_subsystem(MIN(startx, prevx), MIN(starty, prevy), ABS(prevx - startx) + 1, ABS(prevy - starty) + 1);
	    prevx = mx;
	    prevy = my;
	    startx = COORD_X(masses[selection].x);
	    starty = COORD_Y(masses[selection].y);
	    draw_spring( g_pcWndDrawWid, startx, starty, prevx, prevy);
	  } else {
	    move_fake_mass(COORD_X(mx), COORD_Y(my));
	  }
	  break;
      }
      break;

    case M_UP:
      active = FALSE;
      switch (mode)
      {
	case M_EDIT:
	  switch (cur_button)
	  {
	    case Button1:
	      if (selection < 0) {
		select_objects(MIN(COORD_X(startx), COORD_X(mx)), MIN(COORD_Y(starty), COORD_Y(my)),
			       MAX(COORD_X(startx), COORD_X(mx)), MAX(COORD_Y(starty), COORD_Y(my)), cur_shift);
		eval_selection();
		review_system(TRUE);
	      } else {
		boolean is_mass = FALSE;

		if ((selection = nearest_object(COORD_X(mx), COORD_Y(my), &is_mass)) >= 0) {
		  select_object(selection, is_mass, cur_shift);
		  eval_selection();
		  review_system(TRUE);
		}
	      }
	      break;
	    case Button2:
	      tempfixed_obj(FALSE);
	      review_system(TRUE);
	      break;
	    case Button3:
	    {
	      int mvx, mvy;

	      mouse_vel(&mvx, &mvy);

	      changevel_selobj(COORD_DX(mvx), COORD_DY(mvy), FALSE);
	      tempfixed_obj(FALSE);
	      review_system(TRUE);
	    }
	    break;
	  }
	  break;
	case M_MASS:
	{
	  int newm;

	  newm = create_mass();

	  masses[newm].x = COORD_X((double)mx);
	  masses[newm].y = COORD_Y((double)my);
	  masses[newm].mass = mst.cur_mass;
	  masses[newm].radius = mass_radius(mst.cur_mass);
	  masses[newm].elastic = mst.cur_rest;
	  if (mst.fix_mass) masses[newm].status |= S_FIXED;

	  review_system(TRUE);
	}
	break;

	case M_SPRING:
	{
	  boolean is_mass = TRUE;
	  int start_sel = selection, newsel, endx, endy;

	  if (!static_spring) {
	    kill_fake_spring();
	  }

	  selection = nearest_object(COORD_X(mx), COORD_Y(my), &is_mass);
	  if ((static_spring || action == -1 || cur_button == Button1) && selection >= 0 && is_mass && selection != start_sel) {
	    startx = COORD_X(masses[start_sel].x);
	    starty = COORD_Y(masses[start_sel].y);
	    endx = COORD_X(masses[selection].x);
	    endy = COORD_Y(masses[selection].y);

	    newsel = create_spring();
	    springs[newsel].m1 = start_sel;
	    springs[newsel].m2 = selection;
	    springs[newsel].ks = mst.cur_ks;
	    springs[newsel].kd = mst.cur_kd;
	    springs[newsel].restlen = sqrt((double)SQR(startx - endx) + (double)SQR(starty - endy));

	    add_massparent(start_sel, newsel);
	    add_massparent(selection, newsel);
	  }

	  review_system(TRUE);
	}
	break;
      }
      break;
  }
//    XFlush(dpy);
}


extern "C" void review_system( boolean reset )
{

  if (reset) {
    reset_bbox();
  } else {
    calc_bbox();
  }

    /* Clear the old pixmap */
/*  XFillRectangle(dpy, draw_pm, erasegc, bb_ulx, bb_uly, bb_lrx - bb_ulx + 1, bb_lry - bb_uly + 1);	*/

  g_pcWindow->Lock();

  g_pcDrawWid->SetFgColor( 255, 255, 255, 0 );
  g_pcDrawWid->FillRect( Rect( bb_ulx, bb_uly, bb_lrx, bb_lry ) );


  redraw_system();
  view_system();
  g_pcWindow->Unlock();
  mouse_event(M_REDISPLAY, 0, 0, AnyButton, FALSE);

/*
  if (mst.adaptive_step) {
  update_slider(slid_dt_num);
  }
  XFlush(dpy);
  */
}

void redisplay_widgets()
{
  if ( g_pcWindow->Lock() == 0 )
  {
    g_pcWindow->UpdateWidgets();
    g_pcWindow->Unlock();
  }
}

void redraw_system()
{
  int i, rad, num_spr;
  boolean fixed;

  num_spr = (mst.show_spring) ? num_spring : 1;

  g_pcDrawWid->SetFgColor( 0, 0, 0, 0 );

  for (i = 0; i < num_spr; i++)
  {
    if (springs[i].status & S_ALIVE)
    {
      if (springs[i].status & S_SELECTED) {
	g_pcDrawWid->SetFgColor( 0, 0, 255, 0 );
      } else {
	g_pcDrawWid->SetFgColor( 0, 0, 0, 0 );
      }
      g_pcDrawWid->DrawLine( Point( COORD_X(masses[springs[i].m1].x), COORD_Y(masses[springs[i].m1].y) ),
			     Point( COORD_X(masses[springs[i].m2].x), COORD_Y(masses[springs[i].m2].y) ) );
    }
  }

  for (i = 0; i < num_mass; i++)
  {
    if (masses[i].status & S_ALIVE)
    {
      fixed = (masses[i].status & S_FIXED) && !(masses[i].status & S_TEMPFIXED);
      rad = fixed ? NAIL_SIZE : masses[i].radius;

	/* Check if within bounds */
      if (COORD_X(masses[i].x + rad) >= 0 && COORD_X(masses[i].x - rad) <= draw_wid &&
	  COORD_Y(masses[i].y - rad) >= 0 && COORD_Y(masses[i].y + rad) <= draw_ht)
      {
	  /* Get cur_arc value based on if FIXED or SELECTED */

	if ( masses[i].status & S_SELECTED ) {
	  g_pcDrawWid->SetFgColor( 0, 0, 255, 0 );
	} else {
	  g_pcDrawWid->SetFgColor( 0, 0, 0, 0 );
	}
/*
  if (fixed) {
  }
  */
	g_pcDrawWid->FillRect( Rect( COORD_X(masses[i].x) - rad, COORD_Y(masses[i].y) - rad, COORD_X(masses[i].x) + rad, COORD_Y(masses[i].y) + rad ) );
      }
    }
  }

  if (mst.center_id != -1)
  {
    i = mst.center_id;

    g_pcDrawWid->SetFgColor( 0, 255, 0, 0 );
    rad = ((masses[i].status & S_FIXED) && !(masses[i].status & S_TEMPFIXED)) ? NAIL_SIZE : masses[i].radius;
    g_pcDrawWid->FillRect( Rect( COORD_X(masses[i].x) - rad, COORD_Y(masses[i].y) - rad, COORD_X(masses[i].x) + rad, COORD_Y(masses[i].y) + rad ) );
  }
}

void	UpdateFrameCounter( int nValue )
{
  g_pcWindow->Lock();
  NumView* pcNumView = (NumView*) g_pcWindow->FindView( "FRAME_RATE_VIEW" );

  if ( NULL != pcNumView ) {
    pcNumView->SetNumber( nValue );
  }
  g_pcWindow->Unlock();
}

int UpdateThread( void* pData )
{
  file_command( g_pzPath, F_LOAD );


  draw_wid	= 400;
  draw_ht		= 400;

  reset_bbox();


  if ( g_pcWindow->Lock() == 0 )
  {
    g_pcWindow->UpdateWidgets();
    g_pcWindow->Unlock();
  }

  int	fps = 0;
  int	nPrevTime = 0;

  int	i = 0;



#define	MIN_DIFFT	100


  while ( false == g_bQuit )
  {
    if (action != -1 && animate_obj())
    {
      static bigtime_t	nTime, nOldTime;
      static int64 totaldiff = 0;
      static boolean started = FALSE;
      static uint64 fpscount;
      static float av_fps;
      int64 difft;

	/* Get time the first time through */
      if (!started) {
	nTime = get_system_time();
	started = TRUE;
      }
      nOldTime = nTime;

      nTime = get_system_time();
      
      difft = (nTime - nOldTime);

      if (difft < MIN_DIFFT)
	difft = MIN_DIFFT;

      if (difft > 0)
      {
	totaldiff += difft;

	fpscount++;
//	      av_fps += (uint)(1.e6/difft+.5);
	av_fps += ((float)1000000 / difft+.5);
	if (fpscount == 10)
	{
//					set(TX_Fps, MUIA_Text_Contents, (sprintf(temp, "%2ld", av_fps/10), temp));
	  UpdateFrameCounter( int(av_fps/10.0f) );
	  av_fps = fpscount = 0;
	}

	  /* Do updates over 30 frames per second (to make it smooth) */
//	if (totaldiff > 1000000 / 30 )
	{
	  totaldiff = 0;
	  review_system(FALSE);
//		    	XSync(dpy, False);
	}
      }
    }
    else
    {
      snooze( 500 );
    }
  }

/*
  for (;;)
  {
  if ( (i++ % 10) == 0 )
  {
  int	nTime = clock();
  int	nDiff = nTime - nPrevTime;
  nPrevTime = nTime;

  fps = (int) (10.0f / ((float) nDiff / (float) CLOCKS_PER_SEC));
  UpdateFrameCounter( fps );
  }

  while ( action == -1 ) {
  Delay( 500 );
  }
  if ( animate_obj() )	{
  review_system(FALSE);
  }
  }
  */
  return( 0 );
}


extern "C" void InitGUI( char* pzPath )
{
  MyApp* pcApp	= new MyApp;

  g_pzPath = pzPath;

  g_hLock = create_semaphore( "XSpringies", 1, SEM_REQURSIVE );


  pcApp->OpenWindow( Rect( 20, 20, 420, 420 ), "" );

  g_hUpdateThread = spawn_thread( "Update", UpdateThread, 0, 0, NULL );
  resume_thread( g_hUpdateThread );

  pcApp->Run();

}
