/* widget.c -- Xlib widget support for xspringies
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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "defs.h"

#define NAME_LEN	32
#define MAX_OBJS	16
#define NUM_DIGS        12

/* Bitmaps used */
static char check_bits[] = {
   0x00, 0x00, 0xfe, 0x7f, 0x06, 0x60, 0x0a, 0x50, 0x12, 0x48, 0x22, 0x44,
   0x42, 0x42, 0x82, 0x41, 0x82, 0x41, 0x42, 0x42, 0x22, 0x44, 0x12, 0x48,
   0x0a, 0x50, 0x06, 0x60, 0xfe, 0x7f, 0x00, 0x00};
static char checked_bits[] = {
   0x00, 0x00, 0xfe, 0x7f, 0xfe, 0x7f, 0x0e, 0x70, 0x16, 0x68, 0x26, 0x64,
   0x46, 0x62, 0x86, 0x61, 0x86, 0x61, 0x46, 0x62, 0x26, 0x64, 0x16, 0x68,
   0x0e, 0x70, 0xfe, 0x7f, 0xfe, 0x7f, 0x00, 0x00};
static char unchecked_bits[] = {
   0x00, 0x00, 0xfe, 0x7f, 0xfe, 0x7f, 0x06, 0x60, 0x06, 0x60, 0x06, 0x60,
   0x06, 0x60, 0x06, 0x60, 0x06, 0x60, 0x06, 0x60, 0x06, 0x60, 0x06, 0x60,
   0x06, 0x60, 0xfe, 0x7f, 0xfe, 0x7f, 0x00, 0x00};
static char box_bits[] = {
   0x00, 0x00, 0xfe, 0x7f, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40,
   0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40,
   0x02, 0x40, 0x02, 0x40, 0xfe, 0x7f, 0x00, 0x00};
static char rarr_bits[] = {
   0xfc, 0x1f, 0xfe, 0x3f, 0x07, 0x70, 0x13, 0x60, 0x73, 0x60, 0xf3, 0x61,
   0xf3, 0x67, 0xf3, 0x6f, 0xf3, 0x67, 0xf3, 0x61, 0x73, 0x60, 0x13, 0x60,
   0x07, 0x70, 0xfe, 0x3f, 0xfc, 0x1f, 0x00, 0x00};
static char riarr_bits[] = {
   0xfc, 0x1f, 0x06, 0x30, 0xfb, 0x6f, 0xed, 0x5f, 0x8d, 0x5f, 0x0d, 0x5e,
   0x0d, 0x58, 0x0d, 0x50, 0x0d, 0x58, 0x0d, 0x5e, 0x8d, 0x5f, 0xed, 0x5f,
   0xfb, 0x6f, 0x06, 0x30, 0xfc, 0x1f, 0x00, 0x00};
static char larr_bits[] = {
   0xfc, 0x1f, 0xfe, 0x3f, 0x07, 0x70, 0x03, 0x64, 0x03, 0x67, 0xc3, 0x67,
   0xf3, 0x67, 0xfb, 0x67, 0xf3, 0x67, 0xc3, 0x67, 0x03, 0x67, 0x03, 0x64,
   0x07, 0x70, 0xfe, 0x3f, 0xfc, 0x1f, 0x00, 0x00};
static char liarr_bits[] = {
   0xfc, 0x1f, 0x06, 0x30, 0xfb, 0x6f, 0xfd, 0x5b, 0xfd, 0x58, 0x3d, 0x58,
   0x0d, 0x58, 0x05, 0x58, 0x0d, 0x58, 0x3d, 0x58, 0xfd, 0x58, 0xfd, 0x5b,
   0xfb, 0x6f, 0x06, 0x30, 0xfc, 0x1f, 0x00, 0x00};

/* Types of objects */
typedef struct {
    Window win;
    int ulx, uly, lrx, lry;
    int idno;
    char name[NAME_LEN];
    boolean state;
} button;

/* Types of objects */
typedef struct {
    Window win;
    int ulx, uly, lrx, lry;
    int idno;
    char name[NAME_LEN];
    int *activeid;
    Pixmap pm;
    int pmwid, pmht;
    boolean state;
    boolean disable;
} modebutton;

