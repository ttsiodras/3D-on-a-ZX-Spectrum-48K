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

#define TOTAL_FRAMES 36

void cls()
{
    memset((char *)16384, 0, 256*192/8);
    gotoxy(0,0);
}

struct {
    int x;
    int y;
} precomputed[TOTAL_FRAMES][sizeof(points)/sizeof(points[0])];

///////////////////////////////////////////////////////////////
// 3D projection - make a diagram or read any 3D graphics book.
///////////////////////////////////////////////////////////////

void precomputePoints(int angle)
{
    const int Se = 16*16 + maxx/16;
    const int SCREEN_DISTX = 64;
    const int SCREEN_DISTY = 32;
    const int width=256;
    const int height=192;

    int msin = sincos[2*angle].si;
    int mcos = sincos[2*angle].co;

    for(unsigned pt=0; pt<sizeof(points)/sizeof(points[0]); pt++) {

        // Read the statue data.
        int wx = points[pt][0];
        int wy = points[pt][1];
        int wz = points[pt][2];

        // Now that we read the X,Y,Z data, project them to 2D
        int wxnew = wx+mcos; // (mcos*wx - msin*wy)/256L;
        int wynew = wy+msin; // (msin*wx + mcos*wy);
        precomputed[angle][pt].x = width/2L + ((wynew << 6)/(Se-wxnew));
        precomputed[angle][pt].y = height/2L - ((wz << 6)/(Se-wxnew));
    }
}

void drawPoints(int angle)
{
    const int width=256;
    const int height=192;
    int old_angle = angle ? (angle - 1) : (TOTAL_FRAMES - 1);

    for(unsigned pt=0; pt<sizeof(points)/sizeof(points[0]); pt++) {
        // Clear old pixel
        unplot(precomputed[old_angle][pt].x, precomputed[old_angle][pt].y);

        // Set new pixel
        plot(precomputed[angle][pt].x, precomputed[angle][pt].y);
    }
}

main()
{
    long frames = 0;
    long m = 0, st, en;
    unsigned char angle;

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
#ifdef MANUAL_CONTROL
    uint oo = in_LookupKey('o');
    uint pp = in_LookupKey('p');
#endif
    printf("[-] Precomputing");
    uint qq = in_LookupKey('q');
    for(angle=0; angle<TOTAL_FRAMES; angle++) {
        printf(".");
        precomputePoints(angle);
    }
    cls();
    printf("[-] Rendering...\n");
    printf("[-] Q to quit...\n");
    angle = 0;
    st = clock();
    //while(frames<32) {
    while(1) {
        drawPoints(angle);
#ifdef MANUAL_CONTROL
        if (in_KeyPressed(oo))
            frames = (frames + TOTAL_FRAMES - 1)%TOTAL_FRAMES;
        else if (in_KeyPressed(pp))
            frames = (frames + 1)%TOTAL_FRAMES;
#else
        frames++;
        angle++;
        if (angle == TOTAL_FRAMES)
            angle = 0;
        if (in_KeyPressed(qq))
            break;
#endif
    }
    en = clock();
    cls();
    printf("[-] Rendered %ld frames in %ld clock ticks\n", frames, en-st);
}
