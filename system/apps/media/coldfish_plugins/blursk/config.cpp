/* config.c */

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
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "blursk.h"
#include "config.h"

/* These are the default string values.  We store these in explicit character
 * arrays so we can detect whether a config's value is dynamically allocated
 * or not.  This is important when saving presets.
 */
char config_default_color_style[] = "Flame";
char config_default_signal_color[] = "Normal signal";
char config_default_background[] = "Black bkgnd";
char config_default_blur_style[] = "Random";
char config_default_transition_speed[] = "Medium switch";
char config_default_fade_speed[] = "Slow fade";
char config_default_blur_when[] = "Full blur";
char config_default_blur_stencil[] = "No stencil";
char config_default_signal_style[] = "Oscilloscope";
char config_default_plot_style[] = "Line";
char config_default_flash_style[] = "No flash";
char config_default_overall_effect[] = "Normal effect";
char config_default_floaters[] = "No floaters";
char config_default_cpu_speed[] = "Fast CPU";
char config_default_show_info[] = "Always show info";
char config_default_fullscreen_method[] = "None";
#if 0
/* These are the interesting items in the configuration window */
GtkWidget *config_win = NULL;
static GtkWidget *vbox;
static GtkWidget *hbox;
static GtkWidget *options_colorpicker;
static GtkWidget *options_color_style;
static GtkWidget *options_signal_color;
static GtkWidget *options_contour_lines;
static GtkWidget *options_hue_on_beats;
static GtkWidget *options_background;
static GtkWidget *options_blur_style;
static GtkWidget *options_transition_speed;
static GtkWidget *options_fade_speed;
static GtkWidget *options_blur_when;
static GtkWidget *options_blur_stencil;
static GtkWidget *options_slow_motion;
static GtkWidget *options_signal_style;
static GtkWidget *options_plot_style;
static GtkWidget *options_thick_on_beats;
static GtkWidget *options_flash_style;
static GtkWidget *options_floaters;
static GtkWidget *options_overall_effect;
static GtkWidget *bbox, *copybutton, *advanced, *ok, *cancel;

GtkWidget *advanced_win = NULL;
GtkWidget *avbox;
static GtkWidget *options_cpu_speed;
static GtkWidget *options_window_title;
static GtkWidget *options_show_info;
static GtkObject *options_beat_sensitivity;
static GtkWidget *options_beat_hscale;
static GtkWidget *options_fullscreen_method;
static GtkWidget *options_fullscreen_shm;
static GtkWidget *options_fullscreen_yuv709;
static GtkWidget *options_fullscreen_root;
static GtkWidget *options_fullscreen_edges;
static GtkWidget *options_fullscreen_revert;
static GtkWidget *abbox, *aok, *acancel;
#endif
/* This stores the original configuration, so it can be restored when the
 * user hits the [Cancel] button.
 */
static BlurskConfig oldconfig, oldadvanced;

/* This stores the current configuration */
BlurskConfig config;
#if 0
/* This creates a widget that lets you pick one item from a hardcoded list */
static GtkWidget *gtkhelp_oneof(
	GtkSignalFunc callback,	/* called whenever an item is selected */
	char *(*namefunc)(int),	/* called to generate names of items */
	char *initial,		/* initial value to show */
	...)			/* NULL-terminated list of hardcoded items */
{
	GtkWidget *blist, *menu, *item;
	va_list	ap;
	gint	  i, init;
	char	*value;

	/* Create an empty menu widget */
	menu = gtk_menu_new();

	/* Also create the blist that the menu will go into */
	blist = gtk_option_menu_new();

	/* If there is a namefunc, then call it to get the first value */
	va_start(ap, initial);
	i = 0;
	value = namefunc ? (*namefunc)(i) : NULL;
	if (!value)
	{
		namefunc = NULL;
		value = va_arg(ap, char *);
	}

	/* For each arg... */
	for (i = init = 0; value; )
	{
		/* Create a menu item with the given label, and add it to
		 * the menu.
		 */
		item = gtk_menu_item_new_with_label(value);
		gtk_object_set_data(GTK_OBJECT(item), "cmd", (gpointer)value);
		gtk_widget_show(item);
		gtk_menu_append(GTK_MENU(menu), item);

		/* Arrange for the callback to be called when this item is
		 * selected.
		 */
		gtk_signal_connect(GTK_OBJECT(item), "activate", callback, (gpointer)blist);

		/* if this is the initial value, remember that. */
		if (!strcmp(value, initial))
			init = i;
	
		/* get the next value, from either the function or args */
		i++;
		value = namefunc ? (*namefunc)(i) : NULL;
		if (!value)
		{
			namefunc = NULL;
			value = va_arg(ap, char *);
		}
	}
	va_end(ap);

	/* Stick the menu in a visible widget.  I'm not really sure why this
	 * step is necessary, but without it the picker never shows up.
	 */
	gtk_widget_set_usize(blist, 120, -1);
	gtk_option_menu_remove_menu(GTK_OPTION_MENU(blist));
	gtk_option_menu_set_menu(GTK_OPTION_MENU(blist), menu);
	gtk_object_set_data(GTK_OBJECT(blist), "menu", (gpointer)menu);
	gtk_widget_show(blist);

	/* Set the initial value */
	if (init >= 0)
		gtk_option_menu_set_history(GTK_OPTION_MENU(blist), init);

	/* return the menu */
	return blist;
}

/* Return the name of the active item in an options menu */
static char *gtkhelp_get(GtkWidget *blist)
{
	GtkWidget *menu, *item;
	char	*value;

	/* Get the option menu's internal real menu */
	menu = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(blist), "menu");

	/* Get the menu's active item */
	item = gtk_menu_get_active(GTK_MENU(menu));

	/* Get the item's label, and return it */
	value = (char *)gtk_object_get_data(GTK_OBJECT(item), "cmd");

	return value;
}


/* Set a menu's current value by name.  Returns local copy of string (the copy
 * used inside Gtk itself, instead of the copy passed as "initial" arg).
 */
