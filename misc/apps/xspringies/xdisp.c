/* xdisp.c -- Xlib display code for xspringies
 * Copyright (C) 1991,1992  Douglas M. DeCarlo
 *
 * This file is part of XSpringies, a mass and spring simulation system for X
 *
 * XSpringies is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * XSpringies is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XSpringies; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#if 0
#include "defs.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Xos.h>
#include "obj.h"
#include "bitmap.h"
#include "title.h"
#include "bfbm.h"

#ifdef VMS
#include "unix_time.h"
unsigned long int statvms;
unsigned long int LIB$WAIT();     /* VMS timer routine */
float seconds;
#endif /* VMS */

/* Mode values */
#define M_EDIT		0
#define M_MASS		1
#define M_SPRING	2

#define M_ACTION	3

/* Behavior function modes */
#define M_BF		4

/* Button values */
#define B_CENTER	0
#define B_RESTLEN       1
#define B_SELECTALL     2
#define B_DELETE        3
#define B_SAVESTATE	4
#define B_RESSTATE	5
#define B_DUPLICATE	6
#define B_INSERTFILE	7
#define B_LOADFILE	8
#define B_SAVEFILE      9
#define B_RESET	        10
#define B_QUIT		11

#define C_FIXEDMASS	0
#define C_GRIDSNAP	1
#define C_ADAPTIVE_STEP	2
#define C_WTOP		4
#define C_WLEFT		5
#define C_WRIGHT	6
#define C_WBOTTOM	7
#define C_SHOWSPRING	8

#define S_MASS		0
#define S_ELAS		1
#define S_KSPR		2
#define S_KDAMP		3
#define S_GRAV		4
#define S_GDIR		5
#define S_VISC		6
#define S_STICK		7
#define S_TIMESTEP	8
#define S_GRIDSNAP	9
#define S_PRECISION	10

/* Number of previous mouse state saves */
#define MOUSE_PREV	4

#define NAIL_SIZE	4

/* Number of milliseconds that each drawing takes (to prevent stops) */
#define MIN_DIFFT	100

/* Default highlight color for color screen */
#define HLCOLOR		"green"

/* Just in case someone has a 4.2 BSD system (you have my sympathy) */
#ifndef FD_SET
#define fd_set int
#define FD_SET(fd,fdset) (*(fdset) |= (1 << (fd)))
#define FD_CLR(fd,fdset) (*(fdset) &= ~(1 << (fd)))
#define FD_ISSET(fd, fdset) (*(fdset) & (1 << (fd)))
#define FD_ZERO(fdset) (*(fdset) = 0)
#endif

/* X globals */
Display *dpy;
Window main_win, text_win, acts_win, draw_win;
GC fggc, bggc, revgc, hlgc, selectboxgc;
GC sdrawgc, drawgc, erasegc, selectgc, selectlinegc;
Pixmap draw_pm, acts_pm, startup_pm;
int fcolor, bcolor, hcolor;
int main_wid, main_ht, draw_wid, draw_ht, planes, xfd;

/* Other globals */
static int mode = M_EDIT, slid_dt_num, cur_force = 0, action = -1;
static boolean quitting = FALSE;
int cursor_pos = 0, spthick = 0;

/* Current and previous bounding box */
int bb_ulx, bb_uly, bb_lrx, bb_lry;
int bbo_ulx, bbo_uly, bbo_lrx, bbo_lry;
boolean bboxing = TRUE;

/* Current mouse grab for spring */
int static_spring = FALSE;
int startx = 0, prevx = 0, starty = 0, prevy = 0;

t_state mst = {
    1.0, 1.0,
    1.0, 1.0,
    FALSE, TRUE,
    -1,
    { -1, -1, -1, -1 },
    { 10.0, 5.0, 10.0, 10000.0 },
    { 0.0, 2.0, 0.0, 1.0 },
    0.0, 0.0,
    DEF_TSTEP, 1.0,
    FALSE, FALSE,
    20.0,
    TRUE, TRUE, TRUE, TRUE
}, initst, sst;

static double cur_grav_max[BF_NUM] = { 10000000.0, 10000000.0, 10000000.0, 10000000.0 };
static double cur_grav_min[BF_NUM] = { 0.0, -10000000.0, -10000000.0, -10000000.0 };
static double cur_misc_max[BF_NUM] = { 360.0, 10000000.0, 1000.0, 1000.0 };
static double cur_misc_min[BF_NUM] = { -360.0, 0.0, 0.0, -0.0 };

/* Position of Gravity/Direction sliders */
int force_liney, grav_slidx, grav_slidy, dir_slidx, dir_slidy;

/* Mouse recording */
typedef struct {
    int x, y;
    unsigned long t;
} mouse_info;

static mouse_info mprev[MOUSE_PREV];
static int moffset = 0;
static void mouse_event();

/* File operations globals */
int file_op = F_NONE;
char filename[MAXPATH];

/* clean_up: free all of the X objects created
   like pixmaps and gcs
   */
static void clean_up()
{
    /* Free up X objects */
    XFreePixmap(dpy, draw_pm);
    XFreePixmap(dpy, acts_pm);
    XFreePixmap(dpy, startup_pm);

    XFreeGC(dpy, fggc);
    XFreeGC(dpy, bggc);
    XFreeGC(dpy, revgc);
    XFreeGC(dpy, hlgc);
    XFreeGC(dpy, selectboxgc);

    XFreeGC(dpy, drawgc);
    XFreeGC(dpy, sdrawgc);
    XFreeGC(dpy, erasegc);
    XFreeGC(dpy, selectgc);
    XFreeGC(dpy, selectlinegc);
    XCloseDisplay(dpy);
}

/* fatal(msg): print out the message msg, clean up,
   and exit
   */
void fatal(msg)
char *msg;
{
    if (msg != NULL)
      fprintf(stderr, msg);
    clean_up();
    exit(-1);
}

