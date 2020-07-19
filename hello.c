#include <stdio.h>
#include <string.h>
#include <math.h>
#include <input.h>
#include <time.h>

#include "sincos.h"
#include "statue.h"

char *g_screen = (char *)16384;

// The angle of rotation in the Z-axis. Goes from 0 up to 71 for a full circle
// (see lookup table inside sincos.h).
static int angle = 0;

// Sine and cosine of the rotation angle (in fixed point 24.8)
static int msin = 0, mcos = 0;
static int msin_old = 0, mcos_old = 0;

// Distance from the statue (in fixed point 24.8)
const int Se = 16*16 + maxx/16;

// Frame counter
static int frames = 0;


void cls()
{
    memset(g_screen, 0, 256*192/8);
}

void myclear(int x, int y)
{
    uchar *target = zx_py2saddr(y) + (x>>3);
    *target &= ~(128 >> (x&7));
}

void myplot(int x, int y)
{
    uchar *target = zx_py2saddr(y) + (x>>3);
    *target |= (128 >> (x&7));
}

///////////////////////////////////////////////////////////////
// 3D projection - make a diagram or read any 3D graphics book.
///////////////////////////////////////////////////////////////

void drawPoints()
{
#define SCREEN_DIST 128
    const int width=256;
    const int height=192;
    for(unsigned pt=0; pt<sizeof(points)/sizeof(points[0]); pt++) {

        // Read the statue data.
        int wx = points[pt][0];
        int wy = points[pt][1];
        int wz = points[pt][2];

        // Now that we read the X,Y,Z data, project them to 2D
        int wxnew = wx+mcos_old; // (mcos*wx - msin*wy)/256L;
        int wynew = wy+msin_old; // (msin*wx + mcos*wy)/256L;
        int x = width/2 + (wynew*SCREEN_DIST/(Se-wxnew));
        if (x>=0 && x<width) {
            int y = height/2 - (wz*SCREEN_DIST/(Se-wxnew));
            // If the point is within the screen's range, plot it.
            if (y>=0 && y<height) {
                uchar *target = zx_py2saddr(y) + (x>>3);
                *target &= ~(128 >> (x&7));
            }
        }

        // Now that we read the X,Y,Z data, project them to 2D
        wxnew = wx+mcos; // (mcos*wx - msin*wy)/256L;
        wynew = wy+msin; // (msin*wx + mcos*wy)/256L;
        x = width/2L + (wynew*SCREEN_DIST/(Se-wxnew));
        if (x>=0 && x<width) {
            int y = height/2L - (wz*SCREEN_DIST/(Se-wxnew));
            // If the point is within the screen's range, plot it.
            if (y>=0 && y<height) {
                uchar *target = zx_py2saddr(y) + (x>>3);
                *target |= (128 >> (x&7));
            }
        }
    }
}

main()
{
    int frames = 0;
    long m = 0, st, en;

    cls();
    for(unsigned pt=0; pt<sizeof(points)/sizeof(points[0]); pt++) {
        points[pt][0] /= 16;
        points[pt][1] /= 16;
        points[pt][2] /= 16;
    }
    for(unsigned pt=0; pt<sizeof(sincos)/sizeof(sincos[0]); pt++) {
        sincos[pt].si /= 4;
        sincos[pt].co /= 4;
    }
#ifdef MANUAL_CONTROL
    uint oo = in_LookupKey('o');
    uint pp = in_LookupKey('p');
    uint qq = in_LookupKey('q');
#endif
    st = clock();
    while(frames != 71) {
        // Rotate by 5 degrees on each iteration (360/72)
        angle = frames%72;
        // Recompute sin/cos from the lookup table
        msin = sincos[angle].si;
        mcos = sincos[angle].co;
        drawPoints();
#ifdef MANUAL_CONTROL
        if (in_KeyPressed(oo))
            frames = (frames + 71)%72;
        else if (in_KeyPressed(pp))
            frames = (frames + 1)%72;
        else if (in_KeyPressed(qq))
            break;
#else
        frames = (frames + 1)%72;
#endif
        msin_old = msin;
        mcos_old = mcos;
    }
    en = clock();
    cls();
    printf("Took %ld ticks\n", en-st );
}
