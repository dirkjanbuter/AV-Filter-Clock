#include "main.h"
#include <math.h>
#include <time.h>
#include "vector2d.h"
#include "matrix2d.h"

#define SWAP(x, y) (x ^= y ^= x ^= y)
#define ABS(x) ((x >= 0) ? x : -x)

static int _w = 0;
static int _h = 0;
static int _framecount = 0;

static int _s = 0;
static int _sizebytes = 0;
static unsigned char *_oi;

int _fps;
time_t _rawtime;


void clearcolor(unsigned int color) {
    int i;
    unsigned char ina = (color >> 24) & 0xff;
    unsigned char inr = (color >> 16) & 0xff;
    unsigned char ing = (color >> 8) & 0xff;
    unsigned char inb = color & 0xff;

    for(i = 0; i < _s; i++) {
       _oi[i*4] = inr;
       _oi[i*4] = ing;
       _oi[i*4] = inb;
       _oi[i*4] = ina;
    }
}

void blockblend(int x, int y, int w, int h, unsigned int color) {
    int i, j;
    int sx = x;
    int sy = y;
    int ex = x+w;
    int ey = y+h;
    unsigned char ina = (color >> 24) & 0xff;
    unsigned char ininva = 0xff - ina;
    unsigned char inr = (color >> 16) & 0xff;
    unsigned char ing = (color >> 8) & 0xff;
    unsigned char inb = color & 0xff;

    if(sx < 0) sx = 0;
    if(sy < 0) sy = 0;

    for(j = sy; j < ey && j < _h; j++) for(i = sx; i < ex && i < _w; i++) {
       unsigned char orgr = _oi[(j*_w+i)*4];
       unsigned char orgg = _oi[(j*_w+i)*4+1];
       unsigned char orgb = _oi[(j*_w+i)*4+2];
       unsigned char outr = (orgr * ininva + inr * ina) >> 8;
       unsigned char outg = (orgg * ininva + ing * ina) >> 8;
       unsigned char outb = (orgb * ininva + inb * ina) >> 8;
       unsigned char outa = 0xff;
       _oi[(j*_w+i)*4] = outr;
       _oi[(j*_w+i)*4+1] = outg;
       _oi[(j*_w+i)*4+2] = outb;
       _oi[(j*_w+i)*4+3] = outa;
    }
}

void pixelblend(int x, int y, int color)
{
    if(x < 0 || y < 0 || x >= _w || y >= _h)
        return;

    unsigned char ina = (color >> 24) & 0xff;
    unsigned char ininva = 0xff - ina;
    unsigned char inr = (color >> 16) & 0xff;
    unsigned char ing = (color >> 8) & 0xff;
    unsigned char inb = color & 0xff;
    int i = (y*_w+x)*4;

    _oi[i] = (_oi[i] * ininva + inr * ina) >> 8;
    _oi[i+1] = (_oi[i+1] * ininva + ing * ina) >> 8;
    _oi[i+2] = (_oi[i+2] * ininva + inb * ina) >> 8;
    _oi[i+3] = 0xff;
}

void lineblend(VECTOR2D *v1, VECTOR2D *v2, int color)
{
    int x0 = (int)(v1->x+0.5f);
    int y0 = (int)(v1->y+0.5f);
    int x1 = (int)(v2->x+0.5f);
    int y1 = (int)(v2->y+0.5f);


    int Dx = x1 - x0;
    int Dy = y1 - y0;
    int steep = (abs(Dy) >= abs(Dx));
    if (steep) {
        SWAP(x0, y0);
        SWAP(x1, y1);
        // recompute Dx, Dy after swap
        Dx = x1 - x0;
        Dy = y1 - y0;
    }
    int xstep = 1;
    if (Dx < 0) {
        xstep = -1;
        Dx = -Dx;
    }
    int ystep = 1;
    if (Dy < 0) {
        ystep = -1;
        Dy = -Dy;
    }
    int TwoDy = 2*Dy;
    int TwoDyTwoDx = TwoDy - 2*Dx; // 2*Dy - 2*Dx
    int E = TwoDy - Dx; //2*Dy - Dx
    int y = y0;
    int xDraw, yDraw;
    for (int x = x0; x != x1; x += xstep) {
        if (steep) {
            xDraw = y;
            yDraw = x;
        } else {
            xDraw = x;
            yDraw = y;
        }
        // plot
        pixelblend(xDraw, yDraw, color);

        // next
        if (E > 0) {
            E += TwoDyTwoDx; //E += 2*Dy - 2*Dx;
            y = y + ystep;
        } else {
            E += TwoDy; //E += 2*Dy;
        }
    }
}

