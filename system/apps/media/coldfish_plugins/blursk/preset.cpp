/* preset.c */

/*  Blursk - visualization plugin for XMMS
 *  Copyright (C) 1999  Steve Kirkendall
 *
 *  Portions of this file are derived from the XMMS "Blur Scope" plugin.
 *  XMMS is Copyright (C) 1998-1999  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "xmms/configfile.h"
#include "blursk.h"

/* These are used for storing info about presets */
typedef struct preset_s
{
	struct preset_s	*next;	/* some other preset, or NULL */
	gchar		*title;	/* name of this preset */
	BlurskConfig	conf;	/* settings of this preset */
} preset_t;

/* This is a special name for a pseudo-preset which, when selected, causes
 * one of the real presets to be chosen randomly at appropriate times.
 */
static char random_name[] = "Random preset on quiet";

/* These are the interesting items in the configuration window */
static GtkWidget *box;		/* contains Preset portion of config window */
static GtkWidget *combo;	/* a combo box, stores name of a preset */
static GtkWidget *load;		/* the [Load] button */
static GtkWidget *save;		/* the [Save] button */
static GtkWidget *erase;	/* the [Erase] button */

/* This stores a list of the presets */
static preset_t	*preset_list;
static GList	*name_list;

/* This is the number of presets in the list.  Since presets are always given
 * consecutive IDs starting at 0, this is also the ID to be assigned to the
 * next preset that we create.
 */
static int preset_qty;
static int prevqty;

/*----------------------------------------------------------------------------*/
/* MISCELLANEOUS UTILITY FUNCTIONS                                            */

/* Compare two configurations.  Return 0 if identical, or a positive number
 * if different.  The magnitude of the number indicates how much they differ.
 */
static gint preset_diff(BlurskConfig *conf1, BlurskConfig *conf2)
{
	gint	diff;

	/* Compare everything from the normal Configure window.  Count the
	 * number of fields that are different... but make some weigh in as
	 * being more important than others.
	 */
	diff = 0;
	if (!conf1->hue_on_beats && ((conf1->color ^ conf2->color) & 0x00fcfcfc) != 0)
		diff++;
	if (strcmp(conf1->color_style, conf2->color_style))
		diff++;
	if (strcmp(conf1->fade_speed, conf2->fade_speed))
		diff++;
	if (strcmp(conf1->signal_color, conf2->signal_color))
		diff++;
	if (conf1->contour_lines != conf2->contour_lines)
		diff++;
	if (conf1->hue_on_beats != conf2->hue_on_beats)
		diff++;
	if (strcmp(conf1->background, conf2->background))
		diff++;
	if (strcmp(conf1->blur_style, conf2->blur_style))
		diff += 3;
	if (strcmp(conf1->transition_speed, conf2->transition_speed))
		diff++;
	if (strcmp(conf1->blur_when, conf2->blur_when))
		diff += 2;
	if (strcmp(conf1->blur_stencil, conf2->blur_stencil))
		diff++;
	if (conf1->slow_motion != conf2->slow_motion)
		diff++;
	if (strcmp(conf1->signal_style, conf2->signal_style))
		diff += 2;
	if (strcmp(conf1->plot_style, conf2->plot_style))
		diff += 2;
	if (conf1->thick_on_beats != conf2->thick_on_beats)
		diff++;
	if (strcmp(conf1->flash_style, conf2->flash_style))
		diff++;
	if (strcmp(conf1->overall_effect, conf2->overall_effect))
		diff += 2;
	if (strcmp(conf1->floaters, conf2->floaters))
		diff++;
	return diff;
}

/* Return a pointer to a preset's struct, or NULL if there is no preset with
 * the given name.  If lagref is non-NULL, then also set it to the preset
 * before the current item, or to NULL if it is the first.
 */
static preset_t *preset_find(char *title, preset_t **lagref)
{
	preset_t	*scan, *lag;

	/* Search for the preset */
	for (scan = preset_list, lag = NULL;
	     scan && strcasecmp(scan->title, title);
	     lag = scan, scan = scan->next)
	{
	}

	/* if lagref is non-NULL, then store the lag pointer at *lagref */
	if (lagref)
		*lagref = lag;

	/* return the found preset or NULL */
	return scan;
}