static char *gtkhelp_set(
	GtkWidget *blist,	/* the menu to be changed */
	char *(*namefunc)(int),	/* called to generate names of items */
	char *initial,		/* initial value to show */
	...)			/* NULL-terminated list of hardcoded items */
{
	GtkWidget *menu;
	va_list	ap;
	gint	  i, init;
	char	*value, *initvalue;

	/* Get the option menu's internal real menu */
	menu = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(blist), "menu");

	/* If there is a namefunc, then call it to get the first value */
	va_start(ap, initial);
	i = 0;
	value = namefunc ? (*namefunc)(i) : NULL;
	if (!value)
	{
		namefunc = NULL;
		value = va_arg(ap, char *);
	}

	/* Remember the first valid value.  If the "initial" arg turns out to
	 * be invalid, then this will allow us to return a valid value.
	 */
	initvalue = value;

	/* For each arg... */
	for (i = init = 0; value; )
	{
		/* if this is the initial value, remember that. */
		if (!strcmp(value, initial))
			init = i, initvalue = value;
	
		/* get the next value, from either the function or args */
		i++;
		value = namefunc ? (*namefunc)(i) : NULL;
		if (!value)
		{
			namefunc = NULL;
			value = va_arg(ap, char *);
		}
	}
	va_end(ap);

	/* Set the initial value */
	if (init >= 0)
		gtk_option_menu_set_history(GTK_OPTION_MENU(blist), init);

	/* Return the initial value. */
	return initvalue;
}

/* Create a frame within a given container.  Give the frame a label.  Inside
 * the frame, create a vbox.  Give everything reasonable border widths.
 * Show them.  Return the vbox, so we can stuff other things inside it.
 */
static GtkWidget *gtkhelp_frame(GtkWidget *container, char *label)
{
	GtkWidget *frame, *vbox;

	/* Create the frame, and set its border width */
	frame = gtk_frame_new(label);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 5);

	/* Create the vbox, and set its border width */
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	/* Put the vbox inside frame, and frame inside container */
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_add(GTK_CONTAINER(container), frame);

	/* Show vbox and frame */
        gtk_widget_show(vbox);
        gtk_widget_show(frame);

	/* return the vbox */
	return vbox;
}

/* This is called when the user hits the "Advanced" [Okay] button */
static void aok_cb(GtkWidget *w, gpointer data)
{
	/* Save the settings */
	config_write(TRUE, NULL, NULL);

	/* cleanup Gtk */
	gtk_widget_destroy(advanced_win);
	advanced_win = NULL;
}

/* This is called when the user hits the "Advanced" [Cancel] button */
static void acancel_cb(GtkWidget *w, gpointer data)
{
	BlurskConfig *old = data;

	config.cpu_speed = old->cpu_speed;
	config.window_title = old->window_title;
	config.show_info = old->show_info;
	config.beat_sensitivity = old->beat_sensitivity;
	config.fullscreen_method = old->fullscreen_method;
	config.fullscreen_shm = old->fullscreen_shm;
	config.fullscreen_root = old->fullscreen_root;
	config.fullscreen_edges = old->fullscreen_edges;
	config.fullscreen_revert = old->fullscreen_revert;
	img_resize(img_physwidth, img_physheight);
	gtk_widget_destroy(advanced_win);
	advanced_win = NULL;
}

/* This adjusts the sensitivity of the toggle widgets on the Advanced dialog */
static void adjust_fullscreen_flags()
{
	gboolean shm, root;

	/* The flags depend on the fullscreen_method option */
	if (!strncmp(config.fullscreen_method, "Use XV", 6))
	{
		shm = root = TRUE;
	}
	else
	{
		shm = root = FALSE;
	}

	/* Update the sensitivities now */
	gtk_widget_set_sensitive(options_fullscreen_shm, shm);
	gtk_widget_set_sensitive(options_fullscreen_yuv709, shm);
	gtk_widget_set_sensitive(options_fullscreen_root, root);
	gtk_widget_set_sensitive(options_fullscreen_edges, root);
}

/* This is called when one of the fullscreen options is changed */
static void fullscreen_cb(GtkWidget *w, gpointer data)
{
	config.fullscreen_method = gtkhelp_get(options_fullscreen_method);
	config.fullscreen_shm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_fullscreen_shm));
	config.fullscreen_yuv709 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_fullscreen_yuv709));
	config.fullscreen_root = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_fullscreen_root));
	config.fullscreen_edges = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_fullscreen_edges));
	config.fullscreen_revert = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_fullscreen_revert));

	adjust_fullscreen_flags();
}

/* This is called when the user hits the [Copy] button */
static void copy_cb(GtkWidget *w, gpointer data)
{
	gint	status;

	/* claim the selection */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
		status = gtk_selection_owner_set(w, GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
	else
	{
		gtk_selection_owner_set(NULL, GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
		status = FALSE;
	}
	if (!status)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FALSE);
}

/* This is called when Blursk loses ownership of the selection */
static void selection_clear_cb(GtkWidget *w, GdkEventSelection *event, gpointer data)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FALSE);
}

/* This is called when Blursk receives a request for the selection */
static void selection_get_cb(GtkWidget *w, GtkSelectionData *data, guint info, guint when)
{
	guchar	*string;
	gint	len;

	string = (guchar *)paste_genstring();
	len = strlen((char *)string);
	gtk_selection_data_set(data, GDK_SELECTION_TYPE_STRING, 8, string, len);
}

/* This is called when the user hits the [Advanced] button */
static void advanced_cb(GtkWidget *w, gpointer data)
{
	config_advanced();
}

/* This is called when the user hits the [Okay] button */
static void ok_cb(GtkWidget *w, gpointer data)
{
	/* Save the settings */
	config_write(TRUE, NULL, NULL);

	/* cleanup Gtk */
	gtk_widget_destroy(config_win);
	config_win = NULL;
	preset_term();
}

static void cancel_cb(GtkWidget *w, gpointer data)
{
	config = *(BlurskConfig *)data;
	color_genmap(FALSE);
	blursk_genrender();
	img_resize(img_physwidth, img_physheight);
	gtk_widget_destroy(config_win);
	config_win = NULL;
	preset_term();
}

