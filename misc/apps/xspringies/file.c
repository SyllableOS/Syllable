/* file.c -- file loading and saving for xspringies
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
/*#include <pwd.h>	*/

#define USERNAMELEN	8
#define MAGIC_CMD	"#1.0"
#define FILE_EXT	".xsp"

/* tilde_expand: expand ~/.. and ~bar/.. in filenames and put
   the result in filename
   filename is assumed to be null terminated, and after the
   expansion will not exceed MAXPATH in length

   tilde_expand returns its argument, or NULL if the user
   is unknown
   */


char *tilde_expand(fname)
char *fname;
{
#if 0

    register int prelen, len, restlen, i;
    register char *prefix, *s, *u;
    struct passwd *pw;
    char user[USERNAMELEN+1];
    extern char *getenv();

    if (*fname == '~') {
	if (*(fname + 1) == '/' || !*(fname + 1)) {
	    len = 1;
	    /* Do ~/ expansion */
	    if ((prefix = getenv("HOME")) == NULL) {
		/* Use . as home directory if HOME not set */
		prefix = ".";
	    }
	} else {
	    /* Do ~user/ expansion */
	    for (len = 1, s = fname + 1, u = user; len <= USERNAMELEN && *s && *s != '/'; *u++ = *s++, len++);
	    *u = '\0';

	    /* Get name */
	    if ((pw = getpwnam(user)) == NULL)
	      return NULL;
	    prefix = pw->pw_dir;
	}

	prelen = strlen(prefix);
	restlen = strlen(fname + len);

	/* Move over pathname */
	if (prelen < len) {
	    for (i = 0; i <= restlen; i++) {
		fname[prelen+i] = fname[len+i];
	    }
	} else {
	    for (i = restlen; i >= 0; i--) {
		fname[prelen+i] = fname[len+i];
	    }
	}
	/* Copy in prefix */
	(void)memcpy(fname, prefix, prelen);
    }
#endif

    return fname;
}



void skip_to_eol(f)
FILE *f;
{
    int ch;

    while ((ch = fgetc(f)) != EOF && ch != '\n');
}

char *extend_file(file)
char *file;
{
    static char buf[MAXPATH];
    int len, felen;

    strcpy(buf, file);

    felen = strlen(FILE_EXT);
    len = strlen(buf);

    if (len < felen || strcmp(buf + len - felen, FILE_EXT)) {
	strcat(buf, FILE_EXT);
    }

    return buf;
}

#define IS_CMD(s)	(!strncmp(cmd, s, 4))