/* Read the presets from the "blursk-presets" file */
void preset_read(void)
{
	gchar		*filename;
	static int	did_once = FALSE;
	char		*str;
	preset_t	*item, *scan, *lag;
	FILE		*fp;
	char		linebuf[1024];

	/* if we already did this, then don't do it again */
	if (did_once)
		return;
	did_once = TRUE;

	/* scan the "blursk-presets" file for section names (preset names) */
	filename = g_strconcat(g_get_home_dir(), "/.xmms/blursk-presets", NULL);
	fp = fopen(filename, "r");
	while (fp && fgets(linebuf, sizeof linebuf, fp))
	{
		/* skip if not a section name */
		if (linebuf[0] != '[' || (str = strchr(linebuf, ']')) == NULL)
			continue;
		*str = '\0';

		/* allocate a struct for storing this info */
		item = (preset_t *)malloc(sizeof(preset_t));
		item->title = g_strdup(linebuf + 1);

		/* add this to the list of presets, keeping the
		 * list sorted by title.
		 */
		for (scan = preset_list, lag = NULL;
		     scan && strcasecmp(scan->title, item->title) < 0;
		     lag = scan, scan = scan->next)
		{
		}
		item->next = scan;
		if (lag)
			lag->next = item;
		else
			preset_list = item;

		/* increment the number of presets */
		preset_qty++;
	}
	if (fp)
		fclose(fp);

	/* Read the configuration data for each preset */
	for (item = preset_list; item; item = item->next)
	{
		config_read(item->title, &item->conf);
	}
}

/* Save a preset to the "blursk-presets" file, or if item==NULL then save all
 * items into "blursk-presets".  (The latter method is used when deleting a
 * preset -- After deleting it from RAM, it writes all others out to the file.
 */
static void preset_write(preset_t *item)
{
	gint force;

	/* were we given a specific item to save? */
	if (item)
	{
		/* save this item to the "blursk-presets" file */
		config_write(FALSE, item->title, &item->conf);
	}
	else
	{
		/* save all items to the "blursk-presets" file.  For the first
		 * item only, call it with the "force" parameter set to TRUE
		 * so that the file's old contents are discarded.
		 */
		for (force = TRUE, item = preset_list;
		     item;
		     force = FALSE, item = item->next)
		{
			config_write(force, item->title, &item->conf);
		}
	}
}

/*----------------------------------------------------------------------------*/
/* FUNCTIONS FOR UPDATING WIDGET STATUS                                       */

