/* img.c */

/*  Blursk - visualization plugin for XMMS
 *  Copyright (C) 1999  Steve Kirkendall
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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "blursk.h"

/* These global variables store image information */
guchar	*img_buf;	/* base of the current image buffer */
guchar	*img_tmp;	/* base of another image buffer, for temp operations */
guchar	**img_source;	/* an array of pixel pointers, for blur motion */
guint	img_height;	/* height of the current image */
guint	img_width;	/* width of the current image */
guint	img_bpl;	/* bytes per line of the current image */
guint	img_chunks;	/* number of 8-pixel chunks in the image */

guint	img_physheight;	/* height of the current window */
guint	img_physwidth;	/* width of the current window */
guchar	img_rippleshift;/* ripple map cycling counter */

/* These are the base addresses of allocated memory, so we can free the old
 * images when we resize to a new image.
 */
static guchar *base_buf;
static guchar *base_tmp;
static guchar **base_source;

/* This stores the state of the "cpu_speed" option when bufs were allocated */
static char speed;

/* Allocate buffers for an image with a given size.  Initialize the buffers.
 * This function should be called during initialization, and again any time the
 * window size or "cpu_speed" option changes.
 */
void img_resize(int physwidth, int physheight)
{
	size_t	size;
	guchar	*buf, **source;
	int	tmp_factor;

	/* If same size & cpu speed, then do nothing */
	if (physwidth == img_physwidth && physheight == img_physheight
	 && *config.cpu_speed == speed)
		return;

	/* free the old memory, if any */
	if (base_buf)
	{
		free(base_buf);
		free(base_tmp);
		free(base_source);
	}

	/* Store the width, height, and bytes-per-line of the new image size.
	 * Bytes-per-line is an odd number greater than 2; this gives us a
	 * neutral border around the image (which simplifies blurring) and
	 * causes even-byte dithering to have a checkerboard pattern instead
	 * of vertical lines (so dithering looks better).
	 */
	img_physheight = physheight;
	img_physwidth = physwidth;
	speed =  *config.cpu_speed;
	switch (speed)
	{
	  case 'F': /* Fast CPU */
		img_height = physheight;
		img_width = physwidth;
		tmp_factor = 1;
		break;

	  case 'M': /* Medium CPU */
		img_height = physheight;
		img_width = (physwidth + 1) / 2;
		tmp_factor = 2;
		break;

	  default: /* Slow CPU */
		img_height = (physheight + 1) / 2;
		img_width = (physwidth + 1) / 2;
		tmp_factor = 4;
	}
	img_bpl = ((img_width + 3) & ~1) + 1;

	/* Compute the number of chunks.  This is the number of 8-pixel groups
	 * that are needed to cover all visible pixels.
	 */
	img_chunks = (img_height * img_bpl + 7) >> 3;

	/* Compute the number of pixels to allocate.  This should include
	 * two extra rasters above and two below the image.  It should also
	 * include enough extra bytes so that the base of the visible image
	 * is on an 8-byte boundary.
	 */
	size = ((img_height + 4) * img_bpl + 7) & ~7;

	/* allocate the memory */
	base_buf = (guchar *)malloc(size * sizeof(guchar));
	base_tmp = (guchar *)malloc(size * tmp_factor * sizeof(guchar));
	base_source = (guchar **)malloc(size * sizeof(guchar *));

	/* Initialize the memory */
	memset(base_buf, 0, size);
	for (buf = base_buf, source = base_source; size != 0; size--)
		*source++ = buf++;

	/* Set the image pointer bases to the start of the visible pixels */
	size = (img_bpl * 2 + 7) & ~7;
	img_buf = base_buf + size;
	img_tmp = base_tmp + tmp_factor * size;
	img_source = base_source + size;
}

/* Copy the visible parts of img_tmp into img_buf, without disturbing the
 * border pixels.  The image in img_tmp is assumed to be the same size as the
 * one in img_buf, regardless of the cpu_speed option.
 */
void img_copyback(void)
{
	int	i;
	guchar	*src, *dst;

	for (i = img_height, src = img_tmp, dst = img_buf;
	     --i >= 0;
	     src += img_bpl, dst += img_bpl)
	{
		memcpy(dst, src, img_width);
	}
}


/* Invert the visible pixels in img_buf, but not the border pixels */
void img_invert(void)
{
	guchar	*pixel;
	int	y, x;

	for (y = img_height, pixel = img_buf; --y >= 0; pixel += img_bpl - img_width)
		for (x = img_width; --x >= 0; pixel++)
			/* Invert the pixel in such a way that 255 is mapped
			 * back to 255.  This makes the "white signal" color
			 * flag look better.
			 */
			*pixel = 254 - *pixel; 
}


