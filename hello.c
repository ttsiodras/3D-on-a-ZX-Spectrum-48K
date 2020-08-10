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

#define TOTAL_POINTS (sizeof(points)/sizeof(points[0]))

// The angle of rotation in the Z-axis. Goes from 0 up to 71 for a full circle
// (see lookup table inside sincos.h).
static int angle = 0;

static int msin = 0, mcos = 0;
static int msin_old = 0, mcos_old = 0;

void cls()
{
    memset((char *)16384, 0, 256*192/8);
    gotoxy(0,0);
}

///////////////////////////////////////////////////////////////
// 3D projection - make a diagram or read any 3D graphics book.
///////////////////////////////////////////////////////////////

void drawPoints(long angle)
{
#define SE (256+MAXX/16)

    static int old_xx[TOTAL_POINTS];
    static int old_yy[TOTAL_POINTS];

    msin = sincos[angle].si;
    mcos = sincos[angle].co;
    for(unsigned pt=0; pt<TOTAL_POINTS; pt++) {

        // Clear old pixel
        unplot_callee(old_xx[pt], old_yy[pt]);

        // Read the statue data.
        int wx = points[pt][0];
        int wy = points[pt][1];
        int wz = points[pt][2];

        // Now that we read the X,Y,Z data, project them to 2D
        int wxnew = wx+mcos; // (mcos*wx - msin*wy)/256L;
        int wynew = wy+msin; // (msin*wx + mcos*wy);
        int x = 128 + ((wynew << 6)/(SE-wxnew));
        int y = 96 - ((wz << 6)/(SE-wxnew));

        // Set new pixel
        plot_callee(x, y);

        // Remember new pixel to be able to clear it in the next frame
        old_xx[pt] = x;
        old_yy[pt] = y;
    }
    msin_old = msin;
    mcos_old = mcos;
}

main()
{
    long frames = 0;
    long m = 0, st, en;

    cls();
    zx_border(INK_BLACK);
    memset((void *)22528.0, 7, 768);
    printPaper(0);
    printInk(3);
    printf("[-] %d points...\n", TOTAL_POINTS);
    for(unsigned pt=0; pt<TOTAL_POINTS; pt++) {
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
    printInk(7);
    uint qq = in_LookupKey('q');
    st = clock();
    //while(frames<32) {
    while(1) {
        if (in_KeyPressed(qq))
            break;
        // Rotate by 5 degrees on each iteration (360/72)
        drawPoints(frames%72L);

        frames++;
        en = clock();
        if (0xF == (frames & 0xF)) {
            gotoxy(0, 3);
            printInk(3);
            printf("[-] %3.1f FPS\n", ((float)frames)/(((float)en-st)/50.0));
	}
    }
}