void disp_filename(cur_reset)
boolean cur_reset;
{
    char fbuff[MAXPATH + 256];
    int offset;

    if (cur_reset)
      cursor_pos = strlen(filename);

    switch (file_op) {
      case F_NONE:
	break;
      case F_LOAD:
	strcpy(fbuff, "Load file: ");
	break;
      case F_INSERT:
	strcpy(fbuff, "Insert file: ");
	break;
      case F_SAVE:
	strcpy(fbuff, "Save file: ");
	break;
    }

    XFillRectangle(dpy, text_win, bggc, 0, 0, draw_wid, T_HT);
    if (file_op != F_NONE) {
	int cur_offset;

	strncat(fbuff, filename, cursor_pos);

	if ((offset = strlen(fbuff) + 1 - (draw_wid - F_WID) / F_WID) < 0)
	  offset = 0;

	cur_offset = strlen(fbuff) - offset;

	strcat(fbuff, filename + cursor_pos);
	XFillRectangle(dpy, text_win, hlgc, cur_offset * F_WID + F_WID/2, 1, F_WID, F_HT);
	XDrawString(dpy, text_win, fggc, F_WID/2, F_HT, fbuff + offset, strlen(fbuff + offset));
    }
    XFlush(dpy);
}

void file_error()
{
    XBell(dpy, 50);
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

    if (bboxing) {
	if (static_spring) {
	    first = FALSE;
	    bb_ulx = MIN(startx, prevx);
	    bb_uly = MIN(starty, prevy);
	    bb_lrx = MAX(startx, prevx);
	    bb_lry = MAX(starty, prevy);
	}
	for (i = 0; i < num_mass; i++) {
	    if ((masses[i].status & S_ALIVE) || (i == 0 && springs[0].status & S_ALIVE)) {
		fixed = (masses[i].status & S_FIXED) && !(masses[i].status & S_TEMPFIXED);
		rad = fixed ? NAIL_SIZE : masses[i].radius;

		if (i == mst.center_id)
		  rad += 1;

		/* Make sure bounding box includes mass */
		if (first) {
		    bb_ulx = COORD_X(masses[i].x - rad);
		    bb_uly = COORD_Y(masses[i].y + rad);
		    bb_lrx = COORD_X(masses[i].x + rad);
		    bb_lry = COORD_Y(masses[i].y - rad);
		    first = FALSE;
		} else {
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

void draw_startup_picture()
{
    reset_bbox();

    /* Set up draw pixmap with startup picture */
    XFillRectangle(dpy, draw_pm, erasegc, 0, 0, draw_wid, draw_ht);

    XCopyArea(dpy, startup_pm, draw_pm, drawgc, 0, 0, startup_width, startup_height, (draw_wid - startup_width) / 2, (draw_ht - startup_height) / 2);
}

void redraw_system()
{
    static XArc *arcs = NULL, *selarcs = NULL, *circs = NULL, *selcircs = NULL;
    static XSegment *segs = NULL, *selsegs = NULL;
    static num_mass_alloc = 0, num_spring_alloc = 0;
    XArc *cur_arc;
    XSegment *cur_seg;
    int numarcs, numselarcs, numcircs, numselcircs, numsegs, numselsegs;
    int i, rad, num_spr;
    boolean fixed;

    /* Draw springs */
    numsegs = numselsegs = 0;
    if (num_spring_alloc < num_spring) {
	num_spring_alloc = num_spring;
	segs = (XSegment *)xrealloc(segs, num_spring_alloc * sizeof(XSegment));
	selsegs = (XSegment *)xrealloc(selsegs, num_spring_alloc * sizeof(XSegment));
    }
    num_spr = (mst.show_spring) ? num_spring : 1;

    for (i = 0; i < num_spr; i++) {
	if (springs[i].status & S_ALIVE) {
	    if (springs[i].status & S_SELECTED) {
		cur_seg = selsegs + numselsegs++;
	    } else {
		cur_seg = segs + numsegs++;
	    }

	    cur_seg->x1 = COORD_X(masses[springs[i].m1].x);
	    cur_seg->y1 = COORD_Y(masses[springs[i].m1].y);
	    cur_seg->x2 = COORD_X(masses[springs[i].m2].x);
	    cur_seg->y2 = COORD_Y(masses[springs[i].m2].y);
	}
    }
    if (numsegs) {
	XDrawSegments(dpy, draw_pm, sdrawgc, segs, numsegs);
    }
    if (numselsegs) {
	XDrawSegments(dpy, draw_pm, erasegc, selsegs, numselsegs);
	XDrawSegments(dpy, draw_pm, selectlinegc, selsegs, numselsegs);
    }

    /* Draw masses */
    numarcs = numselarcs = numcircs = numselcircs = 0;
    if (num_mass_alloc < num_mass) {
	num_mass_alloc = num_mass;
	arcs = (XArc *)xrealloc(arcs, num_mass_alloc * sizeof(XArc));
	selarcs = (XArc *)xrealloc(selarcs, num_mass_alloc * sizeof(XArc));
	circs = (XArc *)xrealloc(circs, num_mass_alloc * sizeof(XArc));
	selcircs = (XArc *)xrealloc(selcircs, num_mass_alloc * sizeof(XArc));
    }

    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_ALIVE) {
	    /* Check if within bounds */
	    fixed = (masses[i].status & S_FIXED) && !(masses[i].status & S_TEMPFIXED);
	    rad = fixed ? NAIL_SIZE : masses[i].radius;

	    if (COORD_X(masses[i].x + rad) >= 0 && COORD_X(masses[i].x - rad) <= draw_wid &&
		COORD_Y(masses[i].y - rad) >= 0 && COORD_Y(masses[i].y + rad) <= draw_ht) {
		/* Get cur_arc value based on if FIXED or SELECTED */
		if (masses[i].status & S_SELECTED) {
		    if (fixed) {
			cur_arc = selcircs + numselcircs++;
		    } else {
			cur_arc = selarcs + numselarcs++;
		    }
		} else {
		    if (fixed) {
			cur_arc = circs + numcircs++;
		    } else {
			cur_arc = arcs + numarcs++;
		    }
		}

		cur_arc->x = COORD_X(masses[i].x) - rad;
		cur_arc->y = COORD_Y(masses[i].y) - rad;
		cur_arc->width = cur_arc->height = rad * 2 + 1;
		cur_arc->angle1 = 0;
		cur_arc->angle2 = 64 * 360;
	    }
	}
    }
    /* Draw masses and nails */
    if (numarcs) {
	XFillArcs(dpy, draw_pm, drawgc, arcs, numarcs);
    }
    if (numselarcs) {
	XFillArcs(dpy, draw_pm, erasegc, selarcs, numselarcs);
	XFillArcs(dpy, draw_pm, selectgc, selarcs, numselarcs);
    }
    if (numcircs) {
	XFillArcs(dpy, draw_pm, erasegc, circs, numcircs);
	XDrawArcs(dpy, draw_pm, drawgc, circs, numcircs);
    }
    if (numselcircs) {
	XFillArcs(dpy, draw_pm, erasegc, selcircs, numselcircs);
	XDrawArcs(dpy, draw_pm, selectgc, selcircs, numselcircs);
    }

    if (mst.center_id != -1) {
	i = mst.center_id;

	rad = ((masses[i].status & S_FIXED) && !(masses[i].status & S_TEMPFIXED)) ? NAIL_SIZE : masses[i].radius;

	XDrawRectangle(dpy, draw_pm, drawgc, (int)(COORD_X(masses[i].x) - rad), (int)(COORD_Y(masses[i].y) - rad),
		       2 * rad + 1, 2 * rad + 1);
    }
}

void view_system()
{
    XCopyPlane(dpy, draw_pm, draw_win, fggc, bb_ulx, bb_uly, bb_lrx - bb_ulx + 1, bb_lry - bb_uly + 1, bb_ulx, bb_uly, 0x1);
}

void view_subsystem(ulx, uly, wid, ht)
int ulx, uly, wid, ht;
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

    XCopyPlane( dpy, draw_pm, draw_win, fggc, ulx, uly, wid, ht, ulx, uly, 0x1 );
}

void review_system(reset)
boolean reset;
{
    if (reset) {
	reset_bbox();
    } else {
	calc_bbox();
    }

    /* Clear the old pixmap */
    XFillRectangle(dpy, draw_pm, erasegc, bb_ulx, bb_uly, bb_lrx - bb_ulx + 1, bb_lry - bb_uly + 1);

    redraw_system();

    view_system();

    mouse_event(M_REDISPLAY, 0, 0, AnyButton, FALSE);
    if (mst.adaptive_step) {
	update_slider(slid_dt_num);
    }
    XFlush(dpy);
}

void update_slidnum(w)
int w;
{
    change_slider_parms(S_GRAV, &(mst.cur_grav_val[w]), cur_grav_max[w], cur_grav_min[w]);
    change_slider_parms(S_GDIR, &(mst.cur_misc_val[w]), cur_misc_max[w], cur_misc_min[w]);

    update_slider(S_GRAV);
    update_slider(S_GDIR);
}

void redraw_names(w)
int w;
{
    static char *grav_nam[] = { "Gravity", "Magnitude", "Magnitude", "Magnitude" };
    static char *misc_nam[] = { "Direction", "Damping", "Exponent", "Exponent" };
    int offset = 120;

    XDrawLine(dpy, acts_win, bggc, 4, force_liney-1, (BF_NUM)*(BF_SIZ+4)+5, force_liney-1);
    XDrawLine(dpy, acts_win, bggc, 4, force_liney+BF_SIZ+6, (BF_NUM)*(BF_SIZ+4)+5, force_liney+BF_SIZ+6);
    XFillRectangle(dpy, acts_win, bggc, grav_slidx + offset, grav_slidy - F_HT, B_WID - 1 - grav_slidx - offset,
		   dir_slidy + F_HT + 3 - grav_slidy);

    XDrawLine(dpy, acts_win, fggc, w*(BF_SIZ+4)+4, force_liney-1, (w+1)*(BF_SIZ+4)+5, force_liney-1);
    XDrawLine(dpy, acts_win, fggc, w*(BF_SIZ+4)+4, force_liney+BF_SIZ+6, (w+1)*(BF_SIZ+4)+5, force_liney+BF_SIZ+6);
    XDrawString(dpy, acts_win, fggc, grav_slidx + offset, grav_slidy, grav_nam[w], strlen(grav_nam[w]));
    XDrawString(dpy, acts_win, fggc, dir_slidx + offset, dir_slidy, misc_nam[w], strlen(misc_nam[w]));
}

void redisplay_widgets()
{
    XCopyPlane(dpy, acts_pm, acts_win, fggc, 0, 0, B_WID, B_HT, 0, 0, 0x1);

    redraw_names(cur_force);

    redraw_widgets(TRUE);
}

int mass_radius(m)
double m;
{
    int rad;

    rad = (int)(2 * log(4.0 * m + 1.0));

    if (rad < 1) rad = 1;
    if (rad > 64) rad = 64;

    return rad + spthick/2;
}

static void draw_mass(w, mx, my, m)
Window w;
int mx, my;
double m;
{
    int rad;

    rad = mass_radius(m);

    XFillArc(dpy, w, fggc, mx - rad, my - rad, rad * 2 + 1, rad * 2 + 1, 0, 64 * 360);
}

static void draw_spring(w, x1, y1, x2, y2)
Window w;
int x1, y1, x2, y2;
{
    XDrawLine(dpy, w, fggc, x1, y1, x2, y2);
}

static void mouse_vel(mx, my)
int *mx, *my;
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

void widget_notify(type, idno)
int type, idno;
{
    int i;

    switch (type) {

      case O_BUTTON:
	switch (idno) {
	  case B_CENTER:
	    set_center();
	    review_system(TRUE);
	    break;
	  case B_RESTLEN:
	    set_sel_restlen();
	    break;
	  case B_DELETE:
	    delete_selected();
	    review_system(TRUE);
	    break;
	  case B_SELECTALL:
	    select_all();
	    eval_selection();
	    review_system(TRUE);
	    break;
	  case B_SAVESTATE:
	    save_state();
	    sst = mst;
	    break;
	  case B_RESSTATE:
	    restore_state();
	    mst = sst;

	    redisplay_widgets();
	    review_system(TRUE);
	    mouse_event(M_REDISPLAY, 0, 0, AnyButton, FALSE);
	    break;
	  case B_DUPLICATE:
	    duplicate_selected();
	    review_system(TRUE);
	    break;
	  case B_INSERTFILE:
	    file_op = F_INSERT;
	    disp_filename(TRUE);
	    break;
	  case B_LOADFILE:
	    file_op = F_LOAD;
	    disp_filename(TRUE);
	    break;
	  case B_SAVEFILE:
	    file_op = F_SAVE;
	    disp_filename(TRUE);
	    break;
	  case B_RESET:
	    delete_all();
	    mst = initst;
	    init_objects();

	    redisplay_widgets();
	    review_system(TRUE);
	    mouse_event(M_REDISPLAY, 0, 0, AnyButton, FALSE);
	    break;
	  case B_QUIT:
	    quitting = TRUE;
	    break;
	}
	break;

      case O_MBUTTON:
	if (idno >= M_BF && idno < M_BF + BF_NUM && mst.bf_mode[idno - M_BF] > 0) {
	    cur_force = idno - M_BF;
	    update_slidnum(cur_force);
	    redraw_names(cur_force);
	}
	break;

      case O_CHECKBOX:
	switch (idno) {
	  case C_FIXEDMASS:
	    for (i = 0; i < num_mass; i++) {
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
	  case C_SHOWSPRING:
	    review_system(TRUE);
	    break;
	}
	break;

      case O_SLIDER:
	switch (idno) {
	  case S_MASS:
	    for (i = 0; i < num_mass; i++) {
		if (masses[i].status & S_SELECTED) {
		    masses[i].mass = mst.cur_mass;
		    masses[i].radius = mass_radius(mst.cur_mass);
		}
	    }
	    review_system(TRUE);
	    break;
	  case S_ELAS:
	    for (i = 0; i < num_mass; i++) {
		if (masses[i].status & S_SELECTED)
		  masses[i].elastic = mst.cur_rest;
	    }
	    break;
	  case S_KSPR:
	    for (i = 0; i < num_spring; i++) {
		if (springs[i].status & S_SELECTED)
		  springs[i].ks = mst.cur_ks;
	    }
	    break;
	  case S_KDAMP:
	    for (i = 0; i < num_spring; i++) {
		if (springs[i].status & S_SELECTED)
		  springs[i].kd = mst.cur_kd;
	    }
	    break;
	}
    }
}

static void mouse_event(type, mx, my, mbutton, shifted)
int type;
int mx, my, mbutton;
int shifted;
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

    switch (type) {
      case M_REDISPLAY:
	switch (mode) {
	  case M_EDIT:
	    switch (cur_button) {
	      case Button1:
					if (selection < 0) {
		    		XDrawRectangle(dpy, draw_win, selectboxgc, MIN(startx, prevx), MIN(starty, prevy),
				   		ABS(prevx - startx), ABS(prevy - starty));
					}
					break;
	      case Button2:
					break;
	      case Button3:
					break;
	    }
	    break;
	  case M_MASS:
	    draw_mass(draw_win, prevx, prevy, mst.cur_mass);
	    break;
	  case M_SPRING:
	    if (static_spring) {
		startx = COORD_X(masses[selection].x);
		starty = COORD_Y(masses[selection].y);
		draw_spring(draw_win, startx, starty, prevx, prevy);
	    }
	    break;
	}
	break;

      case M_DOWN:
	review_system(TRUE);
	startx = prevx = mx;
	starty = prevy = my;
	cur_button = mbutton;
	active = TRUE;
	cur_shift = shifted;

	switch (mode) {
	  case M_EDIT:
	    switch (cur_button) {
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
	    draw_mass(draw_win, mx, my, mst.cur_mass);
	    break;
	  case M_SPRING:
	    {
		boolean is_mass = TRUE;

		static_spring = (action == -1 || cur_button == Button3);

		selection = nearest_object(COORD_X(mx), COORD_Y(my), &is_mass);
		if (selection >= 0 && is_mass) {
		    startx = COORD_X(masses[selection].x);
		    starty = COORD_Y(masses[selection].y);
		    if (static_spring) {
			draw_spring(draw_win, startx, starty, prevx, prevy);
		    } else {
			attach_fake_spring(selection);
			move_fake_mass(COORD_X(mx), COORD_Y(my));
			review_system(TRUE);
		    }
		} else {
		    active = FALSE;
		}
	    }
	    break;
	}
	break;

      case M_DRAG:
	switch (mode) {
	  case M_EDIT:
	    switch (cur_button) {
	      case Button1:
					if (selection < 0) {
				    view_subsystem(MIN(startx, prevx), MIN(starty, prevy), ABS(prevx - startx) + 1, ABS(prevy - starty) + 1);
				    prevx = mx;
				    prevy = my;
				    XDrawRectangle(dpy, draw_win, selectboxgc, MIN(startx, prevx), MIN(starty, prevy),   ABS(prevx - startx), ABS(prevy - starty));
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
	    draw_mass(draw_win, prevx, prevy, mst.cur_mass);
	    break;
	  case M_SPRING:
	    if (static_spring) {
		view_subsystem(MIN(startx, prevx), MIN(starty, prevy), ABS(prevx - startx) + 1, ABS(prevy - starty) + 1);
		prevx = mx;
		prevy = my;
		startx = COORD_X(masses[selection].x);
		starty = COORD_Y(masses[selection].y);
		draw_spring(draw_win, startx, starty, prevx, prevy);
	    } else {
		move_fake_mass(COORD_X(mx), COORD_Y(my));
	    }
	    break;
	}
	break;

      case M_UP:
	active = FALSE;
	switch (mode) {
	  case M_EDIT:
	    switch (cur_button) {
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
    XFlush(dpy);
}

static boolean x_event()
{
    XEvent event, peek;
    KeySym ks;
    char keybuf[1];

    if (quitting)
      return FALSE;

    XFlush(dpy);

    /* Get next event */
    if (XPending(dpy)) {
	XNextEvent(dpy, &event);

	switch (event.type) {
	  case ButtonPress:
	    if (!check_widgets(event.xbutton.window, event.xbutton.x, event.xbutton.y, event.xbutton.button, M_DOWN)) {
		/* Process button press event */
		if (event.xbutton.window == draw_win) {
		    bzero(mprev, sizeof(mprev));
		    mouse_event(M_DOWN, event.xbutton.x, event.xbutton.y, event.xbutton.button, event.xbutton.state & ShiftMask);
		} else if (event.xbutton.window == acts_win) {
		    /* Show startup picture if clicked on xspringies logo */
		    if (event.xbutton.x >= 4 && event.xbutton.x < 4 + title_width && event.xbutton.y >= 4 && event.xbutton.y < 4 + title_height) {
			draw_startup_picture();
			view_system();
		    }
		}
	    }
	    break;
	  case ButtonRelease:
	    if (!check_widgets(event.xbutton.window, event.xbutton.x, event.xbutton.y, event.xbutton.button, M_UP)) {
		/* Process button release event */
		if (event.xbutton.window == draw_win) {
		    mouse_event(M_UP, event.xbutton.x, event.xbutton.y, event.xbutton.button, FALSE);
		}
	    }
	    break;
	  case MotionNotify:
	    {
		Window mw = event.xmotion.window;

		/* Skip over other motion notify events for this window */
		while (XCheckWindowEvent(dpy, mw, PointerMotionMask, &event));

		if (event.xmotion.state & (Button1Mask | Button2Mask | Button3Mask)) {
		    if (!check_widgets(event.xmotion.window, event.xmotion.x, event.xmotion.y, 0, M_DRAG)) {
			/* Record mouse info */
			mprev[moffset].x = event.xmotion.x;
			mprev[moffset].y = event.xmotion.y;
			mprev[moffset].t = (unsigned long)(event.xmotion.time);
			moffset = (moffset + 1) % MOUSE_PREV;

			/* Process motion notify event */
			if (event.xbutton.window == draw_win) {
			    mouse_event(M_DRAG, event.xbutton.x, event.xbutton.y, AnyButton, FALSE);
			}
		    }
		}
	    }
	    break;

	  case KeyPress:
	    keybuf[0] = '\0';
	    XLookupString (&event.xkey, keybuf, sizeof(keybuf), &ks, NULL);

	    /* Skip modifier key */
	    if (IsModifierKey(ks) || ks == XK_Multi_key)
	      break;

	    key_press((int)(keybuf[0]), ks, event.xkey.window);
	    break;

	  case ConfigureNotify:
	    {
		static boolean created = FALSE;
		int new_wid, new_ht;

		new_wid = event.xconfigure.width;
		new_ht = event.xconfigure.height;

		if (new_wid == main_wid && new_ht == main_ht)
		  break;

		main_wid = new_wid;
		main_ht = new_ht;

		draw_wid = main_wid - B_WID - 1;
		draw_ht = main_ht - T_HT - 1;

		reset_bbox();

		if (created) {
		    XFreePixmap(dpy, draw_pm);
		    created = TRUE;
		}
		draw_pm = XCreatePixmap(dpy, draw_win, draw_wid, draw_ht, 1);
		review_system(TRUE);

		XMoveResizeWindow(dpy, text_win, B_WID + 1, draw_ht + 1, draw_wid, T_HT);
		XMoveResizeWindow(dpy, draw_win, B_WID + 1, 0, draw_wid, draw_ht);
	    }
	    break;

	  case Expose:
	    /* Skip over additional expose events */
	    while (XCheckTypedEvent(dpy, Expose, &peek))
	      event = peek;

	    /* Generic expose event (redraw everything) */
	    redisplay_widgets();

	    view_system();
	    mouse_event(M_REDISPLAY, 0, 0, AnyButton, FALSE);

	    disp_filename(FALSE);
	    break;

	  default:
	    break;
	}
	XFlush(dpy);
    } else {
	/* No events waiting */
	if (scan_flag) {
	    static struct timeval tp, tpo;
	    static struct timezone tzp;
	    int difft;

	    if (tpo.tv_sec == 0)
	      gettimeofday(&tpo, &tzp);

	    gettimeofday(&tp, &tzp);
	    difft = (tp.tv_sec - tpo.tv_sec) * 1000000 + (tp.tv_usec - tpo.tv_usec);

	    /* Do widget scanning buttons about 30 times / sec */
	    if (difft > 30000) {
		tpo = tp;
		(void)check_widgets(acts_win, 0, 0, 0, M_HOLD);
	    }
	} else if (action == -1) {
	    /* Sleep until next X event */
	    fd_set readfds;

	    FD_ZERO(&readfds);
	    FD_SET(xfd, &readfds);

#ifndef VMS
	    (void)select(xfd+1, &readfds, NULL, NULL, NULL);
#else
            seconds = DELAY_VMS;   /* small wait */
            statvms = LIB$WAIT(&seconds);
#endif VMS
	}

	if (action != -1 && animate_obj()) {
	    static struct timeval tp, tpo;
	    static struct timezone tzp;
	    static int totaldiff = 0;
	    static boolean started = FALSE;
	    int difft;

	    /* Get time the first time through */
	    if (!started) {
		gettimeofday(&tp, &tzp);
		started = TRUE;
	    }
	    tpo = tp;

	    gettimeofday(&tp, &tzp);

	    difft = (tp.tv_sec - tpo.tv_sec) * 1000000 + (tp.tv_usec - tpo.tv_usec);

	    if (difft < MIN_DIFFT)
	      difft = MIN_DIFFT;

	    if (difft > 0) {
		totaldiff += difft;

		/* Do updates over 30 frames per second (to make it smooth) */
		if (totaldiff > 20000) {
		    totaldiff = 0;
		    review_system(FALSE);
		    XSync(dpy, False);
		}
	    }
	}
    }

    return TRUE;
}

Pixmap get_pixmap(bits, width, height, inv)
char *bits;
int width, height;
boolean inv;
{
    int black, white;

    black = inv ? 1 : 0;
    white = inv ? 0 : 1;

    return XCreatePixmapFromBitmapData(dpy, main_win, bits, width, height, white, black, 1);
}

unsigned long GetColor(cname, cmap)
char *cname;
Colormap cmap;
{
    /* Color stuff */
    XColor xc;

    if (XParseColor(dpy, cmap, cname, &xc) == 0) {
	fprintf(stderr, "Unknown color: %s\n", cname);
	exit(-1);
    } else {
	if (XAllocColor(dpy, cmap, &xc) == 0) {
	    fprintf(stderr, "Cannot allocate color: %s\n", cname);
	    exit(-1);
	}
    }

    return xc.pixel;
}

static void init_x(argc, argv, displayname, geometry, fgcolor, bgcolor, hlcolor)
int argc;
char *argv[];
char *displayname, *geometry, *fgcolor, *bgcolor, *hlcolor;
{
    XGCValues gcv, gcmv;
    XSetWindowAttributes xswa;
    XSizeHints hints;
    XFontStruct *font;
    Cursor cross;
    Colormap cmap;
    Pixmap icon_p;
    Visual *vis;
    int scn;
    int mask = 0;

    /* Open display */
    if ((dpy = XOpenDisplay(displayname)) == NULL) {
	fprintf(stderr, "Could not open display\n");
	exit(-1);
    }

    /* Get screen and colors */
    scn = DefaultScreen(dpy);
    planes = DisplayPlanes(dpy, scn);
    cmap = DefaultColormap(dpy, scn);
    vis = DefaultVisual(dpy, scn);
    xfd = ConnectionNumber(dpy);

    if (hlcolor == NULL) {
	if (vis->class == StaticGray || vis->class == GrayScale) {
	    hlcolor = fgcolor;
	} else {
	    XColor xc;

	    if (XParseColor(dpy, cmap, HLCOLOR, &xc) == 0) {
		hlcolor = fgcolor;
	    } else {
		hlcolor = HLCOLOR;
	    }
	}
    }

    fcolor = GetColor(fgcolor, cmap);
    bcolor = GetColor(bgcolor, cmap);
    hcolor = GetColor(hlcolor, cmap);

    /* Set up main window */
    main_wid = hints.width = M_WID + 2;
    hints.min_width = B_WID * 2;
    main_ht = hints.height = hints.min_height = M_HT + 2;
    hints.flags = PSize | PMinSize;

    /* Get geometry info */
    if (geometry != NULL) {
	hints.x = hints.y = 0;

	if (mask = XParseGeometry(geometry, &(hints.x), &(hints.y), &(hints.width), &(hints.height))) {
	    hints.flags |= ((mask & (XValue | YValue)) ? USPosition: PPosition) |
	      ((mask & (WidthValue | HeightValue)) ? USSize : PSize);

	    if (hints.width < hints.min_width)
	      hints.width = hints.min_width;
	    if (hints.height < hints.min_height)
	      hints.height = hints.min_height;
	}
    }

    /* Create main window */
    main_win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), hints.x, hints.y, hints.width, hints.height, 1, fcolor, bcolor);

    /* Fix for open-look wm (thanks to Brian.Warkentine@Eng.Sun.COM) */
    {
        XWMHints wmhints;
        XWindowAttributes xwa;

	wmhints.flags = InputHint;
	wmhints.input = True;
	(void) XGetWindowAttributes(dpy, main_win, &xwa);
	XSelectInput(dpy, main_win, xwa.all_event_masks | KeyPressMask);
	XSetWMHints(dpy, main_win, &wmhints);
    }

    icon_p = XCreatePixmapFromBitmapData(dpy, DefaultRootWindow(dpy), icon_bits, icon_width, icon_height, 1, 0, planes);
    XSetStandardProperties(dpy, main_win, "XSpringies", "XSpringies", icon_p, argv, argc, &hints);

    if (mask)
      XSetNormalHints(dpy, main_win, &hints);

    /* Map main window to display */
    XMapWindow(dpy, main_win);

    XFlush(dpy);
    XSync(dpy, False);

    {
	Window tmp_win;
	int xpos, ypos;
	unsigned int bw, dep;

	XGetGeometry(dpy, main_win, &tmp_win, &xpos, &ypos, &main_wid, &main_ht, &bw, &dep);
    }

    /* Make subwindows */
    acts_win = XCreateSimpleWindow(dpy, main_win, 0, 0, B_WID, B_HT, 1, fcolor, bcolor);

    draw_wid = main_wid - B_WID - 1;
    draw_ht = main_ht - T_HT - 1;
    draw_win = XCreateSimpleWindow(dpy, main_win, B_WID + 1, 0, draw_wid, draw_ht, 1, fcolor, bcolor);
    text_win = XCreateSimpleWindow(dpy, main_win, B_WID + 1, draw_ht + 1, draw_wid, T_HT, 1, fcolor, bcolor);

    /* Make pixmaps for drawing and actions areas (to make redraw trivial) */
    draw_pm = XCreatePixmap(dpy, draw_win, draw_wid, draw_ht, 1);

    acts_pm = XCreatePixmap(dpy, acts_win, B_WID, B_HT, 1);

    /* Get a font */
    if ((font = XLoadQueryFont(dpy, F_NAME)) == NULL) {
	fprintf(stderr, "Could not load font: %s\n", F_NAME);
	exit(-1);
    }

    /* Create GCs */
    gcmv.font = gcv.font = font->fid;
    gcmv.plane_mask = gcv.plane_mask = 1;
    gcmv.line_width = spthick;

    gcmv.foreground = 1;
    gcmv.background = 0;

    gcv.foreground = bcolor;
    gcv.background = hcolor;
    revgc = XCreateGC(dpy, main_win, GCForeground | GCBackground | GCFont, &gcv);

    gcv.foreground = hcolor;
    gcv.background = bcolor;
    hlgc = XCreateGC(dpy, main_win, GCForeground | GCBackground | GCFont, &gcv);

    gcv.foreground = fcolor;
    drawgc = XCreateGC(dpy, draw_pm, GCForeground | GCBackground | GCFont, &gcmv);
    sdrawgc = XCreateGC(dpy, draw_pm, GCForeground | GCBackground | GCFont | GCLineWidth, &gcmv);
    fggc = XCreateGC(dpy, main_win, GCForeground | GCBackground | GCFont, &gcv);

    {
	static char stipple_bits[] = { 0x01, 0x02 };
	static char dot_line[2] = { 1, 1 };
	Pixmap stip;

	stip = XCreateBitmapFromData(dpy, main_win, stipple_bits, 2, 2);
	gcmv.stipple = gcv.stipple = stip;
	gcmv.fill_style = gcv.fill_style = FillStippled;

	gcmv.line_style = gcv.line_style = LineOnOffDash;
	selectgc = XCreateGC(dpy, draw_pm, GCForeground | GCBackground | GCFont | GCFillStyle | GCStipple, &gcmv);
 	selectlinegc = XCreateGC(dpy, draw_pm, GCForeground | GCBackground | GCFont | GCLineStyle | GCLineWidth, &gcmv);

	gcv.foreground = hcolor;

	if (hcolor != fcolor && hcolor != bcolor) {
	    selectboxgc = XCreateGC(dpy, main_win, GCForeground | GCBackground | GCFont, &gcv);
	} else {
	    selectboxgc = XCreateGC(dpy, main_win, GCForeground | GCBackground | GCFont | GCLineStyle, &gcv);
	    XSetDashes(dpy, selectboxgc, 0, dot_line, 2);
	}

	XSetDashes(dpy, selectlinegc, 0, dot_line, 2);
    }

    gcv.foreground = gcv.background = bcolor;
    gcmv.foreground = gcmv.background = 0;
    erasegc = XCreateGC(dpy, draw_pm, GCForeground | GCBackground | GCFont, &gcmv);
    bggc = XCreateGC(dpy, main_win, GCForeground | GCBackground | GCFont, &gcv);

    /* Clear out action pixmap */
    XFillRectangle(dpy, acts_pm, erasegc, 0, 0, B_WID, B_HT);

    startup_pm = get_pixmap(startup_bits, startup_width, startup_height, FALSE);
    draw_startup_picture();

    /* Use a cross cursor for the draw window */
    cross = XCreateFontCursor(dpy, XC_tcross);

    /* Set window event masks and other attributes */
    xswa.event_mask = ExposureMask | StructureNotifyMask;
    XChangeWindowAttributes(dpy, main_win, CWEventMask, &xswa);

    xswa.event_mask = KeyPressMask | ExposureMask;
    XChangeWindowAttributes(dpy, text_win, CWEventMask, &xswa);

    xswa.event_mask = ButtonPressMask | ButtonReleaseMask | Button1MotionMask | Button2MotionMask | Button3MotionMask | KeyPressMask | ExposureMask;
    XChangeWindowAttributes(dpy, acts_win, CWEventMask, &xswa);
    xswa.cursor = cross;
    XChangeWindowAttributes(dpy, draw_win, CWEventMask | CWCursor, &xswa);

    XMapWindow(dpy, draw_win);
    XMapWindow(dpy, text_win);
    XMapWindow(dpy, acts_win);

    XFlush(dpy);
}

static void make_widgets()
{
    int i, y;

    {
	Pixmap title_pm = get_pixmap(title_bits, title_width, title_height, FALSE);
	XCopyArea(dpy, title_pm, acts_pm, drawgc, 0, 0, title_width, title_height, 4, 4);
	XFreePixmap(dpy, title_pm);
    }

    add_modebutton(acts_pm, acts_win, B_WID/2, 4, B_WID-6, 54, "Edit", edit_bits, edit_width, edit_height, M_EDIT, &mode, FALSE);
    add_modebutton(acts_pm, acts_win, 4, 54, B_WID/2, 104, "Mass", mass_bits, mass_width, mass_height, M_MASS, &mode, FALSE);
    add_modebutton(acts_pm, acts_win, B_WID/2, 54, B_WID-6, 104, "Spring", spring_bits, spring_width, spring_height, M_SPRING, &mode, FALSE);

    add_slider(acts_pm, acts_win, 4, 110, B_WID-6, 125, "Mass", "%10.2lf", S_MASS, &(mst.cur_mass), 10000000.0, 0.01, 0.01);
    add_slider(acts_pm, acts_win, 4, 127, B_WID-6, 142, "Elasticity", "%10.3lf", S_ELAS, &(mst.cur_rest), 1.0, 0.0, 0.001);
    add_slider(acts_pm, acts_win, 4, 144, B_WID-6, 159, "Kspring", "%10.2lf", S_KSPR, &(mst.cur_ks), 10000000.0, 0.01, 0.01);
    add_slider(acts_pm, acts_win, 4, 161, B_WID-6, 176, "Kdamp", "%10.2lf", S_KDAMP, &(mst.cur_kd), 10000000.0, 0.0, 0.01);

    add_checkbox(acts_pm, acts_win, 4, 178, B_WID/2-16, 194, "Fixed Mass", C_FIXEDMASS, &(mst.fix_mass));
    add_checkbox(acts_pm, acts_win, B_WID/2, 178, B_WID-6, 194, "Show Springs", C_SHOWSPRING, &(mst.show_spring));

    add_button(acts_pm, acts_win, 4, 197, B_WID/2 + 10, 217, "Set Rest Length", B_RESTLEN);
    add_button(acts_pm, acts_win, B_WID/2 + 16, 197, B_WID-6, 217, "Set Center", B_CENTER);

    y = 228;
    XDrawLine(dpy, acts_pm, drawgc, 0, y-4, B_WID-1, y-4);
    y++;

    for (i = 0; i < BF_NUM; i++) {
	mst.bf_mode[i] = -1;
	add_modebutton(acts_pm, acts_win, i*(BF_SIZ+4)+4, y, 4+(i+1)*(BF_SIZ+4),y+BF_SIZ+4,"", b_bits[i], BF_SIZ, BF_SIZ, M_BF + i, &(mst.bf_mode[i]), TRUE);
    }
    force_liney = y;

    add_modebutton(acts_pm, acts_win, 132, y, 136+go_width, y+4+go_height, "", go_bits, go_width, go_height, M_ACTION, &action, TRUE);

    y += 36;
    add_slider(acts_pm, acts_win, 4, y, B_WID-6, y+15, "", "%10.2lf", S_GRAV, &(mst.cur_grav_val[0]), cur_grav_max[0], cur_grav_min[0], 0.01);
    grav_slidx = 4;
    grav_slidy = (y + y+15 + F_HT) / 2 - 2;

    add_slider(acts_pm, acts_win, 4, y+17, B_WID-6, y+32, "", "%10.2lf", S_GDIR, &(mst.cur_grav_val[0]), cur_misc_max[0], cur_misc_min[0], 0.05);

    dir_slidx = 4;
    dir_slidy = (y+17 + y+32 + F_HT) / 2 - 2;
    update_slidnum(0);

    y += 38;
    XDrawLine(dpy, acts_pm, drawgc, 0, y - 4, B_WID-1, y - 4);

    add_slider(acts_pm, acts_win, 4, y, B_WID-6, y+15, "Viscosity", "%10.2lf", S_VISC, &(mst.cur_visc), 10000000.0, 0.0, 0.01);
    add_slider(acts_pm, acts_win, 4, y+17, B_WID-6, y+32, "Stickiness", "%10.2lf", S_STICK, &(mst.cur_stick), 10000000.0, 0.0, 0.01);

    y = y + 37;

    XDrawLine(dpy, acts_pm, drawgc, 0, y - 4, B_WID-1, y - 4);

    add_slider(acts_pm, acts_win, 4, y, B_WID-6, y+15, "Time Step", "%10.4lf", S_TIMESTEP, &(mst.cur_dt), 1.0, 0.0001, 0.0001);
    add_slider(acts_pm, acts_win, 4, y+17, B_WID-6, y+32, "Precision", "%10.4lf", S_PRECISION, &(mst.cur_prec), 1000.0, 0.0001, 0.0001);
    add_checkbox(acts_pm, acts_win, 4, y+33, B_WID/2+32, y+49, "Adaptive Time Step", C_ADAPTIVE_STEP, &(mst.adaptive_step));

    y = y + 54;
    XDrawLine(dpy, acts_pm, drawgc, 0, y-4, B_WID-1, y-4);

    add_slider(acts_pm, acts_win, 4, y, B_WID/2 + 20, y+16, "", "%10.0lf", S_GRIDSNAP, &(mst.cur_gsnap), 200.0, 1.0, 1.0);
    add_checkbox(acts_pm, acts_win, B_WID/2 + 24, y, B_WID - 4, y+16, "Grid Snap", C_GRIDSNAP, &(mst.grid_snap));

    y = y + 2;
    XDrawRectangle(dpy, acts_pm, drawgc, 16, y+24, 64, 30);
    add_checkbox(acts_pm, acts_win, 40, y+18, 56, y+33, "", C_WTOP, &(mst.w_top));
    add_checkbox(acts_pm, acts_win, 8, y+33, 24, y+48, " Walls", C_WLEFT, &(mst.w_left));
    add_checkbox(acts_pm, acts_win, 72, y+33, 88, y+48, "", C_WRIGHT, &(mst.w_right));
    add_checkbox(acts_pm, acts_win, 40, y+48, 56, y+63, "", C_WBOTTOM, &(mst.w_bottom));

    y = y + 16;
    add_button(acts_pm, acts_win, B_WID/2+3, y, B_WID-6, y+20, "Delete", B_DELETE);
    add_button(acts_pm, acts_win, B_WID/2+3, y+25, B_WID-6, y+45, "Select all", B_SELECTALL);

    y = y + 52;
    XDrawLine(dpy, acts_pm, drawgc, 0, y-3, B_WID-1, y-3);

    add_button(acts_pm, acts_win, 4, y, B_WID/2-5, y+20, "Save State", B_SAVESTATE);
    add_button(acts_pm, acts_win, B_WID/2+3, y, B_WID-6, y+20, "Restore State", B_RESSTATE);
    add_button(acts_pm, acts_win, 4, y+25, B_WID/2-5, y+45, "Duplicate", B_DUPLICATE);
    add_button(acts_pm, acts_win, B_WID/2+3, y+25, B_WID-6, y+45, "Insert File", B_INSERTFILE);

    add_button(acts_pm, acts_win, 4, y+50, B_WID/2-5, y+70, "Load File", B_LOADFILE);
    add_button(acts_pm, acts_win, B_WID/2+3, y+50, B_WID-6, y+70, "Save File", B_SAVEFILE);
    add_button(acts_pm, acts_win, 4, y+75, B_WID/2-5, y+95, "Reset", B_RESET);
    add_button(acts_pm, acts_win, B_WID/2+3, y+75, B_WID-6, y+95, "Quit", B_QUIT);

    slid_dt_num = slider_valno(S_TIMESTEP);
}

void usage()
{
    static char *msg1 = "Usage: xspringies [-d|display displayname] [-geometry geom] [-rv]\n";
    static char *msg2 = "                  [-bg color] [-fg color] [-hl color] [-st int] [-nbb]\n";

    fprintf(stderr, msg1);
    fprintf(stderr, msg2);
    exit(-1);
}

main(argc, argv)
int argc;
char *argv[];
{
    char *swch, *displayname = NULL, *geometry = NULL, *bgcolor = "black", *fgcolor = "white", *hlcolor = NULL;
    boolean rev_vid = FALSE;
    extern char *getenv();
    char *path;

    initst = sst = mst;

    if ((path = getenv("SPRINGDIR")) != NULL) {
	strcpy(filename, path);
    } else {
	strcpy(filename, DEF_PATH);
    }

    while (--argc > 0) {
	if (**++argv == '-') {
	    swch = (*argv) + 1;

	    if (!strcmp(swch, "display") || !strcmp(swch, "d")) {
		if (--argc > 0) {
		    displayname = *++argv;
		} else {
		    usage();
		}
	    } else if (!strcmp(swch, "geometry")) {
		if (--argc > 0) {
		    geometry = *++argv;
		} else {
		    usage();
		}
	    } else if (!strcmp(swch, "rv")) {
		rev_vid = TRUE;
	    } else if (!strcmp(swch, "bg")) {
		if (--argc > 0) {
		    bgcolor = *++argv;
		} else {
		    usage();
		}
	    } else if (!strcmp(swch, "fg")) {
		if (--argc > 0) {
		    fgcolor = *++argv;
		} else {
		    usage();
		}
	    } else if (!strcmp(swch, "hl")) {
		if (--argc > 0) {
		    hlcolor = *++argv;
		} else {
		    usage();
		}
	    } else if (!strcmp(swch, "st")) {
		if (--argc > 0) {
		    spthick = atoi(*++argv);
		    if (spthick < 0) {
			fprintf(stderr, "String thickness value must be non-negative\n");
			exit(-1);
		    }
		} else {
		    usage();
		}
	    } else if (!strcmp(swch, "nbb")) {
		bboxing = FALSE;
	    } else {
		usage();
	    }
	} else {
	    usage();
	}
    }

    if (rev_vid) {
	char *swap = fgcolor;

	fgcolor = bgcolor;
	bgcolor = swap;
    }

    init_x(argc, argv, displayname, geometry, fgcolor, bgcolor, hlcolor);

    init_widgets(widget_notify);
    make_widgets();

    init_objects();

    while (x_event());

    clean_up();
    return 0;
}


#endif