boolean file_command( char* file, int command )
{
    FILE *f;
    char cmd[5];
    int i, which, spring_start, temp;
    int *mapfrom, *mapto, num_map, num_map_alloc;
    boolean selectnew = FALSE;

    if (strlen(file) == 0)
      return FALSE;

    tilde_expand(file);

    switch (command) {
      case F_INSERT:
      case F_LOAD:
	if ((f = fopen(extend_file(file), "r")) == NULL) {
	    return FALSE;
	}

	/* Check first line */
	if (fgets(cmd, 5, f) != NULL && IS_CMD(MAGIC_CMD)) {
	    skip_to_eol(f);

	    if (command == F_LOAD) {
		delete_all();
		init_objects();
	    } else {
		if (!anything_selected())
		  selectnew = TRUE;
	    }
	    spring_start = num_spring;

	    num_map = 0;
	    num_map_alloc = ALLOC_SIZE;
	    mapfrom = (int *)xmalloc(sizeof(int) * num_map_alloc);
	    mapto = (int *)xmalloc(sizeof(int) * num_map_alloc);

	    while (fgets(cmd, 5, f) != NULL) {
		if (IS_CMD("mass")) {
		    if (num_map >= num_map_alloc) {
			num_map_alloc += ALLOC_SIZE;
			mapfrom = (int *)xrealloc(mapfrom, sizeof(int) * num_map_alloc);
			mapto = (int *)xrealloc(mapto, sizeof(int) * num_map_alloc);
		    }

		    which = create_mass();
		    mapto[num_map] = which;
		    fscanf(f, "%d %lf %lf %lf %lf %lf %lf\n", &mapfrom[num_map], &masses[which].x, &masses[which].y,
			   &masses[which].vx, &masses[which].vy, &masses[which].mass, &masses[which].elastic);
		    num_map++;
		    if (masses[which].mass < 0) {
			masses[which].mass = -masses[which].mass;
			masses[which].status |= S_FIXED;
		    }
		    masses[which].radius = mass_radius(masses[which].mass);
		    if (selectnew) {
			select_object(which, TRUE, FALSE);
		    }
		} else if (IS_CMD("spng")) {
		    int bogus;

		    which = create_spring();
		    fscanf(f, "%d %d %d %lf %lf %lf\n", &bogus, &springs[which].m1, &springs[which].m2,
			   &springs[which].ks, &springs[which].kd, &springs[which].restlen);
		    if (selectnew) {
			select_object(which, FALSE, FALSE);
		    }

		} else if (command == F_INSERT) {
		    /* skip non mass/spring commands if in insert mode */
		} else if (IS_CMD("cmas")) {
		    fscanf(f, "%lf\n", &(mst.cur_mass));
		} else if (IS_CMD("elas")) {
		    fscanf(f, "%lf\n", &(mst.cur_rest));
		} else if (IS_CMD("kspr")) {
		    fscanf(f, "%lf\n", &(mst.cur_ks));
		} else if (IS_CMD("kdmp")) {
		    fscanf(f, "%lf\n", &(mst.cur_kd));
		} else if (IS_CMD("fixm")) {
		    fscanf(f, "%d\n", &temp);
		    mst.fix_mass = temp ? TRUE : FALSE;
		} else if (IS_CMD("shws")) {
		    fscanf(f, "%d\n", &temp);
		    mst.show_spring = temp ? TRUE : FALSE;
		} else if (IS_CMD("cent")) {
		    fscanf(f, "%d\n", &(mst.center_id));

		} else if (IS_CMD("frce")) {
		    int which, temp;

		    fscanf(f, "%d", &which);
		    if (which >= 0 && which < BF_NUM) {
			fscanf(f, "%d %lf %lf\n", &temp, &(mst.cur_grav_val[which]), &(mst.cur_misc_val[which]));

			activate_mbutton(&(mst.bf_mode[which]), temp);
		    }
		} else if (IS_CMD("visc")) {
		    fscanf(f, "%lf\n", &(mst.cur_visc));
		} else if (IS_CMD("stck")) {
		    fscanf(f, "%lf\n", &(mst.cur_stick));
		} else if (IS_CMD("step")) {
		    fscanf(f, "%lf\n", &(mst.cur_dt));
		} else if (IS_CMD("prec")) {
		    fscanf(f, "%lf\n", &(mst.cur_prec));
		} else if (IS_CMD("adpt")) {
		    fscanf(f, "%d\n", &temp);
		    mst.adaptive_step = temp ? TRUE : FALSE;

		} else if (IS_CMD("gsnp")) {
		    fscanf(f, "%lf %d\n", &(mst.cur_gsnap), &temp);
		    mst.grid_snap = temp ? TRUE : FALSE;
		} else if (IS_CMD("wall")) {
		    int wt, wl, wr, wb;
		    fscanf(f, "%d %d %d %d\n", &wt, &wl, &wr, &wb);
		    mst.w_top = (boolean)wt;
		    mst.w_left = (boolean)wl;
		    mst.w_right = (boolean)wr;
		    mst.w_bottom = (boolean)wb;
		} else {
		    /* unknown command */
		    fprintf(stderr, "Unknown command: %4.4s\n", cmd);
		    skip_to_eol(f);
		}
	    }

	    /* Connect springs to masses */
	    for (i = spring_start; i < num_spring; i++) {
		int j;
		boolean m1done, m2done;

		m1done = m2done = FALSE;

		if (i == fake_spring)
		  continue;

		for (j = 0; (!m1done || !m2done) && j < num_map; j++) {
		    if (!m1done && springs[i].m1 == mapfrom[j]) {
			springs[i].m1 = mapto[j];
			m1done = TRUE;
		    }
		    if (!m2done && springs[i].m2 == mapfrom[j]) {
			springs[i].m2 = mapto[j];
			m2done = TRUE;
		    }
		}
		if (!m1done && !m2done) {
		    /* delete spring */
		    delete_spring(i);
		    fprintf(stderr, "Spring %d not connected to existing mass\n", i);
		}
	    }

	    free(mapfrom);
	    free(mapto);
	    reconnect_masses();
	    review_system(TRUE);
	    redisplay_widgets();
	} else {
	    return FALSE;
	}

	(void)fclose(f);

	break;
      case F_SAVE:
	if ((f = fopen(extend_file(file), "w")) == NULL) {
	    return FALSE;
	}
	fprintf(f, "%s *** XSpringies data file\n", MAGIC_CMD);
	/* Settings */
	fprintf(f, "cmas %.12lg\n", mst.cur_mass);
	fprintf(f, "elas %.12lg\n", mst.cur_rest);
	fprintf(f, "kspr %.12lg\n", mst.cur_ks);
	fprintf(f, "kdmp %.12lg\n", mst.cur_kd);
	fprintf(f, "fixm %d\n", mst.fix_mass);
	fprintf(f, "shws %d\n", mst.show_spring);
	fprintf(f, "cent %d\n", mst.center_id);

	for (i = 0; i < BF_NUM; i++)
	  fprintf(f, "frce %d %d %.12lg %.12lg\n", i, (mst.bf_mode[i] >= 0) ? 1 : 0, mst.cur_grav_val[i], mst.cur_misc_val[i]);

	fprintf(f, "visc %.12lg\n", mst.cur_visc);
	fprintf(f, "stck %.12lg\n", mst.cur_stick);
	fprintf(f, "step %.12lg\n", mst.cur_dt);
	fprintf(f, "prec %.12lg\n", mst.cur_prec);
	fprintf(f, "adpt %d\n", mst.adaptive_step);

	fprintf(f, "gsnp %.12lg %d\n", mst.cur_gsnap, mst.grid_snap);
	fprintf(f, "wall %d %d %d %d\n", (int)mst.w_top, (int)mst.w_left, (int)mst.w_right, (int)mst.w_bottom);

	/* Masses and springs */
	for (i = 0; i < num_mass; i++) {
	    if (masses[i].status & S_ALIVE) {
		fprintf(f, "mass %d %.18lg %.18lg %.12lg %.12lg %.8lg %.8lg\n", i, masses[i].x, masses[i].y, masses[i].vx, masses[i].vy,
		       (masses[i].status & S_FIXED) ? -masses[i].mass : masses[i].mass, masses[i].elastic);
	    }
	}
	for (i = 0; i < num_spring; i++) {
	    if (springs[i].status & S_ALIVE) {
		fprintf(f, "spng %d %d %d %.8lg %.8lg %.18lg\n", i, springs[i].m1, springs[i].m2,
			springs[i].ks, springs[i].kd, springs[i].restlen);
	    }
	}

	if (fclose(f) == EOF)
	  return FALSE;
	break;
    }

    return TRUE;
}