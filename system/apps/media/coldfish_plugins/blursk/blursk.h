/* blursk.h */


#ifndef BLURSK_H
#define BLURSK_H

#include <atheos/types.h>
#include <gui/window.h>

#define QTY(array)	(sizeof(array) / sizeof(*(array)))

#ifdef RAND_MAX
# define rand_0_to(n)	(int)((double)rand() * (double)(n) / (RAND_MAX + 1.0))
#else
# define rand_0_to(n)	(rand() % (n))
#endif

#ifndef TRUE
#define TRUE true
#endif

#ifndef FALSE
#define FALSE false
#endif

typedef int gint;
typedef uint guint;
typedef int32 gint32;
typedef uint32 guint32;
typedef char gchar;
typedef unsigned char guchar;
typedef int16 gint16;
typedef bool gboolean;
typedef double gdouble;

typedef struct
{
	/* position */
	gint	x;
	gint	y;
	gint	width;
	gint	height;

	/* color options */
        guint32 color;
	char	*color_style;
	char	*fade_speed;
	char	*signal_color;
	gint32	contour_lines;
	gint32	hue_on_beats;
	char	*background;

	/* blur/fade options */
	char	*blur_style;
	char	*transition_speed;
	char	*blur_when;
	char	*blur_stencil;
	gint32	slow_motion;

	/* other effects */
	char	*signal_style;
	char	*plot_style;
	gint32	thick_on_beats;
	char	*flash_style;
	char	*overall_effect;
	char	*floaters;

	/* miscellany from the Advanced screen */
	char	*cpu_speed;
	gint32	window_title;
	char	*show_info;

	/* beat detector */
	gint32	beat_sensitivity;

	/* fullscreen options */
	char	*fullscreen_method;
	gint32	fullscreen_shm;
	gint32	fullscreen_root;
	gint32	fullscreen_edges;
	gint32	fullscreen_yuv709;
	gint32	fullscreen_revert;
	gint32	fullscreen_desired;	/* not shown in [Advanced] dialog */

	/* "Random on quiet" preset */
	gint32	random_preset;

} BlurskConfig;

extern char config_default_color_style[];
extern char config_default_signal_color[];
extern char config_default_background[];
extern char config_default_blur_style[];
extern char config_default_transition_speed[];
extern char config_default_fade_speed[];
extern char config_default_blur_when[];
extern char config_default_blur_stencil[];
extern char config_default_signal_style[];
extern char config_default_plot_style[];
extern char config_default_flash_style[];
extern char config_default_overall_effect[];
extern char config_default_floaters[];
extern char config_default_cpu_speed[];
extern char config_default_show_info[];
extern char config_default_fullscreen_method[];

/* in about.c */
extern void about(void);
extern void about_error(char *format, ...);

/* in blur.c */
extern int blur_stencil;
extern int blur(int, int);
extern char *blur_name(int);
extern char *blur_when_name(int);

/* in blursk.c */
extern int blurskinfo;
extern int nspectrums;
extern os::View *blursk_view;
extern void blursk_fullscreen(gint ending);
extern void blursk_genrender(void);
extern char *blursk_name(int);
extern char *blursk_floater_name(int);

/* in color.c */
//extern GdkRgbCmap *color_map; 
extern os::Color32_s color_map[256];
extern guint32 colors[256];
extern void color_transition(int, int, int);
extern void color_genmap(int);
extern void color_bg(gint, gint16*);
extern void color_cleanup(void);
extern char *color_name(int);
extern char *color_background_name(int);
extern int color_good_for_bump(char *);
extern void color_beat(void);

/* in config.c */
extern BlurskConfig config;
//extern GtkWidget *config_win;
extern void config_load_preset(BlurskConfig *);
extern void config_load_color(guint32);
extern void config_dialog(void);
extern void config_advanced(void);
extern void config_default(BlurskConfig *);
extern void config_read(gchar *, BlurskConfig *);
extern void config_write(int, gchar *, BlurskConfig *);

/* in img.c */
#define IMG_PIXEL(x,y)	(img_buf[(y) * img_bpl + (x)])
extern guchar *img_buf;
extern guchar *img_prev;
extern guchar *img_tmp;
extern guchar **img_source;
extern guint img_height, img_physheight;
extern guint img_width, img_physwidth;
extern guint img_bpl, img_physbpl;
extern guint img_chunks;
extern guchar img_rippleshift;
extern void img_resize(int, int);
extern void img_copyback(void);
extern void img_invert(void);
extern guchar *img_expand(gint *, gint *, gint *);
extern guchar *img_bump(gint *, gint *, gint *);
extern guchar *img_travel(gint *, gint *, gint *);
extern guchar *img_ripple(gint *, gint *, gint *);

/* in loop.c or loopx86.s */
extern void loopblur(void);
extern void loopsmear(void);
extern void loopmelt(void);
extern void loopsharp(void);
extern void loopreduced1(void);
extern void loopreduced2(void);
extern void loopreduced3(void);
extern void loopreduced4(void);
extern void loopfade(int change);
extern void loopinterp(void);

/* in render.c */
extern void render_dot(gint x, gint y, guchar color);
extern void render(gint thick, gint center, gint ndata, gint16 *data);
extern char *render_plotname(int);

/* in preset.c */
extern void preset_adjust(int);
extern void preset_read(void);
extern os::Window *preset_init(void);
extern void preset_term(void);
extern void preset_quiet(void);
extern char *preset_gettitle(void);

/* in bitmap.c */
extern int bitmap_index(char *str);
extern int bitmap_test(int bindex, int x, int y);
extern void bitmap_flash(int bindex);
extern char *bitmap_flash_name(int i);
extern char *bitmap_stencil_name(int i);

/* in paste.c */
extern void paste(char *str);
extern char *paste_genstring(void);
extern BlurskConfig *paste_parsestring(char *str);

/* in text.c */
extern void textdraw(guchar *img, int height, int bpl, char *side, char *text);

/* in readme.c (generated from the README text file) */
extern const char readme[];

#endif

