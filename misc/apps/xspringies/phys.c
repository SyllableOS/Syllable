/* phys.c -- xspringies physical modeling and numerical solving routines
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

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define DT_MIN		0.0001
#define DT_MAX		0.5

#define MAXCON		1024

#define CONSTR_KS	1.0
#define CONSTR_KD	1.0

/* Do fudgy bounces */
#define BOUNCE_FUDGE	1

/* Stickiness calibration:  STICK_MAG = 1.0, means that a mass = 1.0 with gravity = 1.0 will remain
   stuck on a wall for all stickiness values > 1.0 */
#define STICK_MAG	1.0

void accumulate_accel()
{
    double gval, gmisc;
    double gx = 0, gy = 0, ogx = 0, ogy = 0;
    double center_x = draw_wid/2.0, center_y = draw_ht/2.0, center_rad = 1.0;
    mass *m, *m1, *m2;
    spring *s;
    register int i;

    /* ------------------ applied force effects ----------------------- */

    if (mst.center_id >= 0) {
	if (masses[mst.center_id].status & S_ALIVE) {
	    center_x = masses[mst.center_id].x;
	    center_y = masses[mst.center_id].y;
	} else {
	    mst.center_id = -1;
	}
    }

    /* Do gravity */
    if (mst.bf_mode[FR_GRAV] > 0) {
	gval = mst.cur_grav_val[FR_GRAV];
	gmisc = mst.cur_misc_val[FR_GRAV];

	gx = COORD_DX(gval * sin(gmisc * M_PI / 180.0));
	gy = COORD_DY(gval * cos(gmisc * M_PI / 180.0));
    }

    /* Keep center of mass in the middle force */
    if (mst.bf_mode[FR_CMASS] > 0) {
	double mixix = 0.0, mixiy = 0.0, mivix = 0.0, miviy = 0.0, msum = 0.0;
	gval = mst.cur_grav_val[FR_CMASS];
	gmisc = mst.cur_misc_val[FR_CMASS];

	for (i = 0; i < num_mass; i++) {
	    m = masses + i;
	    if (i != mst.center_id && (m->status & S_ALIVE) && !(m->status & S_FIXED)) {
		msum += m->mass;
		mixix += m->mass * m->x;
		mixiy += m->mass * m->y;
		mivix += m->mass * m->vx;
		miviy += m->mass * m->vy;
	    }
	}

	if (msum) {
	    mixix /= msum;
	    mixiy /= msum;
	    mivix /= msum;
	    miviy /= msum;

	    mixix -= center_x;
	    mixiy -= center_y;

	    ogx -= (gval * mixix + gmisc * mivix) / msum;
	    ogy -= (gval * mixiy + gmisc * miviy) / msum;
	}
    }

    /* Apply Gravity, CM and air drag to all masses */
    for (i = 0; i < num_mass; i++) {
	m = masses + i;
	if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
	    /* Do viscous drag */
	    if (i != mst.center_id) {
		m->ax = gx + ogx - mst.cur_visc * m->vx;
		m->ay = gy + ogy - mst.cur_visc * m->vy;
	    } else {
		m->ax = gx - mst.cur_visc * m->vx;
		m->ay = gy - mst.cur_visc * m->vy;
	    }
	}
    }

    /* Do point attraction force */
    if (mst.bf_mode[FR_PTATTRACT] > 0) {
	gval = mst.cur_grav_val[FR_PTATTRACT];
	gmisc = mst.cur_misc_val[FR_PTATTRACT];

	for (i = 0; i < num_mass; i++) {
	    m = masses + i;
	    if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
		double dx, dy, mag, fmag;

		dx = (center_x - m->x);
		dy = (center_y - m->y);
		mag = sqrt(dx * dx + dy * dy);

		if (mag < m->radius + center_rad) {
		    dx *= mag / (m->radius + center_rad);
		    dy *= mag / (m->radius + center_rad);
		    mag = m->radius + center_rad;
		}

		fmag = gval / pow(mag, gmisc);

		m->ax += fmag * dx / mag;
		m->ay += fmag * dy / mag;
	    }
	}
    }

    /* Wall attract/repel force */
    if (mst.bf_mode[FR_WALL] > 0) {
	double dax, day, dist;

	gval = -mst.cur_grav_val[FR_WALL];
	gmisc = mst.cur_misc_val[FR_WALL];

	for (i = 0; i < num_mass; i++) {
	    m = masses + i;
	    if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
		dax = day = 0;

		if (mst.w_left && (dist = m->x - m->radius) >= 0) {
		    if (dist < 1)  dist = 1;
		    dist = pow(dist, gmisc);
		    dax -= gval / dist;
		}
		if (mst.w_right && (dist = draw_wid - m->radius - m->x) >= 0) {
		    if (dist < 1)  dist = 1;
		    dist = pow(dist, gmisc);
		    dax += gval / dist;
		}
		if (mst.w_top && (dist = draw_ht - m->radius - m->y) >= 0) {
		    if (dist < 1)  dist = 1;
		    dist = pow(dist, gmisc);
		    day += gval / dist;
		}
		if (mst.w_bottom && (dist = m->y - m->radius) >= 0) {
		    if (dist < 1)  dist = 1;
		    dist = pow(dist, gmisc);
		    day -= gval / dist;
		}

		m->ax += dax;
		m->ay += day;
	    }
	}
    }

    /* ------------------ spring effects ----------------------- */

    /* Spring compression/damping effects on masses */
    for (i = 0; i < num_spring; i++) {
	s = springs + i;
	if (s->status & S_ALIVE) {
	    double dx, dy, force, forcex, forcey, mag, damp, mass1, mass2;

	    m1 = masses + s->m1;
	    m2 = masses + s->m2;

	    dx = m1->x - m2->x;
	    dy = m1->y - m2->y;

	    if (dx || dy) {
		mag = sqrt(dx * dx + dy * dy);

		force = s->ks * (s->restlen - mag);
		if (s->kd) {
		    damp = ((m1->vx - m2->vx) * dx + (m1->vy - m2->vy) * dy) / mag;
		    force -= s->kd * damp;
		}

		force /= mag;
		forcex = force * dx;
		forcey = force * dy;

		mass1 = m1->mass;
		mass2 = m2->mass;

		m1->ax += forcex / mass1;
		m1->ay += forcey / mass1;
		m2->ax -= forcex / mass2;
		m2->ay -= forcey / mass2;
	    }
	}
    }
}

