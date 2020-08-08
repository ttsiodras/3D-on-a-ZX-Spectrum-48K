#include <stdio.h>
#include <string.h>
#include <math.h>
#include <input.h>
#include <time.h>
#include <graphics.h>

#include "sincos.h"
#include "statue.h"

#define printInk(k)          printf("\x10%c", '0'+(k))
#define printPaper(k)        printf("\x11%c", '0'+(k))
#define printAt(row, col)    printf("\x16%c%c", (col)+1, (row)+1)

// The angle of rotation in the Z-axis. Goes from 0 up to 71 for a full circle
// (see lookup table inside sincos.h).
static int angle = 0;

static int msin = 0, mcos = 0;
static int msin_old = 0, mcos_old = 0;

const int Se = 16*16 + maxx/16;
const int SCREEN_DISTX = 100;
const int SCREEN_DISTY = 80;

void cls()
{
    memset((char *)16384, 0, 256*192/8);
}

///////////////////////////////////////////////////////////////
// 3D projection - make a diagram or read any 3D graphics book.
///////////////////////////////////////////////////////////////

void drawPoints()
{
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
        int x = width/2 + (wynew*SCREEN_DISTX/(Se-wxnew));
        int y = height/2 - (wz*SCREEN_DISTY/(Se-wxnew));
        unplot(x, y);

        // Now that we read the X,Y,Z data, project them to 2D
        wxnew = wx+mcos; // (mcos*wx - msin*wy)/256L;
        wynew = wy+msin; // (msin*wx + mcos*wy)/256L;
        x = width/2L + (wynew*SCREEN_DISTX/(Se-wxnew));
        y = height/2L - (wz*SCREEN_DISTY/(Se-wxnew));
        plot(x, y);
    }
}

main()
{
    long frames = 0;
    long m = 0, st, en;

    cls();
    zx_border(INK_BLACK);
    memset((void *)22528.0, 7, 768);
    printPaper(0);
    printInk(7);
    printf("[-] Loading statue...\n");
    for(unsigned pt=0; pt<sizeof(points)/sizeof(points[0]); pt++) {
        points[pt][0] /= 16;
        points[pt][1] /= 16;
        points[pt][2] /= 16;
    }
    for(unsigned pt=0; pt<sizeof(sincos)/sizeof(sincos[0]); pt++) {
        sincos[pt].si /= 4;
        sincos[pt].co /= 4;
    }
    printf("[-] Rendering...\n");
    printf("[-] Q to quit...\n");
#ifdef MANUAL_CONTROL
    uint oo = in_LookupKey('o');
    uint pp = in_LookupKey('p');
#endif
    uint qq = in_LookupKey('q');
    st = clock();
    // while(frames<32) {
    while(1) {
        // Rotate by 5 degrees on each iteration (360/72)
        angle = frames%72L;
        // Recompute sin/cos from the lookup table
        msin = sincos[angle].si;
        mcos = sincos[angle].co;
        drawPoints();
#ifdef MANUAL_CONTROL
        if (in_KeyPressed(oo))
            frames = (frames + 71)%72;
        else if (in_KeyPressed(pp))
            frames = (frames + 1)%72;
#else
        frames++;
        if (in_KeyPressed(qq))
            break;
#endif
        msin_old = msin;
        mcos_old = mcos;
    }
    en = clock();
    printf("[-] Rendered %ld frames in %ld clock ticks\n", frames, en-st);
}