/* Adjust all buttons.  Notice that this can be called from other modules! */
void preset_adjust(int keep_title)
{
	preset_t *scan, *best;
	gint	diff, bestdiff;
	char	*title;
	gint	anytitle, knowntitle, sameconfig, israndom;
	static int busy;

	/* If no widgets, then do nothing */
	if (!box)
		return;

	/* This function is called when the contents of the combo box is
	 * changed, and it also changes the contents itself.  This could lead
	 * to recursion, but we want to avoid that.
	 */
	if (busy)
		return;
	busy = TRUE;

	/* Has the list of presets changed? */
	if (preset_qty != prevqty)
	{
		/* Remember the qty so we can detect the next change too */
		prevqty = preset_qty;

		/* If not supposed to change title, then remember it */
		title = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)));
		/* Recreate the name list */
		if (name_list)
		{
			g_list_free(name_list);
			name_list = NULL;
		}
		for (scan = preset_list; scan; scan = scan->next)
			name_list = g_list_append(name_list, scan->title);
		g_list_append(name_list, random_name);

		/* Update the combo's history list from the name list */
		gtk_combo_set_popdown_strings(GTK_COMBO(combo), name_list);

		/* If supposed to change title, then try to find a current
		 * preset which matches the current settings */
		if (!keep_title)
		{
			best = NULL;
			for (scan = preset_list; scan; scan = scan->next)
			{
				diff = preset_diff(&config, &scan->conf);
				if (!best || bestdiff > diff)
				{
					best = scan;
					bestdiff = diff;
					if (bestdiff == 0)
						break;
				}
			}
			if (best)
				gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), best->title);
		}
		else
		{
			/* restore old title */
			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), title);
			g_free(title);
		}
	}
	else if (!keep_title)
	{
		/* If the current settings match any preset, then set combo
		 * value to its name.
		 */
		best = NULL;
		for (scan = preset_list; scan; scan = scan->next)
		{
			diff = preset_diff(&config, &scan->conf);
			if (!best || bestdiff > diff)
			{
				best = scan;
				bestdiff = diff;
				if (bestdiff == 0)

					break;
			}
		}
		if (best)
			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), best->title);
	}

	/* Set some flags, depending on the text in the combo box */
	title = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
	israndom = anytitle = knowntitle = sameconfig = FALSE;
	if (!strcasecmp(title, random_name))
	{
		/* For "Random on quiet", we want to be able to load it but not
		 * save or erase it.  This is a special case.
		 */
		israndom = knowntitle = TRUE;
	}
	else if (*title)
	{
		anytitle = TRUE;
		scan = preset_find(title, NULL);
		if (scan)
		{
			knowntitle = TRUE;
			if (0 == preset_diff(&config, &scan->conf))
				sameconfig = TRUE;
			else
				/* Apparently the user altered some property
				 * of the current preset.  This should prevent
				 * any future random changes.
				 */
				config.random_preset = FALSE;

		}
	}

	/* Adjust the buttons, depending on those flags */
	gtk_widget_set_sensitive(load, knowntitle && !sameconfig);
	gtk_widget_set_sensitive(save, anytitle && !sameconfig);
	gtk_widget_set_sensitive(erase, !israndom && knowntitle);
	busy = FALSE;
}

/*----------------------------------------------------------------------------*/
/* FUNCTIONS CALLED BY CALLBACKS                                              */

/* Load the preset that has a given title */
static void preset_load(char *title)
{
	preset_t *item;
	int	i;

	/* Try to find a preset with that name.  */
	config.random_preset = FALSE;
	if (!strcasecmp(title, random_name))
	{
		/* For "Random on quiet", randomly choose a preset */
		i = preset_qty > 0 ? rand_0_to(preset_qty) : 0;
		for (item = preset_list; --i > 0; item = item->next)
		{
		}
		config.random_preset = TRUE;
	}
	else
		/* use the named preset */
		item = preset_find(title, NULL);

	/* Load the preset (if it exists) */
	if (item)
		config_load_preset(&item->conf);
}

/* Save the current configuration under a given title */
static void preset_save(char *title)
{
	preset_t *item, *scan, *lag;
	char	*s;

	/* If invalid title, then reject it */
	for (s = title; isalnum(*s) || *s == ' ' || *s == '-' || *s == '.' || *s == '_'; s++)
	{
	}
	if (*s || !strcasecmp(title, random_name))
		return;

	/* Try to find an existing preset with that name. */
	item = preset_find(title, NULL);
	if (!item)
	{
		/* Not found!  Create a new preset and add it to the list */
		item = (preset_t *)malloc(sizeof(preset_t));
		item->title = g_strdup(title);

		/* add this to the list of presets, keeping the
		 * list sorted by title.
		 */
		for (scan = preset_list, lag = NULL;
		     scan && strcmp(scan->title, item->title) < 0;
		     lag = scan, scan = scan->next)
		{
		}
		item->next = scan;
		if (lag)
			lag->next = item;
		else
			preset_list = item;

		/* increment the number of presets */
		preset_qty++;
	}

	/* Copy the new settings into the preset */
	item->conf = config;

	/* Update the config file */
	preset_write(item);

	/* Update the buttons */
	preset_adjust(FALSE);
}

