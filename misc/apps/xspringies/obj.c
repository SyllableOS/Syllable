/* obj.c -- xspringies object (masses, springs) handling
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

#include "defs.h"
#include "obj.h"

#define MPROXIMITY	8.0
#define SPROXIMITY	8.0

/* Object globals */
mass *masses;
spring *springs;
int num_mass, num_spring;
static int num_mass_alloc, num_spring_alloc;
int fake_mass, fake_spring;
static mass *masses_save = NULL;
static spring *springs_save = NULL;
static int num_mass_saved, num_mass_savedalloc, num_spring_saved, num_spring_savedalloc;

/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/

/*** from widget ***/

void activate_mbutton(activeptr, state)
int *activeptr;
boolean state;
{
/*
    int i;

    for (i = 0; i < numm; i++) {
	if (mbuttons[i].activeid == activeptr) {
	    *(mbuttons[i].activeid) = state ? mbuttons[i].idno : -1;
	    break;
	}
    }
*/
}


/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/

/* Current and previous bounding box */
int bb_ulx, bb_uly, bb_lrx, bb_lry;
int bbo_ulx, bbo_uly, bbo_lrx, bbo_lry;
boolean bboxing = TRUE;

/* File operations globals */
int file_op = F_NONE;
char filename[MAXPATH];




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


int cursor_pos = 0, spthick = 0;
/*int main_wid, main_ht, draw_wid, draw_ht, planes, xfd; */
int draw_wid, draw_ht, planes, xfd;


void redisplay_widgets()
{
/*
    XCopyPlane(dpy, acts_pm, acts_win, fggc, 0, 0, B_WID, B_HT, 0, 0, 0x1);

    redraw_names(cur_force);

    redraw_widgets(TRUE);
*/
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


/* fatal(msg): print out the message msg, clean up,
   and exit
   */
void fatal(msg)
char *msg;
{
    if (msg != NULL)
      fprintf(stderr, msg);
/*    clean_up();	*/
    exit(-1);
}

void redraw_widgets(mode)
boolean mode;
{
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
/*
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
*/

/*
    init_x(argc, argv, displayname, geometry, fgcolor, bgcolor, hlcolor);

    init_widgets(widget_notify);
    make_widgets();
*/
    init_objects();



    if ( argc > 1 )
    {
      path = argv[1];
    }
    else
    {
      path = "";
    }
    InitGUI( path );
/*    while (x_event());	*/

/*    clean_up();	*/
    return 0;
}

/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/
/******************************************************************************************************************************/





/* init_objects: create an initial set of masses and
   springs to work with
   */
void init_objects()
{
    /* Create initial objects */
    num_mass = 0;
    num_mass_alloc = ALLOC_SIZE * 2;
    masses = (mass *)xmalloc(sizeof(mass) * num_mass_alloc);

    num_spring = 0;
    num_spring_alloc = ALLOC_SIZE;
    springs = (spring *)xmalloc(sizeof(spring) * num_spring_alloc);

    fake_mass = create_mass();
    masses[fake_mass].status = S_FIXED;
    fake_spring = create_spring();
    springs[fake_spring].status = 0;

    add_massparent(fake_mass, fake_spring);
    springs[fake_spring].m1 = fake_mass;
}

void attach_fake_spring( int tomass )
{
    add_massparent(fake_mass, fake_spring);
    springs[fake_spring].m2 = tomass;
    springs[fake_spring].status |= S_ALIVE;
    springs[fake_spring].ks = mst.cur_ks;
    springs[fake_spring].kd = mst.cur_kd;
}

void kill_fake_spring()
{
    springs[fake_spring].status &= ~S_ALIVE;
}

void move_fake_mass( int mx, int my )
{
    masses[fake_mass].x = mx;
    masses[fake_mass].y = my;
}

/* create_mass: return the index for a new mass,
   possibly allocating more space if necesary
   */
int create_mass()
{
    if (num_mass >= num_mass_alloc) {
	/* Allocate more masses */
	num_mass_alloc += ALLOC_SIZE;
	masses = (mass *)xrealloc(masses, sizeof(mass) * num_mass_alloc);
    }

    /* Set parameters for new mass */
    masses[num_mass].x = masses[num_mass].y = 0.0;
    masses[num_mass].vx = masses[num_mass].vy = 0.0;
    masses[num_mass].ax = masses[num_mass].ay = 0.0;
    masses[num_mass].mass = masses[num_mass].elastic = 0.0;
    masses[num_mass].radius = masses[num_mass].num_pars = 0;
    masses[num_mass].pars = NULL;
    masses[num_mass].status = S_ALIVE;

    /* Return next unused mass */
    return num_mass++;
}

/* create_spring: return the index for a new spring,
   possibly allocating more space if necesary
   */
int create_spring()
{
    if (num_spring >= num_spring_alloc) {
	/* Allocate more springs */
	num_spring_alloc += ALLOC_SIZE;
	springs = (spring *)xrealloc(springs, sizeof(spring) * num_spring_alloc);
    }

    /* Set parameters for new spring */
    springs[num_spring].ks = springs[num_spring].kd = 0.0;
    springs[num_spring].restlen = 0.0;
    springs[num_spring].m1 = springs[num_spring].m2 = 0;
    springs[num_spring].status = S_ALIVE;

    /* Return next unused spring */
    return num_spring++;
}

void add_massparent(which, parent)
int which, parent;
{
    int len = masses[which].num_pars++;

    masses[which].pars = (int *)xrealloc(masses[which].pars, (len + 1) * sizeof(int));

    masses[which].pars[len] = parent;
}

void del_massparent(which, parent)
int which, parent;
{
    int i;

    if (masses[which].status & S_ALIVE) {
	for (i = 0; i < masses[which].num_pars; i++) {
	    if (masses[which].pars[i] == parent) {
		if (i == masses[which].num_pars - 1) {
		    masses[which].num_pars--;
		} else {
		    masses[which].pars[i] = masses[which].pars[--masses[which].num_pars];
		}
		return;
	    }
	}
    }
}

/* delete_spring: delete a particular spring
 */
void delete_spring(which)
int which;
{
    if (springs[which].status & S_ALIVE) {
        springs[which].status = 0;
	del_massparent(springs[which].m1, which);
	del_massparent(springs[which].m2, which);
    }
}

/* delete_mass: delete a particular mass, and all springs
   directly attached to it
   */
void delete_mass(which)
int which;
{
    int i;

    if (masses[which].status & S_ALIVE) {
	masses[which].status = 0;

	/* Delete all springs connected to it */
	for (i = 0; i < masses[which].num_pars; i++) {
	    delete_spring(masses[which].pars[i]);
	}
    }

    if (which == mst.center_id)
      mst.center_id = -1;
}

/* delete_selected: delete all objects which
   are currently selected
   */
void delete_selected()
{
    int i;

    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_SELECTED) {
	    delete_mass(i);
	}
    }

    for (i = 0; i < num_spring; i++) {
	if (springs[i].status & S_SELECTED) {
	    delete_spring(i);
	}
    }
}

