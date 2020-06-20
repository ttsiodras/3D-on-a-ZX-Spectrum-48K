#include <stdio.h>
#include <string.h>
#include <math.h>
#include <input.h>

#include "sincos.h"
#include "statue.h"

char *g_screen = (char *)16384;

// The angle of rotation in the Z-axis. Goes from 0 up to 71 for a full circle
// (see lookup table inside sincos.h).
static int angle = 0;

// Sine and cosine of the rotation angle (in fixed point 24.8)
static long msin = 0, mcos = 0;
static long msin_old = 0, mcos_old = 0;

// Distance from the statue (in fixed point 24.8)
const long Se = 25*256 + maxx;
const long Sc = 20*256 + maxx;

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
    for(unsigned pt=0; pt<sizeof(points)/sizeof(points[0]); pt++) {
        const int width=256;
        const int height=192;

        // Read the statue data.
        int wx = points[pt][0];
        int wy = points[pt][1];
        int wz = points[pt][2];

        // Now that we read the X,Y,Z data, project them to 2D
        int wxnew = (mcos_old*wx - msin_old*wy)/256L;
        int wynew = (msin_old*wx + mcos_old*wy)/256L;
        int x = width/2L + (wynew*(Se-Sc)/(Se-wxnew))/16;
        if (x>=0 && x<width) {
            int y = height/2L - (wz*(Se-Sc)/(Se-wxnew))/16;
            if (y>=0 && y<height) {
                uchar *target = zx_py2saddr(y) + (x>>3);
                *target &= ~(128 >> (x&7));
            }
        }
        // Now that we read the X,Y,Z data, project them to 2D
        wxnew = (mcos*wx - msin*wy)/256L;
        wynew = (msin*wx + mcos*wy)/256L;
        x = width/2L + (wynew*(Se-Sc)/(Se-wxnew))/16;
        if (&& x>=0 && x<width) {
            int y = height/2L - (wz*(Se-Sc)/(Se-wxnew))/16;
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

    cls();
    // for(int i=0; i<72; i++) {
    //     int y = (sincos[i].si + 256) >> 2;
    //     int x = (sincos[i].co + 256) >> 1;
    //     myplot(x, y);
    //     // myplot(i<<1, i);
    //     // in_Pause(100);
    // }
    while(1) {
        // Rotate by 5 degrees on each iteration (360/72)
        angle = frames%72;
        // Recompute sin/cos from the lookup table
        msin = sincos[angle].si;
        mcos = sincos[angle].co;
        drawPoints();
        frames = (frames + 1)%72;
        msin_old = msin;
        mcos_old = mcos;
    }
}
