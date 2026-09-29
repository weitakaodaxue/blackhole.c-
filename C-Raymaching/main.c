/* ===========================================================================
 *   B L A C K   H O L E   ·   Gravitationally Lensed Accretion Disk
 * ---------------------------------------------------------------------------
 *   A single-file, zero-dependency, real-time raytraced Schwarzschild black
 *   hole rendered entirely inside a text terminal.
 *
 *   Photons are *not* faked: every ray is integrated along a null geodesic of
 *   the Schwarzschild metric
 *
 *          d2u/dphi2  =  -u  +  (3/2) * Rs * u^2          ( u = 1/r )
 *
 *   so the event-horizon shadow, the photon ring, the Einstein ring and the
 *   iconic "disk seen above and below the hole" all fall out of the physics
 *   instead of being painted on afterwards.  The accretion disk is shaded with
 *   a Shakura-Sunyaev style temperature profile, relativistic Doppler beaming
 *   and gravitational redshift, then dithered onto a 24-bit ANSI truecolor
 *   character grid.
 *
 * ---------------------------------------------------------------------------
 *   BUILD
 *       gcc   -O3 main.c -lm -o blackhole
 *       clang -O3 main.c -lm -o blackhole      (macOS: -lm is a no-op)
 *       cl /O2 main.c                          (MSVC, optional)
 *
 *   RUN
 *       ./blackhole            (Linux / macOS / MSYS2 / MinGW)
 *       blackhole.exe          (Windows)
 *
 *   KEYS
 *       Ctrl+C   quit  -- the terminal cursor and colors are always restored
 *
 *   NOTES
 *       * Needs a 24-bit-color terminal: Windows Terminal, iTerm2, VSCode's
 *         integrated terminal, GNOME Terminal, kitty, Alacritty, WezTerm, ...
 *       * Zero flicker: the complete frame -- escape codes included -- is
 *         assembled in one heap buffer and pushed with a single fwrite()
 *         after a single cursor-home escape.  No clearing, ever, per frame.
 *       * Character aspect ratio is compensated (cells are ~2x taller than
 *         wide) so the disk renders as a true circle, not an ellipse.
 *       * The terminal is resized live; just drag the window.
 * =========================================================================*/

#ifndef _DEFAULT_SOURCE
#  define _DEFAULT_SOURCE 1          /* clock_gettime / TIOCGWINSZ on glibc  */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <time.h>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <unistd.h>
#  include <termios.h>
#  include <sys/ioctl.h>
#endif

/* ---------------------------------------------------------------------------
 *  Tunables
 * ------------------------------------------------------------------------ */
#define FPS_TARGET   60.0
#define MAX_STEPS    420              /* geodesic integration steps per ray  */
#define DPHI         0.030f           /* angular step of the integrator, rad */
#define R_ESCAPE     90.0f            /* beyond this a photon is free        */
#define RS           1.0f             /* Schwarzschild radius = unit length  */
#define R_ISCO       (3.0f  * RS)     /* inner edge of the disk (ISCO)       */
#define R_OUT        (13.0f * RS)     /* outer edge of the disk              */
#define CAM_DIST     (26.0f * RS)     /* camera distance from the singularity*/
#define CAM_FOV      0.90f            /* tan(half vertical fov) ~ 42 deg     */
#define DISK_THICK   0.28f            /* half thickness of the disk slab     */
#define EXPOSURE     1.45f            /* pre-tonemap gain                    */
#define STAR_GAIN    0.55f
#define NCHAR        10               /* length of the density ramp below    */

/* ---------------------------------------------------------------------------
 *  Tiny vector library (also used for RGB colors)
 * ------------------------------------------------------------------------ */
typedef struct { float x, y, z; } Vec3;