static void preset_erase(char *title)
{
	preset_t *item, *lag;

	/* Try to find the preset.  If not found, do nothing */
	item = preset_find(title, &lag);
	if (!item)
		return;

	/* Remove it from the list */
	if (lag)
		lag->next = item->next;
	else
		preset_list = item->next;


	/* Free the preset's name and struct */
	g_free(item->title);
	free(item);

	/* Adjust the item count */
	preset_qty--;

	/* Save all presets except the one we just deleted.  This is the only
	 * way to delete a whole section from an XMMS config file.
	 */
	preset_write(NULL);

	/* Adjust the buttons */
	preset_adjust(FALSE);
}

/*----------------------------------------------------------------------------*/
/* CALLBACKS AND SIMILAR FUNCTIONS                                            */

/* Called when the user enters a value into the combo box */
static void combo_cb(GtkWidget *w, gpointer data)
{
	/* Adjust the buttons, but not the contents of the combo box */
	preset_adjust(TRUE);
}

/* Called when the user presses [Load], [Save], or [Erase] */
static void button_cb(GtkWidget *w, gpointer data)
{
	char	*title;

	/* Get the text from the combo box */
	title = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));

	/* Do the button's action */
	if (w == load)
		preset_load(title);
	else if (w == save)
		preset_save(title);
	else if (w == erase)
		preset_erase(title);
}

/*----------------------------------------------------------------------------*/
/* THE PRIMARY INTERFACE FUNCTIONS                                            */

/* Create the widgets that will be shown on the Config dialog, and also perform
 * any other loading operations necessary.
 */
GtkWidget *preset_init(void)
{
	GtkWidget *label;

	/* Read the presets, unless we've already read them */
	preset_read();

	/* If the widgets already exist, then don't create them */
	if (box)
	{
		/* but do adjust their states */
		preset_adjust(FALSE);
		return box;
	}

	/* Create the container */
	box = gtk_hbox_new(FALSE, 0);
	/*gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);*/
        gtk_widget_show(box);

	/* Add a label for the combo box */
	label = gtk_label_new("Title: ");
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	/* Add a combo box */
	combo = gtk_combo_new();
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", GTK_SIGNAL_FUNC(combo_cb), NULL);
	gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 0);
	gtk_widget_show(combo);

	/* Add a small label */
	label = gtk_label_new("   ");
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	/* Add the [Load] button */
	load = gtk_button_new_with_label("Load");
	gtk_signal_connect(GTK_OBJECT(load), "clicked", GTK_SIGNAL_FUNC(button_cb), NULL);
	gtk_box_pack_start(GTK_BOX(box), load, TRUE, TRUE, 0);
	gtk_widget_show(load);

	/* Add the [Save] button */
	save = gtk_button_new_with_label("Save");
	gtk_signal_connect(GTK_OBJECT(save), "clicked", GTK_SIGNAL_FUNC(button_cb), NULL);
	gtk_box_pack_start(GTK_BOX(box), save, TRUE, TRUE, 0);
	gtk_widget_show(save);

	/* Add the [Erase] button */
	erase = gtk_button_new_with_label("Erase");
	gtk_signal_connect(GTK_OBJECT(erase), "clicked", GTK_SIGNAL_FUNC(button_cb), NULL);
	gtk_box_pack_start(GTK_BOX(box), erase, TRUE, TRUE, 0);
	gtk_widget_show(erase);

	/* Adjust the states of all controls */
	preset_adjust(FALSE);

	/* Return the container */
	return box;
}

/* Clean up when the configure dialog goes away */
void preset_term(void)
{
	/* Widgets should have already been destroyed by now, so... */
	box = NULL;

	/* The name list also magically disappears.  Remember that */
	prevqty = 0;
}

/* This function is called from blursk.c whenever the input sound is quiet.
 * This gives the "Random on quiet" pseudo-preset a chance to change.
 */
void preset_quiet(void)
{
	if (config.random_preset)
		preset_load(random_name);
}


/* Return the contents of the "Preset" combo box.  Note that this isn't
 * necessarily the name of the currently loaded configuration, though!
 */
char *preset_gettitle(void)
{
	if (box)
		return gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
	else
		return NULL;
}