typedef struct {
    Window win;
    int ulx, uly, lrx, lry;
    int idno;
    char name[NAME_LEN];
    boolean nowstate;
    boolean *state;
} checkbox;

typedef struct {
    Window win;
    int ulx, uly, lrx, lry;
    int idno;
    char name[NAME_LEN];
    char format[NAME_LEN];
    int state, active;
    double nowvalue;
    double *value;
    double vmin, vmax, vincr;
} slider;

/* Object globals */
static button buttons[MAX_OBJS];
static modebutton mbuttons[MAX_OBJS];
static checkbox cboxes[MAX_OBJS];
static slider sliders[MAX_OBJS];
static char keybuff[NUM_DIGS + 1];
static int numb, numm, nums, numc, cur_type, cur_num, cur_but, key_active;
static boolean key_dirty;
static Pixmap cb_pm, cbc_pm, cbcb_pm, cbub_pm, la_pm, lap_pm, ra_pm, rap_pm;

/* Flag if the arrow buttons on sliders are in scan mode */
boolean scan_flag;

/* X variables from elsewhere */
extern Display *dpy;
extern Window main_win;
extern GC drawgc, erasegc, fggc, bggc, revgc, hlgc;
extern Pixmap acts_pm;
void (*notify_func)();




void init_widgets(notify)
void (*notify)();
{
    extern Pixmap get_pixmap();

    numb = nums = numc = numm = cur_type = cur_num = 0;
    key_active = cur_but = -1;
    scan_flag = FALSE;

    notify_func = notify;

    cb_pm = get_pixmap(box_bits, 16, 16, FALSE);
    cbc_pm = get_pixmap(check_bits, 16, 16, FALSE);
    cbcb_pm = get_pixmap(checked_bits, 16, 16, FALSE);
    cbub_pm = get_pixmap(unchecked_bits, 16, 16, FALSE);
    la_pm = get_pixmap(larr_bits, 16, 16, FALSE);
    lap_pm = get_pixmap(liarr_bits, 16, 16, FALSE);
    ra_pm = get_pixmap(rarr_bits, 16, 16, FALSE);
    rap_pm = get_pixmap(riarr_bits, 16, 16, FALSE);
}

void add_button(d, win, ulx, uly, lrx, lry, name, idno)
Drawable d;
Window win;
int ulx, uly, lrx, lry;
char *name;
int idno;
{
    int len;

    if (numb < MAX_OBJS - 1) {
	buttons[numb].win = win;
	buttons[numb].ulx = ulx;
	buttons[numb].uly = uly;
	buttons[numb].lrx = lrx;
	buttons[numb].lry = lry;
	strncpy(buttons[numb].name, name, NAME_LEN-1);
	buttons[numb].name[NAME_LEN-1] = '\0';
	len = strlen(buttons[numb].name);
	buttons[numb].idno = idno;
	buttons[numb].state = FALSE;

	XDrawRectangle(dpy, d, drawgc, ulx, uly, lrx - ulx + 1, lry - uly + 1);
	XDrawRectangle(dpy, d, drawgc, ulx+1, uly+1, lrx - ulx - 1, lry - uly - 1);

	XDrawString(dpy, d, drawgc, (ulx + lrx - len * F_WID) / 2, (uly + lry + F_HT) / 2,
		    buttons[numb].name, len);

	numb++;
    }
}

void draw_modebutton(d, which)
Drawable d;
int which;
{
    int ulx, uly, lrx, lry, pmwid, pmht;
    int len;
    Pixmap pm;

    ulx = mbuttons[which].ulx;
    uly = mbuttons[which].uly;
    lrx = mbuttons[which].lrx;
    lry = mbuttons[which].lry;
    pmwid = mbuttons[which].pmwid;
    pmht = mbuttons[which].pmht;
    pm = mbuttons[which].pm;

    len = strlen(mbuttons[which].name);

    if (len) {
	XCopyArea(dpy, pm, d, drawgc, 0, 0, pmwid, pmht, ulx + (lrx - ulx - pmwid) / 2, uly + (lry - uly - 4 * F_HT / 3 - 1 - pmht) / 2);
	XDrawString(dpy, d, drawgc, ulx + (lrx - ulx - len * F_WID) / 2, lry - F_HT / 3 - 1, mbuttons[which].name, len);
    } else {
	XCopyArea(dpy, pm, d, drawgc, 0, 0, pmwid, pmht, ulx + (lrx - ulx - pmwid) / 2, uly + (lry - uly - pmht) / 2);
    }
}

