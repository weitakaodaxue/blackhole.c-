/* dev-only: dump one real terminal frame as plain ASCII so the shipped
 * render_frame() path can be eyeballed without a TTY. */
#define main bh_unused_main
#include "t1.c"
#undef main

int main(int argc, char **argv)
{
    static char buf[512 * 1024];
    double t   = (argc > 1) ? atof(argv[1]) : 14.0;
    size_t n   = render_frame(buf, 140, 48, t, 60.0);
    size_t i;
    for (i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)buf[i];
        if (c == 0x1b) {                       /* skip CSI ... final byte */
            ++i;
            if (i < n && buf[i] == '[') {
                ++i;
                while (i < n && !(buf[i] >= '@' && buf[i] <= '~')) ++i;
            }
            continue;
        }
        putchar(c);
    }
    return 0;
}