/* This callback is called for options which affect the colormap */
static void color_cb(GtkWidget *w, gpointer data)
{
	gdouble color[3]; 
	GtkWidget *widget = (GtkWidget *)data;

	/* fetch the color */
	gtk_color_selection_get_color(GTK_COLOR_SELECTION(options_colorpicker), color);
	config.color = ((guint32)(255.0*color[0])<<16) |
		              ((guint32)(255.0*color[1])<<8) |
		              ((guint32)(255.0*color[2]));

	/* fetch the other options which affect the colormap */
	config.color_style = gtkhelp_get(options_color_style);
	config.signal_color = gtkhelp_get(options_signal_color);
	config.contour_lines = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_contour_lines));
	config.background = gtkhelp_get(options_background);

	/* Regenerate the colormap.  Disable "Random" changes unless the user
	 * has just now set the color_style or background.
	 */
	color_genmap(widget == options_color_style || widget == options_background);

	/* Update the preset buttons */
	preset_adjust(FALSE);
}

/* This callback is called for options which affect the image buffer sizes */
static void imgsize_cb(GtkWidget *w, gpointer data)
{
	/* fetch options which affect the choice of renderer */
	config.cpu_speed = gtkhelp_get(options_cpu_speed);

	/* regenerate the renderer specs */
	img_resize(img_physwidth, img_physheight);
}

/* This callback is called for the "window title" and "show info" options */
static void misc_cb(GtkWidget *w, gpointer data)
{
	config.window_title = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_window_title));
	config.show_info = gtkhelp_get(options_show_info);
}

/* This callback is called for options which affect the signal source */
static void renderer_cb(GtkWidget *w, gpointer data)
{
	/* fetch options which affect the choice of renderer */
	config.signal_style = gtkhelp_get(options_signal_style);

	/* regenerate the renderer specs */
	blursk_genrender();

	/* Update the preset buttons */
	preset_adjust(FALSE);
}

/* This callback handles the overall_effect option.  It may affect colors */
static void effect_cb(GtkWidget *w, gpointer data)
{
	int	code;

	/* Get the current state of the bump effect flag */
	config.overall_effect = gtkhelp_get(options_overall_effect);

	/* If overall_effect=bump effect, then make sure the color style
	 * is something that makes sense.
	 */
	if (!strcmp(config.overall_effect, "Bump effect"))
	{
		/* Assume color_name(0) is "Dimming" */
		code = color_good_for_bump(config.color_style);
		config.color_style = color_name(code);
		gtk_option_menu_set_history(GTK_OPTION_MENU(options_color_style), code);
		color_genmap(FALSE);
	}

	/* Update the preset buttons */
	preset_adjust(FALSE);
}

/* This callback is called for other options -- things which don't affect the
 * colormap or signal source.
 */
static void other_cb(GtkWidget *w, gpointer data)
{
	/* fetch options which don't affect the colormap or renderer */
	config.hue_on_beats = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_hue_on_beats));
	config.blur_style = gtkhelp_get(options_blur_style);
	config.transition_speed = gtkhelp_get(options_transition_speed);
	config.fade_speed = gtkhelp_get(options_fade_speed);
	config.blur_when = gtkhelp_get(options_blur_when);
	config.blur_stencil = gtkhelp_get(options_blur_stencil);
	config.slow_motion = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_slow_motion));
	config.plot_style = gtkhelp_get(options_plot_style);
	config.thick_on_beats = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(options_thick_on_beats));
	config.flash_style = gtkhelp_get(options_flash_style);
	config.floaters = gtkhelp_get(options_floaters);

	/* Update the preset buttons */
	preset_adjust(FALSE);
}

/* This is called for changing sliders (Adjustments) */
static void slider_cb(GtkAdjustment *adjustment, gpointer data)
{
	config.beat_sensitivity = (gint32)adjustment->value;
}
#endif
/* Change the base color. */
void config_load_color(guint32 color)
{
	gdouble rgb[3];

	/* Copy the new color into config.  However, we won't bother to store
	 * the new color into the XMMS config file right away -- this function
	 * is called mostly from color_beat(), which happens very often.  The
	 * color will be saved whenever the user changes anything else and hits
	 * the [Ok] button.
	 */
	config.color = color;
#if 0
	/* If the config window is shown, then update the colorpicker */
	if (config_win)
	{
		rgb[0]=((gdouble)(config.color / 0x10000)) / 256;
		rgb[1]=((gdouble)((config.color % 0x10000) / 0x100)) / 256;
		rgb[2]=((gdouble)(config.color % 0x100)) / 256;
		gtk_color_selection_set_color(GTK_COLOR_SELECTION(options_colorpicker), rgb);
	}
#endif
	/* Regenerate the colormap, using the new color */
	color_genmap(FALSE);
}
#if 0

/* Copy a configuration into the current options, and update the dialog to
 * reflect the new values.
 */
void config_load_preset(BlurskConfig *conf)
{
	/* Copy the settings into the current config except for a few hidden
	 * or "advanced" options.
	 */
	conf->x = config.x;
	conf->y = config.y;
	conf->width = config.width;
	conf->height = config.height;
	conf->cpu_speed = config.cpu_speed;
	conf->window_title = config.window_title;
	conf->show_info = config.show_info;
	conf->beat_sensitivity = config.beat_sensitivity;
	conf->fullscreen_method = config.fullscreen_method;
	conf->fullscreen_shm = config.fullscreen_shm;
	conf->fullscreen_root = config.fullscreen_root;
	conf->fullscreen_edges = config.fullscreen_edges;
	conf->fullscreen_yuv709 = config.fullscreen_yuv709;
	conf->fullscreen_revert = config.fullscreen_revert;
	conf->fullscreen_desired = config.fullscreen_desired;
	conf->random_preset = config.random_preset;
	config = *conf;

	/* Some options require function calls in order to take effect */
	config_load_color(config.color);
	blursk_genrender();
	color_genmap(FALSE);

	/* If the configuration dialog isn't shown, we're done! */
	if (!config_win)
		return;

	/* Set each widget to reflect the new settings */

	/* ... the "one of" menus (also use local copies of strings) */
	config.color_style = gtkhelp_set(options_color_style, color_name,
					conf->color_style, NULL);
	config.signal_color = gtkhelp_set(options_signal_color, NULL,
					conf->signal_color, "Normal signal",
					"White signal", "Cycling signal", NULL);
	config.fade_speed = gtkhelp_set(options_fade_speed, NULL,
					conf->fade_speed, "No fade",
					"Slow fade", "Medium fade",
					"Fast fade", NULL);
	config.background = gtkhelp_set(options_background,
					color_background_name, conf->background,
					NULL);
	config.blur_style = gtkhelp_set(options_blur_style, blur_name,
					conf->blur_style, NULL);
	config.transition_speed = gtkhelp_set(options_transition_speed, NULL,
					conf->transition_speed, "Slow switch",
					"Medium switch", "Fast switch", NULL);
	config.blur_when = gtkhelp_set(options_blur_when, blur_when_name,
					conf->blur_when, NULL);
	config.blur_stencil = gtkhelp_set(options_blur_stencil,
					bitmap_stencil_name, conf->blur_stencil,
					NULL);
	config.signal_style = gtkhelp_set(options_signal_style, blursk_name,
					conf->signal_style, NULL);
	config.plot_style = gtkhelp_set(options_plot_style, render_plotname,
					conf->plot_style, NULL);
	gtkhelp_set(options_flash_style, bitmap_flash_name, conf->flash_style, NULL);
	gtkhelp_set(options_overall_effect, NULL, conf->overall_effect, "Normal effect", "Bump effect", "Anti-fade effect", "Ripple effect", NULL);
	gtkhelp_set(options_floaters, blursk_floater_name, conf->floaters, NULL);

	/* ... the booleans */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_contour_lines),
							conf->contour_lines);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_hue_on_beats),
							conf->hue_on_beats);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_slow_motion),
							conf->slow_motion);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_thick_on_beats),
							conf->thick_on_beats);

	/* During some of the above changes, the config options may have
	 * briefly been changed so they didn't match the preset.  That should
	 * be corrected by the time we get here, but the intermediate calls
	 * to preset_adjust could have turned off a "Random preset on quiet"
	 * flag.  We don't want that to happen, so set it again now!  (We must
	 * do this immediately before calling preset_adjust(), below.)
	 */
	config.random_preset = conf->random_preset;

	/* ... the "Preset" buttons */
	preset_adjust(FALSE);
}

