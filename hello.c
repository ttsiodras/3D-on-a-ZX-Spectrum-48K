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
#define ELEMENTS(x)          (sizeof(x)/sizeof(x[0]))

// The angle of rotation of the eye around the Z-axis.
// Goes from 0 up to 71 for a full circle
// (see lookup table inside sincos.h).
static int angle = 0;

void cls()
{
    memset((char *)16384, 0, 256*192/8);
    gotoxy(0,0);
}

///////////////////////////////////////////////////////////////
// 3D projection - make a diagram or read any 3D graphics book.
///////////////////////////////////////////////////////////////

void drawPoints(int angle)
{
#define SE (256+MAXX/16)

    static int old_xx[ELEMENTS(points)];
    static int old_yy[ELEMENTS(points)];

    int msin = sincos[angle].si;
    int mcos = sincos[angle].co;
    for(unsigned i=0; i<ELEMENTS(points); i++) {

        // Clear old pixel
        unplot_callee(old_xx[i], old_yy[i]);

        // Project to 2D. z88dk generated code speed is
        // greatly improved by inlining everything.
        int wxnew = points[i][0]-mcos;
        int x = 128 + ((points[i][1]+msin)/wxnew);
        int y = 96 - (points[i][2]/wxnew);

        // Set new pixel
        plot_callee(x, y);

        // Remember new pixel to be able to clear it in the next frame
        old_xx[i] = x;
        old_yy[i] = y;
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
    printInk(3);
    printf("[-] %d points...\n", ELEMENTS(points));
    // To make results fit in 16 bit, and avoid scaling
    // in the drawPoints loop, I pre-scale here
    // with magic constants.
    for(unsigned i=0; i<ELEMENTS(points); i++) {
        points[i][0] /= 18;
        points[i][0] = SE-points[i][0];
        points[i][1] /= 9;
        points[i][1] <<= 6;
        points[i][2] /= 9;
        points[i][2] <<= 6;
    }
    for(unsigned i=0; i<ELEMENTS(sincos); i++) {
        sincos[i].si /= 3;
        sincos[i].si <<= 6;
        sincos[i].co /= 3;
    }
    printf("[-] Rendering...\n");
    printf("[-] Q to quit...\n");
    printInk(7);
    uint qq = in_LookupKey('q');
    st = clock();
    while(1) {
        if (in_KeyPressed(qq))
            break;
        // Rotate by 5 degrees on each iteration (360/72)
        drawPoints((int)(frames%72L));
        // Update FPS info.
        frames++;
        en = clock();
        if (0xF == (frames & 0xF)) {
            gotoxy(0, 3);
            printInk(3);
            printf("[-] %3.1f FPS\n", ((float)frames)/(((float)en-st)/50.0));
	}
    }
}