void runge_kutta(h, testloc)
double h;
boolean testloc;
{
    mass *m;
    int i;

    accumulate_accel();

    /* k1 step */
    for (i = 0; i < num_mass; i++) {
	m = masses + i;

	if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
	    /* Initial storage */
	    m->cur_x = m->x;
	    m->cur_y = m->y;
	    m->cur_vx = m->vx;
	    m->cur_vy = m->vy;

	    m->k1x = m->vx * h;
	    m->k1y = m->vy * h;
	    m->k1vx = m->ax * h;
	    m->k1vy = m->ay * h;

	    m->x = m->cur_x + m->k1x / 2;
	    m->y = m->cur_y + m->k1y / 2;
	    m->vx = m->cur_vx + m->k1vx / 2;
	    m->vy = m->cur_vy + m->k1vy / 2;
	}
    }

    accumulate_accel();

    /* k2 step */
    for (i = 0; i < num_mass; i++) {
	m = masses + i;
	if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
	    m->k2x = m->vx * h;
	    m->k2y = m->vy * h;
	    m->k2vx = m->ax * h;
	    m->k2vy = m->ay * h;

	    m->x = m->cur_x + m->k2x / 2;
	    m->y = m->cur_y + m->k2y / 2;
	    m->vx = m->cur_vx + m->k2vx / 2;
	    m->vy = m->cur_vy + m->k2vy / 2;
	}
    }

    accumulate_accel();

    /* k3 step */
    for (i = 0; i < num_mass; i++) {
	m = masses + i;
	if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
	    m->k3x = m->vx * h;
	    m->k3y = m->vy * h;
	    m->k3vx = m->ax * h;
	    m->k3vy = m->ay * h;

	    m->x = m->cur_x + m->k3x;
	    m->y = m->cur_y + m->k3y;
	    m->vx = m->cur_vx + m->k3vx;
	    m->vy = m->cur_vy + m->k3vy;
	}
    }

    accumulate_accel();

    /* k4 step */
    for (i = 0; i < num_mass; i++) {
	m = masses + i;
	if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
	    m->k4x = m->vx * h;
	    m->k4y = m->vy * h;
	    m->k4vx = m->ax * h;
	    m->k4vy = m->ay * h;
	}
    }

    /* Find next position */
    for (i = 0; i < num_mass; i++) {
	m = masses + i;
	if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
	    if (testloc) {
		m->test_x = m->cur_x + (m->k1x/2.0 + m->k2x + m->k3x + m->k4x/2.0)/3.0;
		m->test_y = m->cur_y + (m->k1y/2.0 + m->k2y + m->k3y + m->k4y/2.0)/3.0;
		m->test_vx = m->cur_vx + (m->k1vx/2.0 + m->k2vx + m->k3vx + m->k4vx/2.0)/3.0;
		m->test_vy = m->cur_vy + (m->k1vy/2.0 + m->k2vy + m->k3vy + m->k4vy/2.0)/3.0;
	    } else {
		m->x = m->cur_x + (m->k1x/2.0 + m->k2x + m->k3x + m->k4x/2.0)/3.0;
		m->y = m->cur_y + (m->k1y/2.0 + m->k2y + m->k3y + m->k4y/2.0)/3.0;
		m->vx = m->cur_vx + (m->k1vx/2.0 + m->k2vx + m->k3vx + m->k4vx/2.0)/3.0;
		m->vy = m->cur_vy + (m->k1vy/2.0 + m->k2vy + m->k3vy + m->k4vy/2.0)/3.0;
	    }
	}
    }
}