/* Display a dialog for configuring the graphics */
void config_dialog(void)
{
	gdouble color[3];
	GtkWidget	*inside;

	/* If already configuring, then do nothing here */
	if (config_win)
		return;

	/* read the current configuration */
	config_read(NULL, NULL);
	color[0]=((gdouble)(config.color /0x10000))/256;
	color[1]=((gdouble)((config.color %0x10000)/0x100))/256;
	color[2]=((gdouble)(config.color %0x100))/256;

	/* save the settings, in case the user hits [Cance] */
	oldconfig = config;

	/* Create the configure window */
	config_win = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_container_set_border_width(GTK_CONTAINER(config_win), 10);
	gtk_window_set_title(GTK_WINDOW(config_win), "Blursk Configuration");
	gtk_window_set_policy(GTK_WINDOW(config_win), FALSE, FALSE, FALSE);
#if GTK_CHECK_VERSION(1,2,6)
	gtk_window_set_position(GTK_WINDOW(config_win), GTK_WIN_POS_CENTER);
#endif
	gtk_signal_connect(GTK_OBJECT(config_win), "destroy",
		GTK_SIGNAL_FUNC(gtk_widget_destroyed), &config_win);
	gtk_signal_connect(GTK_OBJECT(config_win), "delete_event",
		GTK_SIGNAL_FUNC(ok_cb), NULL);

	vbox = gtk_vbox_new(FALSE, 5);

	/* Place the "Presets" frame at the top of the vbox */
	inside = gtkhelp_frame(vbox, "Presets");
	gtk_box_pack_start(GTK_BOX(inside), preset_init(), FALSE, FALSE, 0);
	
	/* Add a "Base color" frame inside the vbox */
	inside = gtkhelp_frame(vbox, "Base color");

	/* Place a colorpicker inside the "Base color" vbox */
	options_colorpicker = gtk_color_selection_new();
	gtk_color_selection_set_color(GTK_COLOR_SELECTION(options_colorpicker), color);
	gtk_signal_connect(GTK_OBJECT(options_colorpicker), "color_changed", GTK_SIGNAL_FUNC(color_cb), NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_colorpicker, FALSE, FALSE, 0);
        gtk_widget_show(options_colorpicker);

	/* Add an hbox to the outer vbox, so we can get multiple columns */
	hbox = gtk_hbox_new(FALSE, 0);
	/*gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);*/
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_widget_show(hbox);

	/* Add a "Color options" vbox inside the hbox */
	inside = gtkhelp_frame(hbox, "Color options");

	/* Add the color options inside the "Color options" vbox */
	options_color_style = gtkhelp_oneof(GTK_SIGNAL_FUNC(color_cb),
		color_name, config.color_style, NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_color_style, FALSE, FALSE, 0);

	options_fade_speed = gtkhelp_oneof(GTK_SIGNAL_FUNC(other_cb), NULL,
		config.fade_speed, "No fade", "Slow fade", "Medium fade",
		"Fast fade", NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_fade_speed, FALSE, FALSE, 0);

	options_signal_color = gtkhelp_oneof(GTK_SIGNAL_FUNC(color_cb), NULL,
		config.signal_color, "Normal signal", "White signal",
		"Cycling signal", NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_signal_color, FALSE, FALSE, 0);

	options_contour_lines = gtk_check_button_new_with_label("Contour lines");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_contour_lines), config.contour_lines);
	gtk_signal_connect(GTK_OBJECT(options_contour_lines), "toggled", GTK_SIGNAL_FUNC(color_cb), NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_contour_lines, FALSE, FALSE, 0);
	gtk_widget_show(options_contour_lines);

	options_hue_on_beats = gtk_check_button_new_with_label("Hue on beats");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_hue_on_beats), config.hue_on_beats);
	gtk_signal_connect(GTK_OBJECT(options_hue_on_beats), "toggled", GTK_SIGNAL_FUNC(other_cb), NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_hue_on_beats, FALSE, FALSE, 0);
	gtk_widget_show(options_hue_on_beats);

	options_background = gtkhelp_oneof(GTK_SIGNAL_FUNC(color_cb),
		color_background_name, config.background, NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_background, FALSE, FALSE, 0);

	/* Add a "Blur options" vbox inside the hbox */
	inside = gtkhelp_frame(hbox, "Blur options");

	options_blur_style = gtkhelp_oneof(GTK_SIGNAL_FUNC(other_cb), blur_name,
		config.blur_style, NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_blur_style, FALSE, FALSE, 0);

	options_transition_speed = gtkhelp_oneof(GTK_SIGNAL_FUNC(other_cb), NULL,
		config.transition_speed, "Slow switch",
		"Medium switch", "Fast switch", NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_transition_speed, FALSE, FALSE, 0);

	options_blur_when = gtkhelp_oneof(GTK_SIGNAL_FUNC(other_cb), blur_when_name,
		config.blur_when, NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_blur_when, FALSE, FALSE, 0);

	options_blur_stencil = gtkhelp_oneof(GTK_SIGNAL_FUNC(other_cb), bitmap_stencil_name,
		config.blur_stencil, NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_blur_stencil, FALSE, FALSE, 0);

	options_slow_motion = gtk_check_button_new_with_label("Slow motion");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_slow_motion), config.slow_motion);
	gtk_signal_connect(GTK_OBJECT(options_slow_motion), "toggled", GTK_SIGNAL_FUNC(other_cb), NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_slow_motion, FALSE, FALSE, 0);
	gtk_widget_show(options_slow_motion);

	/* Add an "Effects" vbox inside the hbox */
	inside = gtkhelp_frame(hbox, "Effects");

	options_signal_style = gtkhelp_oneof(GTK_SIGNAL_FUNC(renderer_cb),
		blursk_name, config.signal_style, NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_signal_style, FALSE, FALSE, 0);

	options_plot_style = gtkhelp_oneof(GTK_SIGNAL_FUNC(other_cb),
		render_plotname, config.plot_style, NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_plot_style, FALSE, FALSE, 0);

	options_thick_on_beats = gtk_check_button_new_with_label("Thick on beats");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_thick_on_beats), config.thick_on_beats);
	gtk_signal_connect(GTK_OBJECT(options_thick_on_beats), "toggled", GTK_SIGNAL_FUNC(other_cb), NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_thick_on_beats, FALSE, FALSE, 0);
	gtk_widget_show(options_thick_on_beats);

	options_flash_style = gtkhelp_oneof(GTK_SIGNAL_FUNC(other_cb),
		bitmap_flash_name, config.flash_style, NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_flash_style, FALSE, FALSE, 0);

	options_overall_effect = gtkhelp_oneof(GTK_SIGNAL_FUNC(effect_cb),
		NULL, config.overall_effect, "Normal effect", "Bump effect", "Anti-fade effect", "Ripple effect", NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_overall_effect, FALSE, FALSE, 0);

	options_floaters = gtkhelp_oneof(GTK_SIGNAL_FUNC(other_cb),
		blursk_floater_name, config.floaters, NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_floaters, FALSE, FALSE, 0);

	/* Add an area for [About], [Ok] and [Cancel] buttons */
	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_button_box_set_child_size(GTK_BUTTON_BOX(bbox), 75, 27);
	gtk_button_box_set_child_ipadding(GTK_BUTTON_BOX(bbox), 5, 0);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	/* Add the [Copy] button */
	copybutton = gtk_toggle_button_new_with_label("Copy");
	gtk_signal_connect(GTK_OBJECT(copybutton), "toggled",
			   GTK_SIGNAL_FUNC(copy_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(copybutton), "selection_clear_event",
			   GTK_SIGNAL_FUNC(selection_clear_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(copybutton), "selection_get",
			   GTK_SIGNAL_FUNC(selection_get_cb), NULL);
	gtk_selection_add_target(copybutton, GDK_SELECTION_PRIMARY, GDK_SELECTION_TYPE_STRING, 0);
	GTK_WIDGET_SET_FLAGS(copybutton, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), copybutton, TRUE, TRUE, 0);
	gtk_widget_show(copybutton);
	
	/* Add the [About] button */
	inside = gtk_button_new_with_label("About");
	gtk_signal_connect(GTK_OBJECT(inside), "clicked",
			   GTK_SIGNAL_FUNC(about), NULL);
	GTK_WIDGET_SET_FLAGS(inside, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), inside, TRUE, TRUE, 0);
	gtk_widget_show(inside);
	
	/* Add the [Advanced] button */
	advanced = gtk_button_new_with_label("Advanced");
	gtk_signal_connect(GTK_OBJECT(advanced), "clicked",
			   GTK_SIGNAL_FUNC(advanced_cb), NULL);
	GTK_WIDGET_SET_FLAGS(advanced, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), advanced, TRUE, TRUE, 0);
	gtk_widget_show(advanced);

	/* Add the [Ok] button */
	ok = gtk_button_new_with_label("Ok");
	gtk_signal_connect(GTK_OBJECT(ok), "clicked",
			   GTK_SIGNAL_FUNC(ok_cb), NULL);
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_widget_show(ok);
	
	/* Add the [Cancel] button */
	cancel = gtk_button_new_with_label("Cancel");
	gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
			   GTK_SIGNAL_FUNC(cancel_cb),
			   (gpointer)&oldconfig);
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);
	gtk_widget_show(cancel);
	gtk_widget_show(bbox);
	
	/* Showtime! */
	gtk_container_add(GTK_CONTAINER(config_win), vbox);
	gtk_widget_show(vbox);
	gtk_widget_show(config_win);
	gtk_widget_grab_default(ok);
}