void add_modebutton(d, win, ulx, uly, lrx, lry, name, pm_bits, pmwid, pmht, idno, activeid, disable)
Drawable d;
Window win;
int ulx, uly, lrx, lry;
char *name;
char *pm_bits;
int pmwid, pmht;
int idno;
int *activeid;
boolean disable;
{
    if (numm < MAX_OBJS - 1) {
	mbuttons[numm].win = win;
	mbuttons[numm].ulx = ulx;
	mbuttons[numm].uly = uly;
	mbuttons[numm].lrx = lrx;
	mbuttons[numm].lry = lry;
	strncpy(mbuttons[numm].name, name, NAME_LEN-1);
	mbuttons[numm].name[NAME_LEN-1] = '\0';
	mbuttons[numm].idno = idno;
	mbuttons[numm].activeid = activeid;
	mbuttons[numm].state = (mbuttons[numm].idno == *activeid);

	mbuttons[numm].disable = disable;

	XDrawRectangle(dpy, d, drawgc, ulx, uly, lrx - ulx + 1, lry - uly + 1);
	XDrawRectangle(dpy, d, drawgc, ulx+1, uly+1, lrx - ulx - 1, lry - uly - 1);

	mbuttons[numm].pm = get_pixmap(pm_bits, pmwid, pmht, FALSE);
	mbuttons[numm].pmwid = pmwid;
	mbuttons[numm].pmht = pmht;

	draw_modebutton(d, numm);

	numm++;
    }
}

void add_checkbox(d, win, ulx, uly, lrx, lry, name, idno, state)
Drawable d;
Window win;
int ulx, uly, lrx, lry;
char *name;
int idno;
boolean *state;
{
    int len;

    if (numc < MAX_OBJS - 1) {
	cboxes[numc].win = win;
	cboxes[numc].ulx = ulx;
	cboxes[numc].uly = uly;
	cboxes[numc].lrx = lrx;
	cboxes[numc].lry = lry;
	strncpy(cboxes[numc].name, name, NAME_LEN-1);
	cboxes[numc].name[NAME_LEN-1] = '\0';
	len = strlen(cboxes[numc].name);
	cboxes[numc].idno = idno;
	cboxes[numc].state = state;
	cboxes[numc].nowstate = *(cboxes[numc].state);

	XCopyArea(dpy, cb_pm, d, drawgc, 0, 0, 16, 16, ulx, (uly + lry - 16) / 2);

	XDrawString(dpy, d, drawgc, ulx + 20, (uly + lry + F_HT) / 2 - 1, cboxes[numc].name, len);

	numc++;
    }
}

void add_slider(d, win, ulx, uly, lrx, lry, name, format, idno, value, vmax, vmin, vincr)
Drawable d;
Window win;
int ulx, uly, lrx, lry;
char *name, *format;
int idno;
double *value;
double vmin, vmax, vincr;
{
    int len;

    if (numc < MAX_OBJS - 1) {
	sliders[nums].win = win;
	sliders[nums].ulx = ulx;
	sliders[nums].uly = uly;
	sliders[nums].lrx = lrx;
	sliders[nums].lry = lry;
	strncpy(sliders[nums].name, name, NAME_LEN-1);
	sliders[nums].name[NAME_LEN-1] = '\0';
	len = strlen(sliders[nums].name);
	strncpy(sliders[nums].format, format, NAME_LEN-1);
	sliders[nums].format[NAME_LEN-1] = '\0';
	sliders[nums].idno = idno;
	sliders[nums].value = value;
	sliders[nums].nowvalue = *(sliders[nums].value);
	sliders[nums].vmax = vmax;
	sliders[nums].vmin = vmin;
	sliders[nums].vincr = vincr;
	sliders[nums].state = O_NOTHING;
	sliders[nums].active = FALSE;

	XCopyArea(dpy, la_pm, d, drawgc, 0, 0, 16, 16, ulx, (uly + lry - 16) / 2);
	XCopyArea(dpy, ra_pm, d, drawgc, 0, 0, 16, 16, ulx + 16 + F_WID * NUM_DIGS + 12, (uly + lry - 16) / 2);

	XDrawRectangle(dpy, d, drawgc, ulx + 16 + 2, (uly + lry - 16) / 2, F_WID * NUM_DIGS + 6, F_HT + 4);
	XDrawRectangle(dpy, d, drawgc, ulx + 16 + 3, (uly + lry - 16) / 2 + 1, F_WID * NUM_DIGS + 4, F_HT + 2);

	XDrawString(dpy, d, drawgc, ulx + 16 + F_WID * NUM_DIGS + 32, (uly + lry + F_HT) / 2 - 2, sliders[nums].name, len);

	nums++;
    }
}