static Vec3 v3(float x, float y, float z)              { Vec3 r; r.x = x; r.y = y; r.z = z; return r; }
static Vec3 vadd(Vec3 a, Vec3 b)                       { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
static Vec3 vsub(Vec3 a, Vec3 b)                       { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
static Vec3 vmul(Vec3 a, float s)                      { return v3(a.x * s, a.y * s, a.z * s); }
static float vdot(Vec3 a, Vec3 b)                      { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vec3 vcross(Vec3 a, Vec3 b)                     { return v3(a.y * b.z - a.z * b.y,
                                                                a.z * b.x - a.x * b.z,
                                                                a.x * b.y - a.y * b.x); }
static float vlen(Vec3 a)                              { return sqrtf(vdot(a, a)); }
static Vec3 vnorm(Vec3 a)                              { float l = vlen(a); return l > 1e-9f ? vmul(a, 1.0f / l) : v3(0.0f, 0.0f, 0.0f); }
static float clampf(float v, float lo, float hi)       { return v < lo ? lo : (v > hi ? hi : v); }

/* ---------------------------------------------------------------------------
 *  Platform layer: wall clock, sleeping, terminal geometry, raw console
 * ------------------------------------------------------------------------ */
static double now_sec(void)
{
#ifdef _WIN32
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)f.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#endif
}

static void sleep_sec(double s)
{
    if (s <= 0.0) return;
#ifdef _WIN32
    Sleep((DWORD)(s * 1000.0));
#else
    {
        struct timespec ts;
        ts.tv_sec  = (time_t)s;
        ts.tv_nsec = (long)((s - (double)ts.tv_sec) * 1e9);
        if (ts.tv_nsec < 0) ts.tv_nsec = 0;
        nanosleep(&ts, NULL);
    }
#endif
}

/* Live terminal geometry with a sane fallback for piped / dumb output. */
static void term_size(int *w, int *h)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO ci;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci)) {
        *w = (int)(ci.srWindow.Right  - ci.srWindow.Left) + 1;
        *h = (int)(ci.srWindow.Bottom - ci.srWindow.Top)  + 1;
    } else {
        *w = 80; *h = 40;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        *w = (int)ws.ws_col;
        *h = (int)ws.ws_row;
    } else {
        *w = 80; *h = 40;
    }
#endif
    if (*w < 24) *w = 24;
    if (*h < 12) *h = 12;
}

static void vt_init(void)
{
#ifdef _WIN32
    /* Turn on ANSI escape handling in the legacy console host. */
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | 0x0004 /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */);
#endif
    /* Hide cursor, clear once, park at home. Never repeated per frame. */
    fputs("\033[?25l\033[2J\033[H", stdout);
    fflush(stdout);
}

static void vt_restore(void)
{
    fputs("\033[?25h\033[0m", stdout);   /* show cursor, reset attributes        */
    fflush(stdout);
}

/* ---------------------------------------------------------------------------
 *  Ctrl+C handling: the loop notices the flag and unwinds cleanly so the
 *  terminal is never left with a hidden cursor or a stuck color.
 * ------------------------------------------------------------------------ */
static volatile sig_atomic_t g_quit = 0;

static void on_signal(int sig)
{
    (void)sig;
    g_quit = 1;
}

/* ---------------------------------------------------------------------------
 *  Fast hashing -- used for the star field and for grain on the disk
 * ------------------------------------------------------------------------ */