/* Display a "Advanced" configuration dialog */
void config_advanced(void)
{
	GtkWidget	*inside;

	/* If already configuring, then do nothing here */
	if (advanced_win)
		return;

	/* save the settings, in case the user hits [Cance] */
	oldadvanced = config;

	/* Create the configure window */
	advanced_win = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_container_set_border_width(GTK_CONTAINER(advanced_win), 10);
	gtk_window_set_title(GTK_WINDOW(advanced_win), "Blursk Advanced");
	gtk_window_set_policy(GTK_WINDOW(advanced_win), FALSE, FALSE, FALSE);
#if GTK_CHECK_VERSION(1,2,6)
	gtk_window_set_position(GTK_WINDOW(advanced_win), GTK_WIN_POS_CENTER);
#endif
	gtk_signal_connect(GTK_OBJECT(advanced_win), "destroy",
		GTK_SIGNAL_FUNC(gtk_widget_destroyed), &advanced_win);
	gtk_signal_connect(GTK_OBJECT(advanced_win), "delete_event",
		GTK_SIGNAL_FUNC(aok_cb), NULL);

	avbox = gtk_vbox_new(FALSE, 5);

	/* Add a "Miscellany" frame inside the vbox */
	inside = gtkhelp_frame(avbox, "Miscellany");

	/* Place "CPU Speed" and "Show window title" inside the Miscellany box*/
	options_cpu_speed = gtkhelp_oneof(GTK_SIGNAL_FUNC(imgsize_cb),
		NULL, config.cpu_speed, "Slow CPU", "Medium CPU", "Fast CPU", NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_cpu_speed, FALSE, FALSE, 0);

	options_window_title = gtk_check_button_new_with_label("Show window title");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_window_title), config.window_title);
	gtk_signal_connect(GTK_OBJECT(options_window_title), "toggled", GTK_SIGNAL_FUNC(misc_cb), NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_window_title, FALSE, FALSE, 0);
	gtk_widget_show(options_window_title);

	options_show_info = gtkhelp_oneof(GTK_SIGNAL_FUNC(misc_cb),
		NULL, config.show_info, "Never show info", "4 seconds info", "Always show info", NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_show_info, FALSE, FALSE, 0);

	/* Add a slider for adjusting the sensitivity of the beat detector */
	inside = gtkhelp_frame(avbox, "Beat sensitivity");
	options_beat_sensitivity = gtk_adjustment_new((double)config.beat_sensitivity, 0.0, 20.0, 1.0, 1.0, 1.0);
	gtk_signal_connect(options_beat_sensitivity, "value_changed", GTK_SIGNAL_FUNC(slider_cb), NULL);
	options_beat_hscale = gtk_hscale_new(GTK_ADJUSTMENT(options_beat_sensitivity));
	gtk_scale_set_draw_value(GTK_SCALE(options_beat_hscale), FALSE);
	gtk_box_pack_start(GTK_BOX(inside), options_beat_hscale, FALSE, FALSE, 0);
	gtk_widget_show(options_beat_hscale);

	/* Add a "Full screen" frame inside the vbox */
	inside = gtkhelp_frame(avbox, "Full screen");

	/* Add the color options inside the "Color options" vbox */
	options_fullscreen_method = gtkhelp_oneof(GTK_SIGNAL_FUNC(fullscreen_cb),
		NULL, config.fullscreen_method, "Disabled", "Use XMMS",
#if HAVE_XV
		"Use XV", "Use XV doubled",
#endif
		NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_fullscreen_method, FALSE, FALSE, 0);

	options_fullscreen_shm = gtk_check_button_new_with_label("Shared memory");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_fullscreen_shm), config.fullscreen_shm);
	gtk_signal_connect(GTK_OBJECT(options_fullscreen_shm), "toggled", GTK_SIGNAL_FUNC(fullscreen_cb), NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_fullscreen_shm, FALSE, FALSE, 0);
	gtk_widget_show(options_fullscreen_shm);

	options_fullscreen_yuv709 = gtk_check_button_new_with_label("Alternative YUV");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_fullscreen_yuv709), config.fullscreen_yuv709);
	gtk_signal_connect(GTK_OBJECT(options_fullscreen_yuv709), "toggled", GTK_SIGNAL_FUNC(fullscreen_cb), NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_fullscreen_yuv709, FALSE, FALSE, 0);
	gtk_widget_show(options_fullscreen_yuv709);

	options_fullscreen_root = gtk_check_button_new_with_label("In root window");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_fullscreen_root), config.fullscreen_root);
	gtk_signal_connect(GTK_OBJECT(options_fullscreen_root), "toggled", GTK_SIGNAL_FUNC(fullscreen_cb), NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_fullscreen_root, FALSE, FALSE, 0);
	gtk_widget_show(options_fullscreen_root);

	options_fullscreen_edges = gtk_check_button_new_with_label("Mask out edges");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_fullscreen_edges), config.fullscreen_edges);
	gtk_signal_connect(GTK_OBJECT(options_fullscreen_edges), "toggled", GTK_SIGNAL_FUNC(fullscreen_cb), NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_fullscreen_edges, FALSE, FALSE, 0);
	gtk_widget_show(options_fullscreen_edges);

	options_fullscreen_revert = gtk_check_button_new_with_label("Revert to window at end");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options_fullscreen_revert), config.fullscreen_revert);
	gtk_signal_connect(GTK_OBJECT(options_fullscreen_revert), "toggled", GTK_SIGNAL_FUNC(fullscreen_cb), NULL);
	gtk_box_pack_start(GTK_BOX(inside), options_fullscreen_revert, FALSE, FALSE, 0);
	gtk_widget_show(options_fullscreen_revert);

	/* Adjust the sensitivity of the fullscreen flags, to reflect the
	 * capabilities of the current fullscreen_method.
	 */
	adjust_fullscreen_flags();

	/* Add an area for [Ok] and [Cancel] buttons */
	abbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(abbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(abbox), 5);
	gtk_box_pack_start(GTK_BOX(avbox), abbox, FALSE, FALSE, 0);

	/* Add the [Ok] button */
	aok = gtk_button_new_with_label("Ok");
	gtk_signal_connect(GTK_OBJECT(aok), "clicked",
			   GTK_SIGNAL_FUNC(aok_cb), NULL);
	GTK_WIDGET_SET_FLAGS(aok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(abbox), aok, TRUE, TRUE, 0);
	gtk_widget_show(aok);
	
	/* Add the [Cancel] button */
	acancel = gtk_button_new_with_label("Cancel");
	gtk_signal_connect(GTK_OBJECT(acancel), "clicked",
			   GTK_SIGNAL_FUNC(acancel_cb),
			   (gpointer)&oldadvanced);
	GTK_WIDGET_SET_FLAGS(acancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(abbox), acancel, TRUE, TRUE, 0);
	gtk_widget_show(acancel);
	gtk_widget_show(abbox);
	
	/* Showtime! */
	gtk_container_add(GTK_CONTAINER(advanced_win), avbox);
	gtk_widget_show(avbox);
	gtk_widget_show(advanced_win);
	gtk_widget_grab_default(aok);
}
#endif
void config_default(BlurskConfig *conf)
{
	/* window geometry -- generally ignored */
	conf->x = -1;
	conf->y = -1;
	conf->width = 256;
	conf->height = 128;

	/* main options */
	conf->color = 0x0ff20;
	conf->color_style = config_default_color_style;
	conf->signal_color = config_default_signal_color;
	conf->contour_lines = FALSE;
	conf->hue_on_beats = FALSE;
	conf->background = config_default_background;
	conf->blur_style = config_default_blur_style;
	conf->transition_speed = config_default_transition_speed;
	conf->fade_speed = config_default_fade_speed;
	conf->blur_when = config_default_blur_when;
	conf->blur_stencil = config_default_blur_stencil;
	conf->slow_motion = FALSE;
	conf->signal_style = config_default_signal_style;
	conf->plot_style = config_default_plot_style;
	conf->thick_on_beats = TRUE;
	conf->flash_style = config_default_flash_style;
	conf->overall_effect = config_default_overall_effect;
	conf->floaters = config_default_floaters;

	/* advanced options */
	conf->cpu_speed = config_default_cpu_speed;
	conf->window_title = FALSE;
	conf->show_info = config_default_show_info;
	conf->beat_sensitivity = 4;
	conf->fullscreen_method = config_default_fullscreen_method;
	conf->fullscreen_shm = TRUE;
	conf->fullscreen_yuv709 = FALSE;
	conf->fullscreen_root = FALSE;
	conf->fullscreen_edges = FALSE;
	conf->fullscreen_revert = TRUE;

	/* set implicitly via other options or commands */
	conf->fullscreen_desired = FALSE;
	conf->random_preset = FALSE;
}
#if 0
/* This is a wrapper around the xmms_cfg_read_string() function.  This version
 * replaces the dynamically-allocated string returned by xmms_cfg_read_string()
 * with a constant string declared elsewhere within Blursk.
 */