void adaptive_runge_kutta()
{
    int i;
    mass *m;
    double err, maxerr;

  restart:
    if (mst.cur_dt > DT_MAX)
      mst.cur_dt = DT_MAX;
    if (mst.cur_dt < DT_MIN)
      mst.cur_dt = DT_MIN;

    runge_kutta(mst.cur_dt/2.0, FALSE);
    runge_kutta(mst.cur_dt/2.0, TRUE);

    for (i = 0; i < num_mass; i++) {
	m = masses + i;
	if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
	    m->x = m->old_x;
	    m->y = m->old_y;
	    m->vx = m->old_vx;
	    m->vy = m->old_vy;
	}
    }
    runge_kutta(mst.cur_dt, FALSE);

    /* Find error */
    maxerr = 0.00001;
    for (i = 0; i < num_mass; i++) {
	m = masses + i;
	if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
	    err = fabs(m->x - m->test_x) + fabs(m->y - m->test_y) +
	      fabs(m->vx - m->test_vx) + fabs(m->vy - m->test_vy);

	    if (err > maxerr) {
		maxerr = err;
	    }
	}
    }

    /* Fudgy scale factor -- user controlled */
    maxerr /= mst.cur_prec;

    if (maxerr < 1.0) {
	mst.cur_dt *= 0.9 * exp(-log(maxerr)/8.0);
    } else {
	if (mst.cur_dt > DT_MIN) {
	    for (i = 0; i < num_mass; i++) {
		m = masses + i;
		if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
		    m->x = m->old_x;
		    m->y = m->old_y;
		    m->vx = m->old_vx;
		    m->vy = m->old_vy;
		}
	    }

	    mst.cur_dt *= 0.9 * exp(-log(maxerr)/4.0);

	    goto restart;
	}
    }
}