void delete_all()
{
    int i;

    for (i = 0; i < num_mass; i++) {
	free(masses[i].pars);
    }
    free(masses);
    num_mass = num_mass_alloc = 0;
    free(springs);
    num_spring = num_spring_alloc = 0;
    mst.center_id = -1;
}

void reconnect_masses()
{
    int i;

    for (i = 0; i < num_mass; i++) {
	masses[i].num_pars = 0;
	masses[i].pars = NULL;
    }

    for (i = 0; i < num_spring; i++) {
	add_massparent(springs[i].m1, i);
	add_massparent(springs[i].m2, i);
    }
}

void restore_state()
{
    delete_all();

    if (masses_save != NULL) {
	num_mass = num_mass_saved;
	num_mass_alloc = num_mass_savedalloc;
	num_spring = num_spring_saved;
	num_spring_alloc = num_spring_savedalloc;

	masses = (mass *)xmalloc(sizeof(mass) * num_mass_alloc);
	bcopy(masses_save, masses, sizeof(mass) * num_mass_alloc);
	springs = (spring *)xmalloc(sizeof(spring) * num_spring_alloc);
	bcopy(springs_save, springs, sizeof(spring) * num_spring_alloc);

	reconnect_masses();
    }
}

void save_state()
{
    masses_save = (mass *)xmalloc(sizeof(mass) * num_mass_alloc);
    bcopy(masses, masses_save, sizeof(mass) * num_mass_alloc);
    num_mass_saved = num_mass;
    num_mass_savedalloc = num_mass_alloc;

    springs_save = (spring *)xmalloc(sizeof(spring) * num_spring_alloc);
    bcopy(springs, springs_save, sizeof(spring) * num_spring_alloc);
    num_spring_saved = num_spring;
    num_spring_savedalloc = num_spring_alloc;
}

/* nearest_object:  Find the nearest spring or mass to the position
   (x,y), or return -1 if none are close.  Set is_mass accordingly
   */