static void read_string(
	ConfigFile *cfg,	/* handle for the config file */
	gchar	*section,	/* name of section */
	gchar	*key,		/* name of option within the section */
	char	**value,	/* default value, and where to store found value */
	char	*(*namefunc)(int),/* called to generate names of items */
	...)			/* NULL-terminated list of hardcoded items */
{
	va_list	ap;
	gchar	*want;		/* value returned by xmms_cfg_read_string() */
	char	*maybe;
	gboolean found;
	int	i;

	/* Try to get the value */
	if (!xmms_cfg_read_string(cfg, section, key, &want))
		return;

	/* Got it!  Search for it in the list of valid values */
	va_start(ap, namefunc);
	i = 0;
	found = FALSE;
	maybe = namefunc ? (*namefunc)(i) : NULL;
	if (!maybe)
	{
		namefunc = NULL;
		maybe = va_arg(ap, char *);
	}
	while (maybe)
	{
		/* if this is the returned value, remember that. */
		if (!strcmp(maybe, (char *)want))
		{
			*value = maybe;
			found = TRUE;
		}
	
		/* get the next value, from either the function or args */
		i++;
		maybe = namefunc ? (*namefunc)(i) : NULL;
		if (!maybe)
		{
			namefunc = NULL;
			maybe = va_arg(ap, char *);
		}
	}
	va_end(ap);

	if (!found)
		printf("read_string() got invalid value \"%s\" for %s.%s\n",
			want, section, key);

	/* Free the string returned by xmms_cfg_read_string().  We don't
	 * need it anymore, since we have either found an identical static
	 * string, or will be keeping the default value.
	 */
	g_free(want);
}