/* Expand the image in img_buf into img_tmp */
guchar *img_expand(gint *widthref, gint *heightref, gint *bplref)
{
	int	i, bpl;
	guchar	*src, *dst;

	switch (speed)
	{
	  case 'F':	/* Fast */
		/* No copying necessary, just return img_buf */
		*widthref = img_width;
		*heightref = img_height;
		*bplref = img_bpl;
		return img_buf;

	  case 'M': /* Medium */
		/* Expand img_buf into img_tmp */
		loopinterp();
		*widthref = img_physwidth;
		*heightref = img_physheight;
		*bplref = img_bpl * 2;
		return img_tmp;

	  default: /* Medium or Fast */
		/* Expand img_buf into img_tmp */
		loopinterp();

		/* Double up every raster line */
		bpl = 2 * img_bpl;
		src = &img_tmp[(img_height - 1) * bpl];
		dst = &img_tmp[(img_physheight - 1) * bpl];
		for (i = img_height; --i >= 0; )
		{
			memcpy(dst, src, img_physwidth);
			dst -= bpl;
			memcpy(dst, src, img_physwidth);
			dst -= bpl;
			src -= bpl;
		}

		/* Return it */
		*widthref = img_physwidth;
		*heightref = img_physheight;
		*bplref = bpl;
		return img_tmp;
	}
}


/* This transforms a normal image into a "bump effect" image.  It also expands
 * the image like img_expand() if necessary.
 */
guchar *img_bump(gint *widthref, gint *heightref, gint *bplref)
{
	guchar	*dst, *src, *end;
	int	delta, bpl, i;

	switch (speed)
	{
	  case 'F':	/* Fast CPU */
		/* Can't generate shadows for the first few pixels, so just use
		 * a generic flat background.  And hope nobody notices.
		 */
		delta = 3 * img_bpl + 2;
		memset(img_tmp, 128, delta);

		/* The remaining ones can have shadows.  Lift the "white_signal"
		 * test outside the loop, for efficiency.
		 */
		src = img_buf + delta;
		dst = img_tmp + delta;
		end = img_tmp + img_height * img_bpl;
		if (*config.signal_color == 'W')
		{
			for (; dst < end; dst++, src++)
			{
				if (*src == 255)
					*dst = 255;
				else
					*dst = (256 + src[0] - src[-delta]) / 2;
			}
		}
		else
		{
			for (; dst < end; dst++, src++)
			{
				*dst = (256 + src[0] - src[-delta]) / 2;
			}
		}

		/* return the image size */
		*widthref = img_width;
		*heightref = img_height;
		*bplref = img_bpl;
		return img_tmp;

	  default: /* Medium CPU or Slow CPU */
		/* Can't generate shadows for the first few pixels, so just use
		 * a generic flat background.  And hope nobody notices.
		 */
		delta = 3 * img_bpl + 2;
		memset(img_tmp, 128, delta * 2);

		/* The remaining ones can have shadows.  Lift the "white_signal"
		 * test outside the loop, for efficiency.
		 */
		src = img_buf + delta;
		dst = img_tmp + delta * 2;
		end = img_tmp + img_height * img_bpl * 2;
		if (*config.signal_color == 'W')
		{
			for (; dst < end; dst += 2, src++)
			{
				if (*src == 255)
					dst[0] = dst[1] = 255;
				else
					dst[0] = dst[1] = (256 + src[0] - src[-delta]) / 2;
			}
		}
		else
		{
			for (; dst < end; dst += 2, src++)
			{
				dst[0] = dst[1] = (256 + src[0] - src[-delta]) / 2;
			}
		}

		/* For "Slow CPU", we also need to double the height */
		if (speed == 'S')
		{
			bpl = 2 * img_bpl;
			src = &img_tmp[(img_height - 1) * bpl];
			dst = &img_tmp[(img_physheight - 1) * bpl];
			for (i = img_height; --i >= 0; )
			{
				memcpy(dst, src, img_physwidth);
				dst -= bpl;
				memcpy(dst, src, img_physwidth);
				dst -= bpl;
				src -= bpl;
			}
		}

		/* return the physical size */
		*widthref = img_physwidth;
		*heightref = img_physheight;
		*bplref = img_bpl * 2;
		return img_tmp;
	}
}

/* This transforms a normal image into a "travel effect" image.  It also
 * expands the image like img_expand() if necessary.
 */