int nearest_object(x, y, is_mass)
int x, y;
boolean *is_mass;
{
    int i, closest = -1;
    double dist, min_dist = MPROXIMITY * MPROXIMITY, rating, min_rating = draw_wid * draw_ht;
    boolean masses_only = *is_mass;

    *is_mass = TRUE;

    if (masses_only)
      min_dist = min_dist * 36;

    /* Find closest mass */
    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_ALIVE) {
	    if ((dist = SQR(masses[i].x - (double)x) + SQR(masses[i].y - (double)y) - (double)SQR(masses[i].radius)) < min_dist) {
		rating = SQR(masses[i].x - (double)x) + SQR(masses[i].y - (double)y);
		if (rating < min_rating) {
		    min_dist = dist;
		    min_rating = rating;
		    closest = i;
		}
	    }
	}
    }

    if (closest != -1)
      return closest;

    if (masses_only)
      return -1;

    *is_mass = TRUE;

    min_dist = SPROXIMITY;

    /* Find closest spring */
    for (i = 0; i < num_spring; i++) {
	double x1, x2, y1, y2;

	if (springs[i].status & S_ALIVE) {
	    x1 = masses[springs[i].m1].x;
	    y1 = masses[springs[i].m1].y;
	    x2 = masses[springs[i].m2].x;
	    y2 = masses[springs[i].m2].y;

	    if (x > MIN(x1, x2) - SPROXIMITY && x < MAX(x1, x2) + SPROXIMITY &&
		y > MIN(y1, y2) - SPROXIMITY && y < MAX(y1, y2) + SPROXIMITY) {
		double a1, b1, c1, dAB, d;

		a1 = y2 - y1;
		b1 = x1 - x2;
		c1 = y1 * x2 - y2 * x1;
		dAB = sqrt((double)(a1*a1 + b1*b1));
		d = (x * a1 + y * b1 + c1) / dAB;

		dist = ABS(d);

		if (dist < min_dist) {
		    min_dist = dist;
		    closest = i;
		    *is_mass = FALSE;
		}
	    }
	}
    }

    return closest;
}

void eval_selection()
{
    int i;
    double sel_mass, sel_elas, sel_ks, sel_kd;
    boolean sel_fix;
    boolean found = FALSE, changed = FALSE;
    boolean mass_same, elas_same, ks_same, kd_same, fix_same;

    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_SELECTED) {
	    if (found) {
		if (mass_same && masses[i].mass != sel_mass) {
		    mass_same = FALSE;
		}
		if (elas_same && masses[i].elastic != sel_elas) {
		    elas_same = FALSE;
		}
		if (fix_same && (masses[i].status & S_FIXED)) {
		    fix_same = FALSE;
		}
	    } else {
		found = TRUE;
		sel_mass = masses[i].mass;
		mass_same = TRUE;
		sel_elas = masses[i].elastic;
		elas_same = TRUE;
	        sel_fix = (masses[i].status & S_FIXED);
		fix_same = TRUE;
	    }
	}
    }

    if (found) {
	if (mass_same && sel_mass != mst.cur_mass) {
	    mst.cur_mass = sel_mass;
	    changed = TRUE;
	}
	if (elas_same && sel_elas != mst.cur_rest) {
	    mst.cur_rest = sel_elas;
	    changed = TRUE;
	}
	if (fix_same && sel_fix != mst.fix_mass) {
	    mst.fix_mass = sel_fix;
	    changed = TRUE;
	}
    }

    found = FALSE;
    for (i = 0; i < num_spring; i++) {
	if (springs[i].status & S_SELECTED) {
	    if (found) {
		if (ks_same && springs[i].ks != sel_ks) {
		    ks_same = FALSE;
		}
		if (ks_same && springs[i].ks != sel_ks) {
		    ks_same = FALSE;
		}
	    } else {
		found = TRUE;
		sel_ks = springs[i].ks;
		ks_same = TRUE;
	        sel_kd = springs[i].kd;
		kd_same = TRUE;
	    }
	}
    }

    if (found) {
	if (ks_same && sel_ks != mst.cur_ks) {
	    mst.cur_ks = sel_ks;
	    changed = TRUE;
	}
	if (kd_same && sel_kd != mst.cur_kd) {
	    mst.cur_kd = sel_kd;
	    changed = TRUE;
	}
    }

    if (changed) {
	redraw_widgets(FALSE);
    }
}

boolean anything_selected()
{
    int i;

    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_SELECTED)
	  return TRUE;
    }
    for (i = 0; i < num_spring; i++) {
	if (springs[i].status & S_SELECTED)
	  return TRUE;
    }
    return FALSE;
}

void select_object(selection, is_mass, shifted)
int selection;
boolean is_mass, shifted;
{
    if (is_mass) {
	if (shifted) {
	    masses[selection].status ^= S_SELECTED;
	} else {
	    masses[selection].status |= S_SELECTED;
	}
    } else {
	if (shifted) {
	    springs[selection].status ^= S_SELECTED;
	} else {
	    springs[selection].status |= S_SELECTED;
	}
    }
}