/* Read the Blursk configuration from the XMMS configuration file */
void config_read(gchar *preset, BlurskConfig *presetconfig)
{
	ConfigFile *cfg;
	gchar *filename;
	gint	gi;
	static int did_once = FALSE;
	gchar *section_name;
	BlurskConfig *c;

	if (preset || !did_once)
	{
		/* Choose a BlurskConfig buffer and a section name */
		if (preset)
		{
			filename = g_strconcat(g_get_home_dir(),
						"/.xmms/blursk-presets", NULL);
			section_name = preset;
			c = presetconfig;
		}
		else
		{
			filename = g_strconcat(g_get_home_dir(),
						"/.xmms/config", NULL);
			section_name = "Blursk";
			c = &config;
		}

		/* set options to default values */
		config_default(c);

		/* try to read saved options */
		cfg = xmms_cfg_open_file(filename);
		if (cfg)
		{
			xmms_cfg_read_int(cfg, section_name, "x", &c->x);
			xmms_cfg_read_int(cfg, section_name, "y", &c->y);
			xmms_cfg_read_int(cfg, section_name, "width", &c->width);
			xmms_cfg_read_int(cfg, section_name, "height", &c->height);
			gi = c->color;
			xmms_cfg_read_int(cfg, section_name, "color", &gi);
			c->color = (guint32)gi;
			read_string(cfg, section_name, "color_style",
					&c->color_style, color_name, NULL);
			read_string(cfg, section_name, "signal_color",
					&c->signal_color, NULL, "Normal signal",
					"White signal", "Cycling signal", NULL);
			xmms_cfg_read_boolean(cfg, section_name,
					"contour_lines", &c->contour_lines);
			xmms_cfg_read_boolean(cfg, section_name,
					"hue_on_beats", &c->hue_on_beats);
			read_string(cfg, section_name, "background",
					&c->background, color_background_name,
					NULL);
			read_string(cfg, section_name, "blur_style",
					&c->blur_style, blur_name, NULL);
			read_string(cfg, section_name, "transition_speed",
					&c->transition_speed, NULL,
					"Slow switch", "Medium switch",
					"Fast switch", NULL);
			read_string(cfg, section_name, "blur_when",
					&c->blur_when, blur_when_name, NULL);
			read_string(cfg, section_name, "blur_stencil",
					&c->blur_stencil, bitmap_stencil_name,
					NULL);
			read_string(cfg, section_name, "fade_speed",
					&c->fade_speed, NULL, "No fade",
					"Slow fade", "Medium fade", "Fast fade",
					NULL);
			xmms_cfg_read_boolean(cfg, section_name, "slow_motion", &c->slow_motion);
			read_string(cfg, section_name, "signal_style",
					&c->signal_style, blursk_name, NULL);
			read_string(cfg, section_name, "plot_style",
					&c->plot_style, render_plotname, NULL);
			xmms_cfg_read_boolean(cfg, section_name, "thick_on_beats", &c->thick_on_beats);
			read_string(cfg, section_name, "flash_style",
					&c->flash_style, bitmap_flash_name,
					NULL);
			read_string(cfg, section_name, "overall_effect",
					&c->overall_effect, NULL,
					"Normal effect", "Bump effect",
					"Anti-fade effect", "Ripple effect",
					NULL);
			read_string(cfg, section_name, "floaters",
					&c->floaters, blursk_floater_name,NULL);
			read_string(cfg, section_name, "cpu_speed",
					&c->cpu_speed, NULL, "Slow CPU",
					"Medium CPU", "Fast CPU", NULL);
			xmms_cfg_read_boolean(cfg, section_name, "window_title", &c->window_title);
			read_string(cfg, section_name, "show_info",
					&c->show_info, NULL, "Never show info",
					"4 seconds info", "Always show info",
					NULL);
			gi = (gint)c->beat_sensitivity;
			xmms_cfg_read_int(cfg, section_name, "beat_sensitivity", &gi);
			c->beat_sensitivity = (guint32)gi;
			read_string(cfg, section_name, "fullscreen_method",
					&c->fullscreen_method, NULL,
					"Disabled", "Use XMMS",
#if HAVE_XV
					"Use XV", "Use XV doubled",
#endif
					NULL);
			xmms_cfg_read_boolean(cfg, section_name, "fullscreen_shm", &c->fullscreen_shm);
			xmms_cfg_read_boolean(cfg, section_name, "fullscreen_yuv709", &c->fullscreen_yuv709);
			xmms_cfg_read_boolean(cfg, section_name, "fullscreen_root", &c->fullscreen_root);
			xmms_cfg_read_boolean(cfg, section_name, "fullscreen_edges", &c->fullscreen_edges);
			xmms_cfg_read_boolean(cfg, section_name, "fullscreen_revert", &c->fullscreen_revert);
			xmms_cfg_read_boolean(cfg, section_name, "fullscreen_desired", &c->fullscreen_desired);
			xmms_cfg_read_boolean(cfg, section_name, "random_preset", &c->random_preset);
			xmms_cfg_free(cfg);
		}
		g_free(filename);
		if (!preset)
			did_once = TRUE;
	}
}

