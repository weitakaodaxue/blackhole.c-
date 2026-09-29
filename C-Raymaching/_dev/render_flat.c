/* dev-only harness: renders the same image the terminal shows, but at
   supersampled square-pixel resolution, into a PNG so it can be eyeballed.
   NOT part of the shipped program.                                      */
#define main bh_unused_main
#include "main_flat.c"
#undef main

static unsigned crc_table[256];
static void crc_init(void)
{
    unsigned c; int n, k;
    for (n = 0; n < 256; n++) {
        c = (unsigned)n;
        for (k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        crc_table[n] = c;
    }
}
static unsigned crc32_of(const unsigned char *b, size_t n)
{
    unsigned c = 0xFFFFFFFFu; size_t i;
    for (i = 0; i < n; i++) c = crc_table[(c ^ b[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
static unsigned adler32_of(const unsigned char *b, size_t n)
{
    unsigned a = 1, d = 0; size_t i;
    for (i = 0; i < n; i++) { a = (a + b[i]) % 65521u; d = (d + a) % 65521u; }
    return (d << 16) | a;
}
static unsigned char *chunk(unsigned char *p, const char *type,
                            const unsigned char *data, size_t len)
{
    unsigned c;
    p[0]=(unsigned char)(len>>24); p[1]=(unsigned char)(len>>16);
    p[2]=(unsigned char)(len>>8);  p[3]=(unsigned char)len;
    memcpy(p + 4, type, 4);
    if (len) memcpy(p + 8, data, len);
    c = crc32_of(p + 4, len + 4);
    p[8+len]  =(unsigned char)(c>>24); p[9+len] =(unsigned char)(c>>16);
    p[10+len] =(unsigned char)(c>>8);  p[11+len]=(unsigned char)c;
    return p + 12 + len;
}
static void write_png(const char *path, const unsigned char *rgb, int w, int h)
{
    size_t stride = (size_t)w * 3 + 1;
    size_t rawlen = stride * (size_t)h;
    unsigned char *raw = (unsigned char *)malloc(rawlen);
    unsigned char *z, *zp, *out, *op;
    size_t zlen, off = 0, i, nblocks;
    int y; FILE *f;

    for (y = 0; y < h; y++) {
        raw[(size_t)y * stride] = 0;
        memcpy(raw + (size_t)y * stride + 1, rgb + (size_t)y * w * 3, (size_t)w * 3);
    }
    nblocks = (rawlen + 65534) / 65535;
    zlen = 2 + nblocks * 5 + rawlen + 4;
    z = (unsigned char *)malloc(zlen);
    zp = z; *zp++ = 0x78; *zp++ = 0x01;
    for (i = 0; i < nblocks; i++) {
        size_t blk = rawlen - off; unsigned short nlen;
        int final;
        if (blk > 65535) blk = 65535;
        final = (i == nblocks - 1);
        nlen = (unsigned short)(~blk & 0xFFFF);
        *zp++ = (unsigned char)(final ? 1 : 0);
        *zp++ = (unsigned char)(blk & 0xFF);   *zp++ = (unsigned char)((blk >> 8) & 0xFF);
        *zp++ = (unsigned char)(nlen & 0xFF);  *zp++ = (unsigned char)((nlen >> 8) & 0xFF);
        memcpy(zp, raw + off, blk); zp += blk; off += blk;
    }
    { unsigned ad = adler32_of(raw, rawlen);
      *zp++=(unsigned char)(ad>>24); *zp++=(unsigned char)(ad>>16);
      *zp++=(unsigned char)(ad>>8);  *zp++=(unsigned char)ad; }
    zlen = (size_t)(zp - z);

    out = (unsigned char *)malloc(8 + 25 + 12 + zlen + 12 + 64);
    op = out;
    memcpy(op, "\x89PNG\r\n\x1a\n", 8); op += 8;
    { unsigned char ih[13];
      ih[0]=(unsigned char)(w>>24); ih[1]=(unsigned char)(w>>16); ih[2]=(unsigned char)(w>>8); ih[3]=(unsigned char)w;
      ih[4]=(unsigned char)(h>>24); ih[5]=(unsigned char)(h>>16); ih[6]=(unsigned char)(h>>8); ih[7]=(unsigned char)h;
      ih[8]=8; ih[9]=2; ih[10]=0; ih[11]=0; ih[12]=0;
      op = chunk(op, "IHDR", ih, 13); }
    op = chunk(op, "IDAT", z, zlen);
    op = chunk(op, "IEND", (const unsigned char *)"", 0);
    f = fopen(path, "wb");
    fwrite(out, 1, (size_t)(op - out), f);
    fclose(f);
    free(raw); free(z); free(out);
}

int main(int argc, char **argv)
{
    int    RW = 140, RH = 47;                 /* emulate a 140x48 terminal */
    int    SX, SY;
    double t = (argc > 1) ? atof(argv[1]) : 14.0;
    float  az, el, k;
    Vec3   cam, fwd, rgt, up;
    unsigned char *img;
    int    x, y;
    double t0;

    crc_init();
    SY = 380;
    SX = (int)((double)SY * ((double)RW * 0.5) / (double)RH + 0.5);

    az  = (float)t * 0.22f;
    el  = (argc > 2) ? (float)atof(argv[2])
                     : 0.150f + 0.055f * sinf((float)t * 0.113f);
    cam = v3(CAM_DIST * cosf(el) * cosf(az),
             CAM_DIST * sinf(el),
             CAM_DIST * cosf(el) * sinf(az));
    fwd = vmul(vnorm(cam), -1.0f);
    rgt = vnorm(vcross(fwd, v3(0.0f, 1.0f, 0.0f)));
    up  = vcross(rgt, fwd);
    k   = CAM_FOV / ((float)RH * 0.5f);

    img = (unsigned char *)malloc((size_t)SX * SY * 3);
    t0 = now_sec();

    for (y = 0; y < SY; y++) {
        float vv = ((float)RH) * (((float)y + 0.5f) / (float)SY - 0.5f);
        for (x = 0; x < SX; x++) {
            float uu = ((float)RW * 0.5f) * (((float)x + 0.5f) / (float)SX - 0.5f);
            Vec3  d  = vnorm(vadd(fwd, vadd(vmul(rgt, uu * k), vmul(up, vv * k))));
            Vec3  c  = vmul(trace_ray(cam, d, t), EXPOSURE);
            unsigned char *q = img + ((size_t)y * SX + x) * 3;
            if (!(c.x > 0.0f)) c.x = 0.0f;
            if (!(c.y > 0.0f)) c.y = 0.0f;
            if (!(c.z > 0.0f)) c.z = 0.0f;
            c.x = sqrtf(c.x / (1.0f + c.x));
            c.y = sqrtf(c.y / (1.0f + c.y));
            c.z = sqrtf(c.z / (1.0f + c.z));
            q[0] = (unsigned char)(clampf(c.x, 0.0f, 1.0f) * 255.0f + 0.5f);
            q[1] = (unsigned char)(clampf(c.y, 0.0f, 1.0f) * 255.0f + 0.5f);
            q[2] = (unsigned char)(clampf(c.z, 0.0f, 1.0f) * 255.0f + 0.5f);
        }
    }
    printf("%dx%d in %.3f s\n", SX, SY, now_sec() - t0);

    write_png("flat.png", img, SX, SY);
    free(img);
    return 0;
}