void select_objects(ulx, uly, lrx, lry, shifted)
int ulx, uly, lrx, lry;
boolean shifted;
{
    int i;

    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_ALIVE) {
	    if (ulx <= masses[i].x && masses[i].x <= lrx && uly <= masses[i].y && masses[i].y <= lry) {
		select_object(i, TRUE, FALSE);
	    }
	}
    }

    for (i = 0; i < num_spring; i++) {
	if (springs[i].status & S_ALIVE) {
	    int m1, m2;

	    m1 = springs[i].m1;
	    m2 = springs[i].m2;

	    if (ulx <= masses[m1].x && masses[m1].x <= lrx && uly <= masses[m1].y && masses[m1].y <= lry &&
		ulx <= masses[m2].x && masses[m2].x <= lrx && uly <= masses[m2].y && masses[m2].y <= lry) {
		select_object(i, FALSE, FALSE);
	    }
	}
    }
}

void unselect_all()
{
    int i;

    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_SELECTED) {
	    masses[i].status &= ~S_SELECTED;
	}
    }

    for (i = 0; i < num_spring; i++) {
	if (springs[i].status & S_SELECTED) {
	    springs[i].status &= ~S_SELECTED;
	}
    }
}

void select_all()
{
    int i;

    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_ALIVE) {
	    masses[i].status |= S_SELECTED;
	}
    }

    for (i = 0; i < num_spring; i++) {
	if (springs[i].status & S_ALIVE) {
	    springs[i].status |= S_SELECTED;
	}
    }
}

void duplicate_selected()
{
    int i, j, *mapfrom, *mapto, num_map, num_map_alloc, spring_start;
    int which;

    spring_start = num_spring;

    num_map = 0;
    num_map_alloc = ALLOC_SIZE;
    mapfrom = (int *)xmalloc(sizeof(int) * num_map_alloc);
    mapto = (int *)xmalloc(sizeof(int) * num_map_alloc);

    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_SELECTED) {
	    if (num_map >= num_map_alloc) {
		num_map_alloc += ALLOC_SIZE;
		mapfrom = (int *)xrealloc(mapfrom, sizeof(int) * num_map_alloc);
		mapto = (int *)xrealloc(mapto, sizeof(int) * num_map_alloc);
	    }

	    which = create_mass();
	    mapto[num_map] = which;
	    mapfrom[num_map] = i;
	    num_map++;
	    masses[which] = masses[i];
	    masses[which].status &= ~S_SELECTED;
	    masses[which].num_pars = 0;
	    masses[which].pars = NULL;
	}
    }

    for (i = 0; i < spring_start; i++) {
	if (springs[i].status & S_SELECTED) {
	    boolean m1done, m2done;

	    m1done = m2done = FALSE;

	    which = create_spring();
	    springs[which] = springs[i];
	    springs[which].status &= ~S_SELECTED;

	    for (j = 0; (!m1done || !m2done) && j < num_map; j++) {
		if (!m1done && springs[which].m1 == mapfrom[j]) {
		    springs[which].m1 = mapto[j];
		    add_massparent(mapto[j], which);
		    m1done = TRUE;
		}
		if (!m2done && springs[which].m2 == mapfrom[j]) {
		    springs[which].m2 = mapto[j];
		    add_massparent(mapto[j], which);
		    m2done = TRUE;
		}
	    }
	    if (!m1done && !m2done) {
		/* delete spring that isn't connected to anyone */
		delete_spring(which);
	    }
	}
    }

    free(mapfrom);
    free(mapto);
}

void translate_selobj(dx, dy)
int dx, dy;
{
    int i;

    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_SELECTED) {
	    masses[i].x += dx;
	    masses[i].y += dy;
	}
    }
}

void changevel_selobj( int vx, int vy, boolean relative )
{
    int i;

    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_SELECTED) {
	    if (relative) {
		masses[i].vx += vx;
		masses[i].vy += vy;
	    } else {
		masses[i].vx = vx;
		masses[i].vy = vy;
	    }
	}
    }
}

void tempfixed_obj( boolean store )
{
    int i;

    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_SELECTED) {
	    if (store) {
		masses[i].status &= ~S_TEMPFIXED;
		if (!(masses[i].status & S_FIXED)) {
		    masses[i].status |= (S_TEMPFIXED | S_FIXED);
		}
	    } else {
		if (masses[i].status & S_TEMPFIXED) {
		    masses[i].status &= ~S_FIXED;
		}
	    }
	}
    }
}

void set_sel_restlen()
{
    int i;
    double dx, dy;

    for (i = 0; i < num_spring; i++) {
	if (springs[i].status & S_SELECTED) {
	    dx = masses[springs[i].m1].x - masses[springs[i].m2].x;
	    dy = masses[springs[i].m1].y - masses[springs[i].m2].y;
	    springs[i].restlen = sqrt(dx * dx + dy * dy);
	}
    }
}

void set_center()
{
    int i, cent = -1;

    for (i = 0; i < num_mass; i++) {
	if (masses[i].status & S_SELECTED) {
	    if (cent != -1)
	      return;

	    cent = i;
	}
    }

    mst.center_id = cent;
}