static unsigned hash_u32(unsigned x)
{
    x ^= x >> 16; x *= 0x7feb352du;
    x ^= x >> 15; x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

static float hash3f(int x, int y, int s)
{
    unsigned h = ((unsigned)x * 73856093u) ^
                 ((unsigned)y * 19349663u) ^
                 ((unsigned)s * 83492791u);
    return (float)(hash_u32(h) & 0xFFFFFFu) * (1.0f / 16777215.0f);
}

/* ---------------------------------------------------------------------------
 *  Deep-space background: three octaves of jittered points plus a soft
 *  nebula wash, so escaping rays do not land on plain black.
 * ------------------------------------------------------------------------ */
static Vec3 starfield(Vec3 d)
{
    static const float sdens[3] = { 0.900f, 0.940f, 0.966f };
    static const float samp[3]  = { 1.000f, 0.520f, 0.260f };
    float u = atan2f(d.z, d.x) * (1.0f / 6.28318531f) + 0.5f;
    float v = asinf(clampf(d.y, -1.0f, 1.0f)) * (1.0f / 3.14159265f) + 0.5f;
    Vec3  c = v3(0.0f, 0.0f, 0.0f);
    int   layer;

    for (layer = 0; layer < 3; ++layer) {
        float cells = 70.0f * (float)(1 << layer);
        float fu = u * cells;
        float fv = v * cells * 0.5f;                 /* squash toward the poles */
        int   iu = (int)floorf(fu);
        int   iv = (int)floorf(fv);
        float h0 = hash3f(iu, iv, layer * 17 + 1);
        float h1 = hash3f(iu, iv, layer * 17 + 2);
        float h2 = hash3f(iu, iv, layer * 17 + 3);
        float h3 = hash3f(iu, iv, layer * 17 + 4);
        float h4 = hash3f(iu, iv, layer * 17 + 5);

        if (h0 > sdens[layer]) {                     /* only some cells hold a star */
            float cx = (float)iu + 0.25f + 0.50f * h1;   /* jitter stays central, so  */
            float cy = (float)iv + 0.25f + 0.50f * h2;   /* a star never gets clipped */
            float dx = fu - cx, dy = fv - cy;
            float d2 = dx * dx + dy * dy;
            /* tight core, halo dead within ~0.3 cell: one crisp terminal char */
            float b   = 1.0f / (1.0f + d2 * 30.0f);
            float mag = h3 * h3;                     /* a few bright ones         */
            Vec3  tint = v3(0.76f + 0.26f * h4, 0.84f + 0.14f * h4, 1.00f - 0.32f * h4);
            c = vadd(c, vmul(tint, b * b * mag * samp[layer] * STAR_GAIN));
        }
    }

    {   /* faint interstellar wash -- kept far below the sqrt() gamma's
           visibility floor so deep space stays genuinely black */
        float n = sinf(d.x * 3.10f + 1.0f) * sinf(d.y * 4.70f - 2.0f) * sinf(d.z * 3.70f + 3.0f);
        n = n * n;
        c = vadd(c, vmul(v3(0.16f, 0.10f, 0.30f), n * 0.00012f));
    }
    return c;
}

/* ---------------------------------------------------------------------------
 *  Disk palette: cool aurora blue at the rim -> violet -> crimson -> flame
 *  orange -> white hot toward the ISCO, so the Doppler-boosted inner edge
 *  scorches and the outer disk trails off into cyan.
 * ------------------------------------------------------------------------ */
static Vec3 disk_palette(float t)
{
    static const float stop[6][4] = {
        { 0.00f, 0.02f, 0.05f, 0.38f },   /* deep indigo   (outer rim)  */
        { 0.25f, 0.05f, 0.46f, 0.98f },   /* electric blue              */
        { 0.45f, 0.56f, 0.20f, 0.92f },   /* violet                     */
        { 0.62f, 0.98f, 0.24f, 0.30f },   /* crimson                    */
        { 0.80f, 1.00f, 0.58f, 0.10f },   /* flame orange               */
        { 1.00f, 1.00f, 0.95f, 0.78f }    /* white hot   (inner edge)   */
    };
    int i;
    t = clampf(t, 0.0f, 1.0f);
    for (i = 0; i < 5; ++i) {
        if (t <= stop[i + 1][0]) {
            float s = (t - stop[i][0]) / (stop[i + 1][0] - stop[i][0]);
            s = s * s * (3.0f - 2.0f * s);            /* smoothstep between stops */
            return v3(stop[i][1] + (stop[i + 1][1] - stop[i][1]) * s,
                      stop[i][2] + (stop[i + 1][2] - stop[i][2]) * s,
                      stop[i][3] + (stop[i + 1][3] - stop[i][3]) * s);
        }
    }
    return v3(stop[5][1], stop[5][2], stop[5][3]);
}

/* ---------------------------------------------------------------------------
 *  Branch-free-ish unsigned byte -> decimal, so we never touch sprintf()
 *  inside the pixel loop.
 * ------------------------------------------------------------------------ */
static char *put_u8(char *p, unsigned v)
{
    char tmp[3];
    int  n = 0;
    if (v > 255u) v = 255u;
    do { tmp[n++] = (char)('0' + (int)(v % 10u)); v /= 10u; } while (v);
    while (n > 0) *p++ = tmp[--n];
    return p;
}

static float sstep(float e0, float e1, float x)
{
    float t = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/* ---------------------------------------------------------------------------
 *  Accretion-disk shading.
 *
 *  nhat       unit vector pointing from the disk surface toward the camera
 *  col_depth  path length of the ray through the emitting slab (optical depth
 *             proxy, so grazing rays flare up the way they physically should)
 *
 *  Returns the emitted radiance (already HDR) and writes the coverage.
 * ------------------------------------------------------------------------ */
static Vec3 disk_shade(float r, float th, Vec3 nhat, double t,
                       float col_depth, float *alpha_out)
{
    float T    = powf(R_ISCO / r, 0.75f);          /* Shakura-Sunyaev T(r)   */
    float beta = sqrtf(0.5f * RS / r);             /* circular-orbit speed   */
    if (beta > 0.96f) beta = 0.96f;

    /* relativistic Doppler beaming: the side sweeping toward us is brighter
       and bluer, the receding side is dimmed and reddened */
    float vx   = -sinf(th), vz = cosf(th);         /* prograde velocity dir  */
    float dopp = vx * nhat.x + vz * nhat.z;
    float gam  = 1.0f / sqrtf(1.0f - beta * beta);
    float den  = gam * (1.0f - beta * dopp);
    if (den < 0.06f) den = 0.06f;
    float delta = 1.0f / den;

    /* gravitational redshift */
    float grav  = sqrtf(clampf(1.0f - RS / r, 0.02f, 1.0f));
    float shift = clampf(delta * grav, 0.12f, 2.8f);

    /* differentially rotating turbulence -- omega ~ r^-3/2 shears the pattern
       into the spiral filaments you see in real GRMHD simulations */
    float omega = 2.2f / (r * sqrtf(r));
    float a  = th - omega * (float)t;
    float f  = 0.50f * sinf(3.0f * a + 1.30f * r)
             + 0.30f * sinf(5.0f * a - 2.10f * r + 1.3f)
             + 0.20f * sinf(8.0f * a + 3.20f * r + 4.1f);
    float turb = 0.62f + 0.38f * f;

    /* radial envelope: razor inner rim at the ISCO, soft outer fade */
    float env = sstep(R_ISCO, R_ISCO + 0.30f, r)
              * (1.0f - sstep(R_OUT - 3.0f, R_OUT, r));

    float dens  = (0.30f + 0.70f * T) * (0.35f + 0.65f * turb) * env;
    float tau   = dens * col_depth * 2.1f;
    float alpha = tau / (1.0f + tau);              /* cheap 1 - exp(-tau)    */

    float boost = powf(shift, 2.2f);               /* beaming + shift gain   */
    float I     = (0.06f + 0.94f * T * T) * boost * (0.40f + 0.60f * turb);

    /* map (T * doppler shift) onto the palette: the white-hot inner rim, then
       flame orange, crimson, violet, electric blue and deep indigo outward */
    float pal = clampf((T * shift - 0.22f) / 0.62f, 0.0f, 1.0f);

    *alpha_out = alpha;
    return vmul(disk_palette(pal), I);
}

/* ---------------------------------------------------------------------------
 *  THE core: integrate one photon backwards along its null geodesic.
 *
 *  The trajectory is planar, so we work in the plane spanned by e1 (radial at
 *  the camera) and e2 (the in-plane direction of the ray) and solve
 *
 *        d2u/dphi2 = -u + (3/2) Rs u^2 ,      u = 1/r
 *
 *  with a velocity-Verlet step.  Basis rotation is incremental, so there is
 *  not a single trig call in the hot loop.
 * ------------------------------------------------------------------------ */
static Vec3 trace_ray(Vec3 cam, Vec3 dir, double t)
{
    Vec3  e1   = vnorm(cam);
    Vec3  perp = vsub(dir, vmul(e1, vdot(dir, e1)));
    float plen = vlen(perp);
    float u, dudp, h, c, s, ay_prev, trans;
    float A, B, cd, sd;
    Vec3  e2, col;
    int   i, escaped = 0;

    if (plen < 1e-5f) return v3(0.0f, 0.0f, 0.0f);     /* dead-on: swallowed */
    e2 = vmul(perp, 1.0f / plen);

    u    = 1.0f / vlen(cam);
    dudp = -u * vdot(dir, e1) / plen;                  /* du/dphi at the camera */

    A = e1.y; B = e2.y;                                /* disk-plane projection */
    cd = cosf(DPHI); sd = sinf(DPHI);
    h  = 0.5f * DPHI;
    c  = 1.0f; s = 0.0f;
    ay_prev = A;
    trans   = 1.0f;
    col     = v3(0.0f, 0.0f, 0.0f);

    for (i = 0; i < MAX_STEPS; ++i) {
        float u_start = u, r_start, ay, acc;

        /* ---- one velocity-Verlet step of the geodesic ---- */
        acc   = -u + 1.5f * RS * u * u;
        dudp += acc * h;
        u    += dudp * DPHI;
        acc   = -u + 1.5f * RS * u * u;
        dudp += acc * h;

        /* ---- rotate the orbital-plane basis incrementally ---- */
        {
            float cn = c * cd - s * sd;
            float sn = s * cd + c * sd;
            c = cn; s = sn;
        }

        if (u <= 1.0f / R_ESCAPE) { escaped = 1; break; }   /* photon is free  */
        if (u >= 1.0f / RS)       { escaped = 0; break; }   /* hit the horizon */

        /* ---- did the photon punch through the disk plane this step? ---- */
        ay = c * A + s * B;
        if ((ay_prev < 0.0f) != (ay < 0.0f)) {
            float denom = ay_prev * u - ay * u_start;
            float frac  = (denom != 0.0f) ? (ay_prev * u) / denom : 0.0f;
            float rc;

            frac  = clampf(frac, 0.0f, 1.0f);
            rc    = 1.0f / (u_start + frac * (u - u_start));
            r_start = 1.0f / u_start;

            if (rc > R_ISCO && rc < R_OUT) {
                /* local photon propagation direction; the integrator marches
                   camera-ward, so the real photon travels opposite to tang */
                Vec3  radial  = vadd(vmul(e1, c), vmul(e2, s));
                Vec3  angular = vadd(vmul(e1, -s), vmul(e2, c));
                Vec3  tang    = vadd(vmul(radial, -dudp / (u * u)),
                                     vmul(angular, 1.0f / u));
                float tl      = vlen(tang);
                Vec3  nhat    = (tl > 1e-9f) ? vmul(tang, -1.0f / tl) : vnorm(cam);
                float px      = rc * (c * e1.x + s * e2.x);
                float pz      = rc * (c * e1.z + s * e2.z);
                float th      = atan2f(pz, px);
                float dr      = 1.0f / u - r_start;
                float dl      = sqrtf(dr * dr + rc * rc * DPHI * DPHI);
                float dyw     = fabsf(ay_prev / u_start - ay / u);
                float slope   = (dl > 1e-9f) ? (dyw / dl) : 1.0f;
                float col_depth, alpha = 0.0f;
                Vec3  S;

                if (slope < 0.03f) slope = 0.03f;
                col_depth = (DISK_THICK * sqrtf(rc / R_ISCO)) / slope;
                if (col_depth > 7.0f) col_depth = 7.0f;

                S    = disk_shade(rc, th, nhat, t, col_depth, &alpha);
                col  = vadd(col, vmul(S, trans * alpha));
                trans *= (1.0f - alpha);
                if (trans < 0.012f) break;             /* disk is opaque now */
            }
        }
        ay_prev = ay;
    }

    if (escaped) {                                     /* look up at the sky  */
        Vec3  radial  = vadd(vmul(e1, c), vmul(e2, s));
        Vec3  angular = vadd(vmul(e1, -s), vmul(e2, c));
        Vec3  tang    = vadd(vmul(radial, -dudp / (u * u)),
                             vmul(angular, 1.0f / u));
        float tl      = vlen(tang);
        if (tl > 1e-9f)
            col = vadd(col, vmul(starfield(vmul(tang, 1.0f / tl)), trans));
    }

    return col;
}

/* ===========================================================================
 *  Frame assembly
 *
 *  Every pixel becomes  "\033[38;2;R;G;Bm" + one density character.  Identical
 *  consecutive colors reuse the previous escape, which pays for itself many
 *  times over inside the black shadow.
 * ======================================================================== */
#define PUTLIT(s) do { memcpy(p, (s), sizeof(s) - 1); p += sizeof(s) - 1; } while (0)

static const char RAMP[NCHAR + 1] = " .:-=+*#%@";

/* One image row. Returns the advanced pointer. */
static char *render_row(char *p, int y, int rw, int rh, Vec3 cam, Vec3 fwd,
                        Vec3 rgt, Vec3 up, float k, double t)
{
    int x, lr = -1, lg = -1, lb = -1;

    for (x = 0; x < rw; ++x) {
        /* ---- char-cell aspect correction: a text cell is ~2x taller
                than it is wide, hence the 0.5 on the horizontal axis ---- */
        float uu = ((float)x + 0.5f - (float)rw * 0.5f) * 0.5f;
        float vv = ((float)y + 0.5f - (float)rh * 0.5f);
        Vec3  d  = vnorm(vadd(fwd, vadd(vmul(rgt, uu * k), vmul(up, vv * k))));
        Vec3  c  = vmul(trace_ray(cam, d, t), EXPOSURE);
        float lum;
        int   ri, gi, bi, ci;

        /* per-channel Reinhard tonemap + one gamma step */
        if (!(c.x > 0.0f)) c.x = 0.0f;
        if (!(c.y > 0.0f)) c.y = 0.0f;
        if (!(c.z > 0.0f)) c.z = 0.0f;
        c.x = sqrtf(c.x / (1.0f + c.x));
        c.y = sqrtf(c.y / (1.0f + c.y));
        c.z = sqrtf(c.z / (1.0f + c.z));

        ri = (int)(clampf(c.x, 0.0f, 1.0f) * 255.0f + 0.5f);
        gi = (int)(clampf(c.y, 0.0f, 1.0f) * 255.0f + 0.5f);
        bi = (int)(clampf(c.z, 0.0f, 1.0f) * 255.0f + 0.5f);

        lum = 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
        if (!(lum > 0.0f)) lum = 0.0f;
        ci = (int)(sqrtf(lum) * (float)(NCHAR - 1) * 1.0001f);
        if (ci > NCHAR - 1) ci = NCHAR - 1;

        if (ri != lr || gi != lg || bi != lb) {
            PUTLIT("\033[38;2;");
            p = put_u8(p, (unsigned)ri); *p++ = ';';
            p = put_u8(p, (unsigned)gi); *p++ = ';';
            p = put_u8(p, (unsigned)bi); *p++ = 'm';
            lr = ri; lg = gi; lb = bi;
        }
        *p++ = RAMP[ci];
    }
    return p;
}

static size_t render_frame(char *buf, int W, int H, double t, double fps)
{
    int    rw = W - 1;                    /* last column stays untouched      */
    int    rh = H - 1;                    /* last row is the status bar       */
    char  *p  = buf;
    float  az, el, k;
    Vec3   cam, fwd, rgt, up;
    int    y, j;

    if (rw < 8)  rw = 8;
    if (rh < 4)  rh = 4;

    /* home the cursor, then paint the page black. Spare cells are never given
       a glyph, so without this the event horizon would show whatever colour
       the user's terminal happens to use as its background. */
    PUTLIT("\033[H\033[48;2;0;0;0m");

    /* ---- orbiting camera, slowly nodding across the disk plane ---- */
    az  = (float)t * 0.22f;
    el  = 0.32f + 0.15f * sinf((float)t * 0.113f);   /* 10 deg .. 27 deg      */
    cam = v3(CAM_DIST * cosf(el) * cosf(az),
             CAM_DIST * sinf(el),
             CAM_DIST * cosf(el) * sinf(az));
    fwd = vmul(vnorm(cam), -1.0f);
    rgt = vnorm(vcross(fwd, v3(0.0f, 1.0f, 0.0f)));
    up  = vcross(rgt, fwd);
    k   = CAM_FOV / ((float)rh * 0.5f);

    for (y = 0; y < rh; ++y) {
        p = render_row(p, y, rw, rh, cam, fwd, rgt, up, k, t);
        PUTLIT("\r\n");
    }

    /* ---- status bar ---- */
    {
        char line[256];
        int  n = snprintf(line, sizeof line,
                          "  SCHWARZSCHILD BLACK HOLE   %dx%d   %5.1f FPS   "
                          "Rs=1.0   geodesic raymarcher   Ctrl+C to exit  ",
                          W, H, fps);
        if (n < 0) n = 0;
        if (n > rw) n = rw;
        PUTLIT("\033[0m\033[38;2;112;134;176m");
        for (j = 0; j < n; ++j) *p++ = line[j];
        for (; j < rw; ++j) *p++ = ' ';
        PUTLIT("\033[0m");
    }

    return (size_t)(p - buf);
}

/* ===========================================================================
 *  Entry point
 * ======================================================================== */
int main(void)
{
    int     W = 0, H = 0, cw = 0, ch = 0;
    char   *buf;
    double  t0, tprev, fps = 0.0;

    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);

    vt_init();
    term_size(&W, &H);
    cw = W; ch = H;

    buf = (char *)malloc((size_t)(W * H * 32 + 2048));
    if (buf == NULL) { vt_restore(); return 1; }

    t0    = now_sec();
    tprev = t0;

    while (!g_quit) {
        double t, dt, sp;
        size_t len;

        term_size(&W, &H);                       /* pick up live resizes */
        if (W != cw || H != ch) {
            char *nb = (char *)realloc(buf, (size_t)(W * H * 32 + 2048));
            if (nb != NULL) buf = nb;
            cw = W; ch = H;
            fputs("\033[2J", stdout);            /* one clean repaint    */
        }

        t   = now_sec() - t0;
        len = render_frame(buf, W, H, t, fps);
        fwrite(buf, 1, len, stdout);
        fflush(stdout);

        {
            double now = now_sec();
            dt    = now - tprev;
            tprev = now;
        }
        if (dt > 1e-6) {
            double inst = 1.0 / dt;
            fps = (fps > 0.0) ? (fps * 0.90 + inst * 0.10) : inst;
        }
        sp = 1.0 / FPS_TARGET - dt;              /* pace to the target rate   */
        if (sp > 0.0) sleep_sec(sp);
    }

    vt_restore();
    free(buf);
    return 0;
}