/* Save the Blursk configuration into XMMS' config file */
void config_write(int force, gchar *preset, BlurskConfig *presetconfig)
{
	ConfigFile *cfg;	
	gchar *filename;
	gchar *section_name;
	BlurskConfig *c;

	/* If config window is shown, then don't write unless forced */
	if (config_win && !force && !preset)
		return;

	/* Retrieve the entire XMMS configuration file */
	if (preset)
	{
		filename = g_strconcat(g_get_home_dir(), "/.xmms/blursk-presets", NULL);
		section_name = preset;
		c = presetconfig;

		/* if invoked with "force" then delete any existing presets */
		if (force)
			unlink(filename);
	}
	else
	{
		filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
		section_name = "Blursk";
		c = &config;
	}
	cfg = xmms_cfg_open_file(filename);
	if (!cfg)
		cfg = xmms_cfg_new();

	/* Save the current configuration in the "Blursk" section */
	if (!preset)
	{
		xmms_cfg_write_int(cfg, section_name, "x", c->x);
		xmms_cfg_write_int(cfg, section_name, "y", c->y);
		xmms_cfg_write_int(cfg, section_name, "width", c->width);
		xmms_cfg_write_int(cfg, section_name, "height", c->height);
	}
	xmms_cfg_write_int(cfg, section_name, "color", c->color);
	xmms_cfg_write_string(cfg, section_name, "color_style", c->color_style);
	xmms_cfg_write_string(cfg, section_name, "signal_color", c->signal_color);
	xmms_cfg_write_boolean(cfg, section_name, "contour_lines", c->contour_lines);
	xmms_cfg_write_boolean(cfg, section_name, "hue_on_beats", c->hue_on_beats);
	xmms_cfg_write_string(cfg, section_name, "background", c->background);
	xmms_cfg_write_string(cfg, section_name, "fade_speed", c->fade_speed);
	xmms_cfg_write_string(cfg, section_name, "blur_style", c->blur_style);
	xmms_cfg_write_string(cfg, section_name, "transition_speed", c->transition_speed);
	xmms_cfg_write_string(cfg, section_name, "blur_when", c->blur_when);
	xmms_cfg_write_string(cfg, section_name, "blur_stencil", c->blur_stencil);
	xmms_cfg_write_boolean(cfg, section_name, "slow_motion", c->slow_motion);
	xmms_cfg_write_string(cfg, section_name, "signal_style", c->signal_style);
	xmms_cfg_write_string(cfg, section_name, "plot_style", c->plot_style);
	xmms_cfg_write_boolean(cfg, section_name, "thick_on_beats", c->thick_on_beats);
	xmms_cfg_write_string(cfg, section_name, "flash_style", c->flash_style);
	xmms_cfg_write_string(cfg, section_name, "overall_effect", c->overall_effect);
	xmms_cfg_write_string(cfg, section_name, "floaters", c->floaters);
	if (!preset)
	{
		xmms_cfg_write_string(cfg, section_name, "cpu_speed", c->cpu_speed);
		xmms_cfg_write_boolean(cfg, section_name, "window_title", c->window_title);
		xmms_cfg_write_string(cfg, section_name, "show_info", c->show_info);
		xmms_cfg_write_int(cfg, section_name, "beat_sensitivity", c->beat_sensitivity);
		xmms_cfg_write_string(cfg, section_name, "fullscreen_method", c->fullscreen_method);
		xmms_cfg_write_boolean(cfg, section_name, "fullscreen_shm", c->fullscreen_shm);
		xmms_cfg_write_boolean(cfg, section_name, "fullscreen_yuv709", c->fullscreen_yuv709);
		xmms_cfg_write_boolean(cfg, section_name, "fullscreen_root", c->fullscreen_root);
		xmms_cfg_write_boolean(cfg, section_name, "fullscreen_edges", c->fullscreen_edges);
		xmms_cfg_write_boolean(cfg, section_name, "fullscreen_revert", c->fullscreen_revert);
		xmms_cfg_write_boolean(cfg, section_name, "fullscreen_desired", c->fullscreen_desired);
		xmms_cfg_write_boolean(cfg, section_name, "random_preset", c->random_preset);
	}

	/* write out the XMMS config file */
	xmms_cfg_write_file(cfg, filename);
	xmms_cfg_free(cfg);
	g_free(filename);
}
#endif
