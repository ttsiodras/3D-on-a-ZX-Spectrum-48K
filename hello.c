#include <stdio.h>
#include <string.h>
#include <math.h>
#include <input.h>
#include <time.h>
#include <graphics.h>
#include <conio.h>

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
const int SCREEN_DISTX = 64;
const int SCREEN_DISTY = 32;

void cls()
{
    memset((char *)16384, 0, 256*192/8);
    gotoxy(0,0);
}

///////////////////////////////////////////////////////////////
// 3D projection - make a diagram or read any 3D graphics book.
///////////////////////////////////////////////////////////////

void drawPoints()
{
    static int xx[sizeof(points)/sizeof(points[0])];
    static int yy[sizeof(points)/sizeof(points[0])];

    const int width=256;
    const int height=192;
    for(unsigned pt=0; pt<sizeof(points)/sizeof(points[0]); pt++) {

        // Clear old pixel
        unplot(xx[pt], yy[pt]);

        // Read the statue data.
        int wx = points[pt][0];
        int wy = points[pt][1];
        int wz = points[pt][2];

        // Now that we read the X,Y,Z data, project them to 2D
        int wxnew = wx+mcos; // (mcos*wx - msin*wy)/256L;
        int wynew = wy+msin; // (msin*wx + mcos*wy);
        int x = width/2L + ((wynew << 6)/(Se-wxnew));
        int y = height/2L - ((wz << 6)/(Se-wxnew));

        // Set new pixel
        plot(x, y);

        // Remember new pixel to be able to clear it in the next frame
        xx[pt] = x;
        yy[pt] = y;
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
    printf("[-] Scaling statue...\n");
    for(unsigned pt=0; pt<sizeof(points)/sizeof(points[0]); pt++) {
        points[pt][0] /= 18;
        points[pt][1] /= 9;
        points[pt][2] /= 9;
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
    while(frames<32) {
    // while(1) {
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
    cls();
    printf("[-] Rendered %ld frames in %ld clock ticks\n", frames, en-st);
}
