/* keypress.c -- keyboard input support for xspringies
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
#include <X11/keysym.h>

/* key_press: process a key press of key */
void key_press(key, ksym, win)
int key;
KeySym ksym;
Window win;
{
    int i;
    static char cutbuff[MAXPATH];

    switch (ksym) {
      case XK_BackSpace:
      case XK_Delete:
	key = K_DELETE;
	break;
      case XK_Return:
	key = K_RETURN;
	break;
      case XK_Escape:
	key = K_ESCAPE;
	break;
    }

    /* Check for widget keyboard input */
    if (key_widgets(key, win))
      return;

    if (file_op != F_NONE) {
	int len;

	len = strlen(filename);

	if (ksym == XK_BackSpace || ksym == XK_Delete) {
	    if (len > 0 && cursor_pos > 0) {
		for (i = cursor_pos-1; i < len; i++) {
		    filename[i] = filename[i+1];
		}
		cursor_pos--;
	    }
	} else if (ksym == XK_Return) {
	    if (!file_command(filename, file_op))
	      file_error();
	    file_op = F_NONE;
	} else if (ksym == XK_Escape) {
	    file_op = F_NONE;
	} else if (ksym == XK_Left || key == '\002') {
	    /* control-b */
	    if (cursor_pos > 0)
	      cursor_pos--;
	} else if (ksym == XK_Right || key == '\006') {
	    /* control-f */
	    if (cursor_pos < len)
	      cursor_pos++;
	} else if (ksym == XK_Begin || ksym == XK_Home || key == '\001') {
	    /* control-a */
	    cursor_pos = 0;
	} else if (ksym == XK_End || key == '\005') {
	    /* control-e */
	    cursor_pos = len;
	} else if (ksym == XK_Clear || key == '\025') {
	    /* control-u */
	    filename[0] = '\0';
	    cursor_pos = 0;
	} else if (ksym == XK_Tab) {
	    /* File complete ... */

	} else if (key != '\0') {
	    switch (key) {
	      case '\004': /* control-d */
		if (len > 0 && cursor_pos < len) {
		    for (i = cursor_pos; i < len; i++) {
			filename[i] = filename[i+1];
		    }
		}
		break;

	      case '\013': /* control-k */
		strcpy(cutbuff, filename + cursor_pos);
		filename[cursor_pos] = '\0';
		break;
	      case '\031': /* control-y */
		{
		    int cblen = strlen(cutbuff);

		    if (len + cblen < MAXPATH) {
			for (i = len; i >= cursor_pos; i--) {
			    filename[i + cblen] = filename[i];
			}
			for (i = 0; i < cblen; i++) {
			    filename[i + cursor_pos] = cutbuff[i];
			}
			cursor_pos += cblen;
		    }
		}
		break;

	      case '\024': /* control-t */
		if (cursor_pos > 0 && len > 1) {
		    char tempc;
		    int cpos;

		    cpos = (cursor_pos == len) ? cursor_pos-1 : cursor_pos;

		    tempc = filename[cpos-1];
		    filename[cpos-1] = filename[cpos];
		    filename[cpos] = tempc;

		    if (cursor_pos < len)
		      cursor_pos++;
		}
		break;

	      default:
		if (len < MAXPATH - 1 && key >= ' ') {
		    for (i = len+1; i > cursor_pos; i--) {
			filename[i] = filename[i-1];
		    }
		    filename[cursor_pos] = key;
		    cursor_pos++;
		}
	    }
	}

	disp_filename(FALSE);
	return;
    }

    switch (key) {
      case K_DELETE:
	delete_selected();
	review_system(TRUE);
	break;
      case 'Q':
	fatal(NULL);
	break;
      case 0x0c:
	/* ^L is redraw screen */
	review_system(TRUE);
	break;
      default:
	break;
    }
}

#endif