int filtercreate(int fps) {
    _fps = fps;
    time(&_rawtime);
    return 1;
}

int filterstep(unsigned char *buffer, int w, int h, unsigned int color, char *text, int64_t framecount) {
    int i;
    VECTOR2D *v1, *v2;
    float halfw = w * 0.5f - 0.5f;
    float halfh = h * 0.5f - 0.5f;

    float angle = 0.0;
    float stepsize = 0.01;

    struct tm *timeinfo;
    time_t rawtime;


    _w = w;
    _h = h;


    _framecount = framecount;

    _s = _w * _h;
    _sizebytes = _s * 4;

    _oi = buffer;

    // outline
    stepsize = M_PI / 30.0f;
    v2 = vector2d_create(halfw*2.0f, halfh);
    for(angle=0.0f; angle < 2.0f * M_PI; angle+=stepsize) {
        v1 = vector2d_create(
            halfw+cos(angle)*halfw,
            halfh+sin(angle)*halfh
        );
        lineblend(v2, v1, color);
        vector2d_destroy(v2);
        v2 = v1;
    }
    v1 = vector2d_create(halfw*2.0f, halfh);
    lineblend(v2, v1, color);
    vector2d_destroy(v2);
    vector2d_destroy(v1);

    // markers 5 sec
    stepsize = M_PI / 6.0f;
    for(angle=0.0f; angle < 2.0f * M_PI; angle+=stepsize) {
        v1 = vector2d_create(
            halfw+cos(angle)*halfw,
            halfh+sin(angle)*halfh
        );
        v2 = vector2d_create(
            halfw+cos(angle)*(halfw-20),
            halfh+sin(angle)*(halfh-20)
        );
        lineblend(v2, v1, color);
        vector2d_destroy(v2);
        vector2d_destroy(v1);
    }

    // markers sec
    stepsize = M_PI / 30.0f;
    for(angle=0.0f; angle < 2.0f * M_PI; angle+=stepsize) {
        v1 = vector2d_create(
            halfw+cos(angle)*halfw,
            halfh+sin(angle)*halfh
        );
        v2 = vector2d_create(
            halfw+cos(angle)*(halfw-5),
            halfh+sin(angle)*(halfh-5)
        );
        lineblend(v2, v1, color);
        vector2d_destroy(v2);
        vector2d_destroy(v1);
    }



    rawtime = _rawtime + (int)((float)_framecount/(float)_fps);
    timeinfo = localtime(&rawtime);



    // hour
    angle =  ((float)(timeinfo->tm_hour%12)+(float)timeinfo->tm_min/60.0f)*(M_PI*2.0f)/12.0f + (M_PI*0.5f);
    v1 = vector2d_create(
            w-(halfw+cos(angle)*(halfw/2.0f)),
            h-( halfh+sin(angle)*(halfh/2.0f))
    );
    v2 = vector2d_create(
            halfw,
            halfh
    );
    lineblend(v2, v1, color);
    vector2d_destroy(v2);
    vector2d_destroy(v1);


    // minutes
    angle =  timeinfo->tm_min*(M_PI*2.0f)/60.0f + (M_PI*0.5f);
    v1 = vector2d_create(
            w-(halfw+cos(angle)*(halfw*0.8f)),
            h-(halfh+sin(angle)*(halfh*0.8f))
    );
    v2 = vector2d_create(
            halfw,
            halfh
    );
    lineblend(v2, v1, color);
    vector2d_destroy(v2);
    vector2d_destroy(v1);


    // secconds
    angle =  timeinfo->tm_sec*(M_PI*2.0f)/60.0f + (M_PI*0.5f);
    v1 = vector2d_create(
            w-(halfw+cos(angle)*(halfw*0.9f)),
            h-(halfh+sin(angle)*(halfh*0.9f))
    );
    v2 = vector2d_create(
            halfw,
            halfh
    );
    lineblend(v2, v1, color);
    vector2d_destroy(v2);
    vector2d_destroy(v1);



    return 1;
}