guchar *img_travel(gint *widthref, gint *heightref, gint *bplref)
{
	guchar	*dst, *src;
	int	bpl, i;
	static guchar shift;

	/* Compute colormap shift factor, based on fade speed and whether this
	 * function is called for every frame, or just alternate frames.
	 */
	switch (*config.fade_speed)
	{
	  case 'N':	i = 0;	break;
	  case 'S':	i = 1;	break;
	  case 'M':	i = 3;	break;
	  default:	i = 9;	break;
	}
	shift = (shift + i) & 0xff;

	/* Copy the image, expanding it for lower CPU speeds */
	switch (speed)
	{
	  case 'F':	/* Fast CPU */
		/* The remaining ones can have shadows.  Lift the "white_signal"
		 * test outside the loop, for efficiency.
		 */
		src = img_buf;
		dst = img_tmp;
		i = img_chunks;
		if (*config.signal_color == 'W')
		{
			for (i <<= 3; --i >= 0; dst++, src++)
			{
				if (*src == 255 || *src < 3)
					*dst = *src;
				else if ((guchar)(*src + shift) == 255)
					*dst = 254;
				else
					*dst = *src + shift;
			}
		}
		else
		{
			for (; --i >= 0; )
			{
				if ((*dst = *src++) >= 3) *dst += shift;
				dst++;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst++;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst++;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst++;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst++;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst++;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst++;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst++;
			}
		}

		/* return the image size */
		*widthref = img_width;
		*heightref = img_height;
		*bplref = img_bpl;
		return img_tmp;

	  default: /* Medium CPU or Slow CPU */
		/* The remaining ones can have shadows.  Lift the "white_signal"
		 * test outside the loop, for efficiency.
		 */
		src = img_buf;
		dst = img_tmp;
		i = img_chunks;
		if (*config.signal_color == 'W')
		{
			for (i <<= 3; --i >= 0; dst += 2, src++)
			{
				if (*src == 255 || *src < 3)
					dst[0] = dst[1] = *src;
				else if ((guchar)(*src + shift) == 255)
					*dst = 254;
				else
					dst[0] = dst[1] = *src + shift;
			}
		}
		else
		{
			for (; --i >= 0; )
			{
				if ((*dst = *src++) >= 3) *dst += shift;
				dst[1] = dst[0];
				dst += 2;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst[1] = dst[0];
				dst += 2;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst[1] = dst[0];
				dst += 2;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst[1] = dst[0];
				dst += 2;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst[1] = dst[0];
				dst += 2;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst[1] = dst[0];
				dst += 2;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst[1] = dst[0];
				dst += 2;
				if ((*dst = *src++) >= 3) *dst += shift;
				dst[1] = dst[0];
				dst += 2;
			}
		}

		/* For "Slow CPU", we also need to double the height */
		if (speed == 'S')
		{
			bpl = 2 * img_bpl;
			src = &img_tmp[(img_height - 1) * bpl];
			dst = &img_tmp[(img_physheight - 1) * bpl];
			for (i = img_height; --i >= 0; )
			{
				memcpy(dst, src, img_physwidth);
				dst -= bpl;
				memcpy(dst, src, img_physwidth);
				dst -= bpl;
				src -= bpl;
			}
		}

		/* return the physical size */
		*widthref = img_physwidth;
		*heightref = img_physheight;
		*bplref = img_bpl * 2;
		return img_tmp;
	}
}

/* This transforms a normal image into a "Ripple effect" image.  It also
 * expands the image like img_expand() if necessary.
 */
guchar *img_ripple(gint *widthref, gint *heightref, gint *bplref)
{
	guchar	*dst, *src;
	int	bpl, i;
	guchar	tbl[256];

	/* Compute the mapping table */
	for (i = QTY(tbl); --i >= 0; )
	{
		tbl[i] = i + (guchar)((double)((QTY(tbl)/2 - abs(QTY(tbl)/2 - i)) >> 1) * sin((double)(i + img_rippleshift) / 10.0));
	}

	/* Copy the image, expanding it for lower CPU speeds */
	switch (speed)
	{
	  case 'F':	/* Fast CPU */
		/* copy the image, computing deltas */
		for (src = img_buf, dst = img_tmp, i = img_chunks;
		     --i >= 0;
		     )
		{
			*dst++ = tbl[*src++];
			*dst++ = tbl[*src++];
			*dst++ = tbl[*src++];
			*dst++ = tbl[*src++];
			*dst++ = tbl[*src++];
			*dst++ = tbl[*src++];
			*dst++ = tbl[*src++];
			*dst++ = tbl[*src++];
		}

		/* return the image size */
		*widthref = img_width;
		*heightref = img_height;
		*bplref = img_bpl;
		return img_tmp;

	  default: /* Medium CPU or Slow CPU */
		for (src = img_buf, dst = img_tmp, i = img_chunks;
		     --i >= 0;
		     )
		{
			dst[0] = dst[1] = tbl[*src++], dst += 2;
			dst[0] = dst[1] = tbl[*src++], dst += 2;
			dst[0] = dst[1] = tbl[*src++], dst += 2;
			dst[0] = dst[1] = tbl[*src++], dst += 2;
			dst[0] = dst[1] = tbl[*src++], dst += 2;
			dst[0] = dst[1] = tbl[*src++], dst += 2;
			dst[0] = dst[1] = tbl[*src++], dst += 2;
			dst[0] = dst[1] = tbl[*src++], dst += 2;
		}

		/* For "Slow CPU", we also need to double the height */
		if (speed == 'S')
		{
			bpl = 2 * img_bpl;
			src = &img_tmp[(img_height - 1) * bpl];
			dst = &img_tmp[(img_physheight - 1) * bpl];
			for (i = img_height; --i >= 0; )
			{
				memcpy(dst, src, img_physwidth);
				dst -= bpl;
				memcpy(dst, src, img_physwidth);
				dst -= bpl;
				src -= bpl;
			}
		}

		/* return the physical size */
		*widthref = img_physwidth;
		*heightref = img_physheight;
		*bplref = img_bpl * 2;
		return img_tmp;
	}
}
