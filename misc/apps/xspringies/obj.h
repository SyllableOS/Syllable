#define S_ALIVE		0x01
#define S_SELECTED	0x02
#define S_FIXED		0x04
#define S_TEMPFIXED	0x08

#define ALLOC_SIZE	32

typedef struct {
    /* Current position, velocity, acceleration */
    double x, y;
    double vx, vy;
    double ax, ay;

    /* Mass and radius of mass */
    double mass;
    double elastic;
    int radius;

    /* Connections to springs */
    int *pars;
    int num_pars;

    int status;

    /* RK temporary space */
    double cur_x, cur_y, cur_vx, cur_vy;
    double old_x, old_y, old_vx, old_vy;
    double test_x, test_y, test_vx, test_vy;
    double k1x, k1y, k1vx, k1vy;
    double k2x, k2y, k2vx, k2vy;
    double k3x, k3y, k3vx, k3vy;
    double k4x, k4y, k4vx, k4vy;
} mass;

typedef struct {
    /* Ks, Kd and rest length of spring */
    double ks, kd;
    double restlen;

    /* Connected to masses m1 and m2 */
    int m1, m2;

    int status;
} spring;

extern mass *masses;
extern spring *springs;
extern int num_mass, num_spring, fake_mass, fake_spring;