boolean animate_obj()
{
    mass *m;
    spring *s;
    int i;
    double stick_mag;
    static int num_since = 0;
    static double time_elapsed = 0.0;

    /* Save initial values */
    for (i = 0; i < num_mass; i++) {
	m = masses + i;
	if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
	    m->old_x = m->x;
	    m->old_y = m->y;
	    m->old_vx = m->vx;
	    m->old_vy = m->vy;
	}
    }

    if (mst.adaptive_step) {
	boolean any_spring = FALSE;

	for (i = 0; i < num_spring; i++) {
	    s = springs + i;
	    if (s->status & S_ALIVE) {
		any_spring = TRUE;
		break;
	    }
	}

	/* If no springs, then use dt=DEF_TSTEP */
	if (any_spring) {
	    adaptive_runge_kutta();
	} else {
	    runge_kutta(mst.cur_dt = DEF_TSTEP, FALSE);
	}
    } else {
	runge_kutta(mst.cur_dt, FALSE);
    }

    stick_mag = STICK_MAG * mst.cur_dt * mst.cur_stick;

    /* Crappy wall code */
    for (i = 0; i < num_mass; i++) {
	m = masses + i;

	if ((m->status & S_ALIVE) && !(m->status & S_FIXED)) {
	    /* Delete "exploded" objects */
/*
			Seems to fail on AMD-K6!!

	    if (m->ax - m->ax != 0.0 || m->ay - m->ay != 0.0 || m->x - m->x != 0.0 || m->y - m->y != 0.0) {
				delete_mass(i);
				continue;
	    }
*/
	    /* Check if stuck to a wall */
	    if (m->old_vx == 0.0 && m->old_vy == 0.0) {
		/* Left or right wall */
		if ((mst.w_left && ABS(m->old_x - m->radius) < 0.5) || (mst.w_right && ABS(m->old_x - draw_wid + m->radius) < 0.5)) {
		    if (ABS(m->vx) < stick_mag / m->mass) {
			m->vx = m->vy = 0;
			m->x = m->old_x;
			m->y = m->old_y;

			continue;
		    }
		} else if ((mst.w_bottom && ABS(m->old_y - m->radius) < 0.5) || (mst.w_top && ABS(m->old_y - draw_ht + m->radius) < 0.5)) {
		    /* Top or bottom wall */
		    if (ABS(m->vy) < stick_mag / m->mass) {
			m->vx = m->vy = 0;
			m->x = m->old_x;
			m->y = m->old_y;

			continue;
		    }
		}
	    }

	    /* Bounce off left or right wall */
	    if (mst.w_left && m->x < m->radius && m->old_x >= m->radius) {
		m->x = m->radius;

		if (m->vx < 0) {
		    m->vx = -m->vx * m->elastic;
		    m->vy *= m->elastic;

		    /* Get stuck if not going fast enough */
		    if (m->vx > 0) {
			m->vx -= STICK_MAG * mst.cur_stick / m->mass;

			if (m->vx < 0) {
			    m->vx = m->vy = 0;
			}
		    }
		}
	    } else if (mst.w_right && m->x > draw_wid - m->radius && m->old_x <= draw_wid - m->radius) {
		m->x = draw_wid - m->radius;

		if (m->vx > 0) {
		    m->vx = -m->vx * m->elastic;
		    m->vy *= m->elastic;

		    /* Get stuck if not going fast enough */
		    if (m->vx < 0) {
			m->vx += STICK_MAG * mst.cur_stick / m->mass;

			if (m->vx > 0) {
			    m->vx = m->vy = 0;
			}
		    }
		}
	    }
	    /* Stick to top or bottom wall */
	    if (mst.w_bottom && m->y < m->radius && m->old_y >= m->radius) {
		m->y = m->radius;

		if (m->vy < 0) {
		    m->vy = -m->vy * m->elastic;
		    m->vx *= m->elastic;

		    /* Get stuck if not going fast enough */
		    if (m->vy > 0) {
			m->vy -= STICK_MAG * mst.cur_stick / m->mass;

			if (m->vy < 0) {
			    m->vx = m->vy = 0;
			}
		    }
		}
	    } else if (mst.w_top && m->y > (draw_ht - m->radius) && m->old_y <= (draw_ht - m->radius)) {
		m->y = draw_ht - m->radius;

		if (m->vy > 0) {
		    m->vy = -m->vy * m->elastic;
		    m->vx *= m->elastic;

		    /* Get stuck if not going fast enough */
		    if (m->vy < 0) {
			m->vy += STICK_MAG * mst.cur_stick / m->mass;

			if (m->vy > 0) {
			    m->vx = m->vy = 0;
			}
		    }
		}
	    }
	}
    }

    time_elapsed += mst.cur_dt;

    if (time_elapsed > 0.05) {
        time_elapsed -= 0.05;
        num_since = 0;
        return TRUE;
    }

    num_since++;

    if (num_since > 8) {
        num_since = 0;
        return TRUE;
    }
    return FALSE;
}