void activate_mbutton(activeptr, state)
int *activeptr;
boolean state;
{
    int i;

    for (i = 0; i < numm; i++) {
	if (mbuttons[i].activeid == activeptr) {
	    *(mbuttons[i].activeid) = state ? mbuttons[i].idno : -1;
	    break;
	}
    }
}

static void update_mbutton(old_id, new_id)
int old_id, new_id;
{
    int i, ulx, uly, lrx, lry;

    /* Invert old mode button */
    for (i = 0; i < numm; i++) {
	if (mbuttons[i].idno == old_id)
	  break;
    }
    if (mbuttons[i].idno == old_id) {
	ulx = mbuttons[i].ulx;
	uly = mbuttons[i].uly;
	lrx = mbuttons[i].lrx;
	lry = mbuttons[i].lry;

	XCopyPlane(dpy, acts_pm, mbuttons[i].win, fggc, ulx+3, uly+3, lrx - ulx - 4, lry - uly - 4, ulx+3, uly+3, 0x1);
    }

    /* Invert new mode button */
    for (i = 0; i < numm; i++) {
	if (mbuttons[i].idno == new_id)
	  break;
    }
    if (mbuttons[i].idno == new_id) {
	ulx = mbuttons[i].ulx;
	uly = mbuttons[i].uly;
	lrx = mbuttons[i].lrx;
	lry = mbuttons[i].lry;

	XCopyPlane(dpy, acts_pm, mbuttons[i].win, revgc, ulx+3, uly+3, lrx - ulx - 4, lry - uly - 4, ulx+3, uly+3, 0x1);
    }
}

void update_slider_box(cur, ulx, uly, lrx, lry, inverted)
int cur;
boolean inverted;
{
    int len;
    char valuebuf[256];

    if (cur == key_active && key_dirty) {
	strcpy(valuebuf, keybuff);
	len = strlen(valuebuf);
    } else {
	sprintf(valuebuf, sliders[cur].format, *(sliders[cur].value));
	if ((len = strlen(valuebuf)) > NUM_DIGS)
	  len = NUM_DIGS;
    }

    XFillRectangle(dpy, sliders[cur].win, bggc, ulx + 16 + 4, (uly + lry - 16) / 2 + 2, F_WID * NUM_DIGS + 3, F_HT + 1);

    if (inverted) {
	XFillRectangle(dpy, sliders[cur].win, hlgc, ulx + 16 + 5, (uly + lry - 16) / 2 + 3, F_WID * NUM_DIGS + 1, F_HT - 1);
    }
    XDrawString(dpy, sliders[cur].win, inverted ? bggc : fggc, ulx + 16 + 4, (uly + lry + F_HT) / 2 - 2, valuebuf, len);
}

void update_slider(cur)
int cur;
{
    int ulx, uly, lrx, lry;

    ulx = sliders[cur].ulx;
    uly = sliders[cur].uly;
    lrx = sliders[cur].lrx;
    lry = sliders[cur].lry;

    /* Set to proper range */
    if (*(sliders[cur].value) < sliders[cur].vmin) {
	*(sliders[cur].value) = sliders[cur].vmin;
    } else if (*(sliders[cur].value) > sliders[cur].vmax) {
	*(sliders[cur].value) = sliders[cur].vmax;
    }

    update_slider_box(cur, ulx, uly, lrx, lry, cur == key_active);
}

