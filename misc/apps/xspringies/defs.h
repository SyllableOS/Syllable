#include <stdio.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef char boolean;

#define TRUE            1
#define FALSE           0

/* Window size info */

/*
#define D_WID	640
#define D_HT	548
#define B_WID	200
#define T_WID	D_WID
#define T_HT	(F_HT + 4)
#define B_HT	(D_HT + T_HT + 1)
#define M_WID	(B_WID + D_WID + 1)
#define M_HT	B_HT
*/

#define COORD_X(x)	((int)(x))
#define COORD_Y(y)	((int)(draw_ht - (y)))
#define COORD_DX(dx)	((int)(dx))
#define COORD_DY(dy)	((int)(-(dy)))

/* Font size info */
#ifndef VMS
#define F_NAME	"6x10"
#else
#define F_NAME "-*-courier-medium-r-normal-*-10-100-*-*-*-*-*-*"
#endif /* VMS */

#define F_WID	6
#define F_HT	10

/* File operations */
#define F_NONE		0
#define F_LOAD		1
#define F_SAVE		2
#define F_INSERT	3

/* Longest filename */
#define MAXPATH		256

#define SQR(x)          ((x) * (x))
#define SGN(x)          ((x) < 0 ? (-1) : 1)
#ifndef ABS
#define ABS(x)          ((x) < 0 ? (-(x)) : (x))
#endif
#ifndef MIN
#define MIN(x,y)        ((x) < (y) ? (x) : (y))
#define MAX(x,y)        ((x) < (y) ? (y) : (x))
#endif

/* Key codes used */
#define K_DELETE	(-1)
#define K_RETURN	(-2)
#define K_ESCAPE	(-3)

/* Types of widgets */
#define O_NOTHING	(-1)
#define O_ACTING	0
#define O_BUTTON	1
#define O_SLIDER	2
#define O_CHECKBOX	3
#define O_MBUTTON	4
#define O_LSLIDER	5
#define O_RSLIDER	6
#define O_TSLIDER	7

/* Types of mouse actions */
#define M_DOWN		0
#define M_UP		1
#define M_DRAG		2
#define M_HOLD		3
#define M_REDISPLAY	4

/* Forces */
#define FR_GRAV		0
#define FR_CMASS	1
#define FR_PTATTRACT	2
#define FR_WALL		3

#define BF_NUM		4

/* Defaults */
#define DEF_TSTEP	0.025

typedef struct {
    double 	cur_mass, cur_rest;
    double 	cur_ks, cur_kd;
    boolean fix_mass, show_spring;
    int 		center_id;
    int 		bf_mode[BF_NUM];
    double 	cur_grav_val[BF_NUM];
    double 	cur_misc_val[BF_NUM];
    double 	cur_visc, cur_stick;
    double 	cur_dt, cur_prec;
    boolean	adaptive_step, grid_snap;
    double 	cur_gsnap;
    boolean w_top, w_left, w_right, w_bottom;
} t_state;

/* -------- Functions/variables defined --------- */

/* file.c	*/
boolean file_command( char* file, int command );

/* widget.c */
extern boolean scan_flag, key_widgets(), check_widgets();
extern void init_widgets(), add_button(), add_modebutton(), add_checkbox(), add_slider(), redraw_widgets();
extern void update_slider(), activate_mbutton();
extern int slider_valno();

/* xdisp.c */
/*extern int main_wid, main_ht, draw_wid, draw_ht;	*/
extern int draw_wid, draw_ht;

extern char filename[];
extern int cursor_pos, spthick;
extern int file_op, mass_radius();
extern void fatal(), disp_filename(), redraw_system(), review_system( boolean reset );
extern t_state mst;
extern t_state sst;	/* May move it altgui.cpp insted	*/


/* keypress.c */
extern void key_press();

/* misc.c */
extern char *xmalloc(), *xrealloc();

/* obj.c */
extern void init_objects(), delete_mass(), delete_spring(), delete_selected(), add_massparent(), del_massparent();
extern void select_object(), select_objects(), unselect_all(), select_all(), translate_selobj(), set_center();
extern boolean anything_selected();
extern int create_mass(), create_spring(), nearest_object();

void kill_fake_spring();
void move_fake_mass( int mx, int my );
void attach_fake_spring( int tomass );
void changevel_selobj( int vx, int vy, boolean relative );
void tempfixed_obj( boolean store );
void eval_selection();
void set_sel_restlen();
void set_center();
void duplicate_selected();


/* phys.c */

boolean animate_obj();


#ifdef __cplusplus
}
#endif