void change_slider_parms(cur, valp, max, min)
int cur;
double *valp, max, min;
{
    sliders[cur].value = valp;
    sliders[cur].vmax = max;
    sliders[cur].vmin = min;
}

int slider_valno(idno)
int idno;
{
    int i;

    /* Draw sliders */
    for (i = 0; i < nums; i++) {
	if (sliders[i].idno == idno)
	  return i;
    }
    return -1;
}

static void update_checkbox(cur, active)
int cur;
boolean active;
{
    int ulx, uly, lry;
    Pixmap which_pm;

    ulx = cboxes[cur].ulx;
    uly = cboxes[cur].uly;
    lry = cboxes[cur].lry;

    if (active) {
	which_pm = cboxes[cur].nowstate ? cbub_pm : cbcb_pm;
    } else {
	which_pm = *(cboxes[cur].state) ? cbc_pm : cb_pm;
    }

    XCopyPlane(dpy, which_pm, cboxes[cur].win, fggc, 0, 0, 16, 16, ulx, (uly + lry - 16) / 2, 0x1);
}

void redraw_widgets(mode)
boolean mode;
{
    int i;

    if (mode){
	/* Draw mode buttons */
	for (i = 0; i < numm; i++) {
	    if (*(mbuttons[i].activeid) == mbuttons[i].idno)
	      update_mbutton(-1, mbuttons[i].idno);
	}
    }

    /* Draw checkboxes */
    for (i = 0; i < numc; i++) {
	update_checkbox(i, FALSE);
    }

    /* Draw sliders */
    for (i = 0; i < nums; i++) {
	update_slider(i);
    }


    /* Redraw active object */
}

boolean key_widgets(key, win)
int key;
Window win;
{
    int len, ka = key_active;

    if (key_active < 0 || sliders[key_active].win != win)
      return FALSE;

    len = strlen(keybuff);

    switch (key) {
      case K_DELETE:
	if (len > 0) {
	    keybuff[len - 1] = '\0';
	}
	break;
      case K_RETURN:
	if (keybuff[0]) {
	    sscanf(keybuff, "%lf", sliders[key_active].value);
	}
      case K_ESCAPE:
	key_active = -1;
	break;
      default:
	if (len < NUM_DIGS - 1) {
	    keybuff[len] = (char)key;
	    keybuff[len+1] = '\0';
	}
	break;
    }

    key_dirty = TRUE;
    update_slider(ka);
    if (key == K_RETURN) {
	notify_func(O_SLIDER, ka);
    }
    return TRUE;
}

boolean check_widgets(win, mx, my, butn, mstat)
Window win;
int mx, my;
int butn, mstat;
{
    int i;
    int ulx, uly, lrx, lry;

    switch (mstat) {
      case M_UP:
	/* If button up, then just deactivate current widget */
	if (cur_but == butn) {
	    switch (cur_type) {
	      case O_BUTTON:
		ulx = buttons[cur_num].ulx;
		uly = buttons[cur_num].uly;
		lrx = buttons[cur_num].lrx;
		lry = buttons[cur_num].lry;

		if (buttons[cur_num].state) {
		    XCopyPlane(dpy, acts_pm, win, fggc, ulx+3, uly+3, lrx - ulx - 4, lry - uly - 4, ulx+3, uly+3, 0x1);
		    buttons[cur_num].state = FALSE;
		    notify_func(O_BUTTON, buttons[cur_num].idno);
		}
		break;

	      case O_MBUTTON:
		if (mbuttons[cur_num].state) {
		    *(mbuttons[cur_num].activeid) = (*(mbuttons[cur_num].activeid) != mbuttons[cur_num].idno) ? mbuttons[cur_num].idno : -1;
		    mbuttons[cur_num].state = FALSE;
		    notify_func(O_MBUTTON, mbuttons[cur_num].idno);
		}
		break;

	      case O_SLIDER:
		ulx = sliders[cur_num].ulx;
		uly = sliders[cur_num].uly;
		lrx = sliders[cur_num].lrx;
		lry = sliders[cur_num].lry;

		if (sliders[cur_num].state == O_LSLIDER) {
		    XCopyPlane(dpy, la_pm, win, fggc, 0, 0, 16, 16, ulx, (uly + lry - 16) / 2, 0x1);
		    sliders[cur_num].active = FALSE;
		    notify_func(O_SLIDER, sliders[cur_num].idno);
		} else if (sliders[cur_num].state == O_RSLIDER) {
		    XCopyPlane(dpy, ra_pm, win, fggc, 0, 0, 16, 16, ulx + 16 + F_WID * NUM_DIGS + 12, (uly + lry - 16) / 2, 0x1);
		    sliders[cur_num].active = FALSE;
		    notify_func(O_SLIDER, sliders[cur_num].idno);
		} else if (sliders[cur_num].state == O_TSLIDER) {
		    if (sliders[cur_num].active) {
			key_active = cur_num;
			key_dirty = FALSE;
			keybuff[0] = '\0';
		    }
		}

		scan_flag = FALSE;
		break;

	      case O_CHECKBOX:
		ulx = cboxes[cur_num].ulx;
		uly = cboxes[cur_num].uly;
		lrx = cboxes[cur_num].lrx;
		lry = cboxes[cur_num].lry;

		if (*(cboxes[cur_num].state) != cboxes[cur_num].nowstate) {
		    *(cboxes[cur_num].state) = cboxes[cur_num].nowstate;
		    update_checkbox(cur_num, FALSE);

		    notify_func(O_CHECKBOX, cboxes[cur_num].idno);
		}
		break;
	    }
	    cur_type = cur_num = 0;
	    cur_but = -1;
	    return TRUE;
	}
	break;

      case M_DOWN:
	if (cur_but < 0) {
	    /* Check buttons */
	    for (i = 0; i < numb; i++) {
		ulx = buttons[i].ulx;

		uly = buttons[i].uly;
		lrx = buttons[i].lrx;
		lry = buttons[i].lry;

		if (buttons[i].win == win && ulx < mx && mx < lrx && uly < my && my < lry) {
		    if (!buttons[i].state) {
			XCopyPlane(dpy, acts_pm, win, revgc, ulx+3, uly+3, lrx - ulx - 4, lry - uly - 4, ulx+3, uly+3, 0x1);
			buttons[i].state = TRUE;
		    }
		    cur_type = O_BUTTON;
		    cur_num = i;
		    cur_but = butn;
		    return TRUE;
		}
	    }

	    /* Check checkboxes */
	    for (i = 0; i < numc; i++) {
		ulx = cboxes[i].ulx;
		uly = cboxes[i].uly;
		lrx = cboxes[i].lrx;
		lry = cboxes[i].lry;

		if (cboxes[i].win == win && ulx < mx && mx < lrx && uly < my && my < lry) {
		    if (*(cboxes[i].state)) {
			cboxes[i].nowstate = FALSE;
		    } else {
			cboxes[i].nowstate = TRUE;
		    }
		    update_checkbox(i, TRUE);

		    cur_type = O_CHECKBOX;
		    cur_num = i;
		    cur_but = butn;
		    return TRUE;
		}
	    }

	    /* Check mode buttons */
	    for (i = 0; i < numm; i++) {
		ulx = mbuttons[i].ulx;
		uly = mbuttons[i].uly;
		lrx = mbuttons[i].lrx;
		lry = mbuttons[i].lry;

		if (mbuttons[i].win == win && ulx < mx && mx < lrx && uly < my && my < lry) {
		    if ((*(mbuttons[i].activeid) == mbuttons[i].idno) && !mbuttons[i].disable)
		      return FALSE;

		    update_mbutton(*(mbuttons[i].activeid), (*(mbuttons[i].activeid) != mbuttons[i].idno) ? mbuttons[i].idno : -1);
		    mbuttons[i].state = TRUE;

		    cur_type = O_MBUTTON;
		    cur_num = i;
		    cur_but = butn;
		    return TRUE;
		}
	    }

	    /* Check sliders */
	    for (i = 0; i < nums; i++) {
		ulx = sliders[i].ulx;
		uly = sliders[i].uly;
		lrx = sliders[i].lrx;
		lry = sliders[i].lry;

		if (sliders[i].win == win && ulx < mx && (uly + lry - 16) / 2 < my && my < (uly + lry - 16) / 2 + 16) {
		    if (mx < ulx + 16) {
			/* Do left arrow */
			if (i == key_active) {
			    key_widgets(K_RETURN, sliders[key_active].win);
			}
			sliders[i].state = O_LSLIDER;
			XCopyPlane(dpy, lap_pm, win, fggc, 0, 0, 16, 16, ulx, (uly + lry - 16) / 2, 0x1);
			if (butn == 1)
			  *(sliders[i].value) -= sliders[i].vincr;
			update_slider(i);
			sliders[i].active = TRUE;
			if (butn != 1)
			  scan_flag = TRUE;
		    } else if (ulx + 16 + F_WID * NUM_DIGS + 12 < mx && mx < ulx + 16 + F_WID * NUM_DIGS + 12 + 16) {
			/* Do right arrow */
			if (i == key_active) {
			    key_widgets(K_RETURN, sliders[key_active].win);
			}
			sliders[i].state = O_RSLIDER;
			XCopyPlane(dpy, rap_pm, win, fggc, 0, 0, 16, 16, ulx + 16 + F_WID * NUM_DIGS + 12, (uly + lry - 16) / 2, 0x1);
			if (butn == 1)
			  *(sliders[i].value) += sliders[i].vincr;
			update_slider(i);
			sliders[i].active = TRUE;
			if (butn != 1)
			  scan_flag = TRUE;
		    } else if (ulx + 18 < mx && mx < ulx + 18 + F_WID * NUM_DIGS + 6) {
			/* Do text box */
			key_widgets(K_RETURN, sliders[key_active].win);
			sliders[i].state = O_TSLIDER;
			sliders[i].active = TRUE;
			update_slider_box(i, ulx, uly, lrx, lry, TRUE);
		    }

		    cur_type = O_SLIDER;
		    cur_num = i;
		    cur_but = butn;
		    return TRUE;
		}
	    }

	}
	break;

      case M_DRAG:
	if (cur_but >= 0) {
	    boolean inside;

	    switch (cur_type) {
	      case O_BUTTON:
		ulx = buttons[cur_num].ulx;
		uly = buttons[cur_num].uly;
		lrx = buttons[cur_num].lrx;
		lry = buttons[cur_num].lry;

		inside = (buttons[cur_num].win == win) && ulx < mx && mx < lrx && uly < my && my < lry;

		if ((inside && !buttons[cur_num].state) || (!inside && buttons[cur_num].state)) {
		    /* Flip button value */
		    XCopyPlane(dpy, acts_pm, win, buttons[cur_num].state ? fggc : revgc, ulx+3, uly+3, lrx - ulx - 4, lry - uly - 4, ulx+3, uly+3, 0x1);
		    buttons[cur_num].state = !buttons[cur_num].state;
		}
		break;

	      case O_MBUTTON:
		ulx = mbuttons[cur_num].ulx;
		uly = mbuttons[cur_num].uly;
		lrx = mbuttons[cur_num].lrx;
		lry = mbuttons[cur_num].lry;

		inside = (mbuttons[cur_num].win == win) && ulx < mx && mx < lrx && uly < my && my < lry;

		if ((inside && !mbuttons[cur_num].state) || (!inside && mbuttons[cur_num].state)) {
		    /* Flip button value */
		    if (mbuttons[cur_num].state)
		      update_mbutton((*(mbuttons[cur_num].activeid) != mbuttons[cur_num].idno) ? mbuttons[cur_num].idno : -1, *(mbuttons[cur_num].activeid));
		    else
		      update_mbutton(*(mbuttons[cur_num].activeid), (*(mbuttons[cur_num].activeid) != mbuttons[cur_num].idno) ? mbuttons[cur_num].idno : -1);

		    mbuttons[cur_num].state = !mbuttons[cur_num].state;
		}
		break;

	      case O_SLIDER:
		ulx = sliders[cur_num].ulx;
		uly = sliders[cur_num].uly;
		lrx = sliders[cur_num].lrx;
		lry = sliders[cur_num].lry;

		inside = ((uly + lry - 16) / 2 < my && my < (uly + lry - 16) / 2 + 16);

		if (sliders[cur_num].state == O_LSLIDER) {
		    inside = inside && (ulx < mx) && (mx < ulx + 16);

		    if (sliders[cur_num].active && !inside) {
			XCopyPlane(dpy, la_pm, win, fggc, 0, 0, 16, 16, ulx, (uly + lry - 16) / 2, 0x1);
			sliders[cur_num].active = FALSE;
		    } else if (!sliders[cur_num].active && inside) {
			XCopyPlane(dpy, lap_pm, win, fggc, 0, 0, 16, 16, ulx, (uly + lry - 16) / 2, 0x1);
			sliders[cur_num].active = TRUE;
		    }
		    scan_flag = (cur_but != 1 && sliders[cur_num].active);
		} else if (sliders[cur_num].state == O_RSLIDER) {
		    inside = inside && (ulx + 16 + F_WID * NUM_DIGS + 12 < mx) && (mx < ulx + 16 + F_WID * NUM_DIGS + 12 + 16);

		    if (sliders[cur_num].active && !inside) {
			XCopyPlane(dpy, ra_pm, win, fggc, 0, 0, 16, 16, ulx + 16 + F_WID * NUM_DIGS + 12, (uly + lry - 16) / 2, 0x1);
			sliders[cur_num].active = FALSE;
		    } else if (!sliders[cur_num].active && inside) {
			XCopyPlane(dpy, rap_pm, win, fggc, 0, 0, 16, 16, ulx + 16 + F_WID * NUM_DIGS + 12, (uly + lry - 16) / 2, 0x1);
			sliders[cur_num].active = TRUE;
		    }
		    scan_flag = (cur_but != 1 && sliders[cur_num].active);
		} else if (sliders[cur_num].state == O_TSLIDER) {
		    inside = (uly + lry - 16) / 2 < my && my <= (uly + lry - 16) / 2 + F_HT + 3 && ulx + 18 < mx && mx < ulx + 18 + F_WID * NUM_DIGS + 6;

		    if ((sliders[cur_num].active && !inside) || (!sliders[cur_num].active && inside)) {
			update_slider_box(cur_num, ulx, uly, lrx, lry, inside);
			sliders[cur_num].active = !sliders[cur_num].active;
		    }
		}
		break;

	      case O_CHECKBOX:
		ulx = cboxes[cur_num].ulx;
		uly = cboxes[cur_num].uly;
		lrx = cboxes[cur_num].lrx;
		lry = cboxes[cur_num].lry;

		inside = (cboxes[cur_num].win == win) && ulx < mx && mx < lrx && uly < my && my < lry;

		if (inside && *(cboxes[cur_num].state) == cboxes[cur_num].nowstate) {
		    cboxes[cur_num].nowstate = !*(cboxes[cur_num].state);
		} else if (!inside && *(cboxes[cur_num].state) != cboxes[cur_num].nowstate) {
		    cboxes[cur_num].nowstate = *(cboxes[cur_num].state);
		} else {
		    break;
		}

		/* Change checkbox value */
		update_checkbox(cur_num, inside);
		break;
	    }
	}
	break;

      case M_HOLD:
	if (cur_but >= 1 && cur_type == O_SLIDER) {
	    double mag = 1.0;

	    if (cur_but == 3)
	      mag = 10.0;

	    if (sliders[cur_num].state == O_LSLIDER) {
		*(sliders[cur_num].value) -= sliders[cur_num].vincr * mag;
		update_slider(cur_num);
	    } else if (sliders[cur_num].state == O_RSLIDER) {
		*(sliders[cur_num].value) += sliders[cur_num].vincr * mag;
		update_slider(cur_num);
	    }
	}
	break;
    }

    return FALSE;
}

#endif

