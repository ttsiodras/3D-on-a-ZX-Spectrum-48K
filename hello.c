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
    const long Se = 25L*256L + maxx;
    const long Sc = 20L*256L + maxx;
    const int width = 256;
    const int height = 192;

    long msin = sincos[angle].si;
    long mcos = sincos[angle].co;

    for(unsigned pt=0; pt<sizeof(points)/sizeof(points[0]); pt++) {

        // Read the statue data.
        int wx = points[pt][0];
        int wy = points[pt][1];
        int wz = points[pt][2];

        // Now that we read the X,Y,Z data, project them to 2D
        long wxnew = (mcos*wx - msin*wy)/256L;
        long wynew = (msin*wx + mcos*wy)/256L;

        precomputed[angle][pt].x = width/2L + (25L*wynew*(Se-Sc)/(Se-wxnew))/256;
        precomputed[angle][pt].y = height/2L - (25L*wz*(Se-Sc)/(Se-wxnew))/256;

        // plot(precomputed[angle][pt].x, precomputed[angle][pt].y);
    }
}

void drawPoints(int angle, int old_angle)
{
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
    char angle, old_angle, dangle;

    cls();
    zx_border(INK_BLACK);
    memset((void *)22528.0, 7, 768);
    printPaper(0);
    printInk(7);
#ifdef MANUAL_CONTROL
    uint oo = in_LookupKey('o');
    uint pp = in_LookupKey('p');
#endif
    uint qq = in_LookupKey('q');
    printf("[-] Precomputing:\n", (int) angle);
    for(angle=0; angle<TOTAL_FRAMES; angle++) {
        gotoxy(0, 1);
        printf("[-] Frame %d/%d...\n", (int) angle, TOTAL_FRAMES);
        precomputePoints(angle);
    }
    cls();
    printf("[-] Rendering...\n");
    printf("[-] Q to quit...\n");
    angle = 0;
    dangle = 1;
    st = clock();
    //while(frames<32) {
    while(1) {
        drawPoints(angle, old_angle);
#ifdef MANUAL_CONTROL
        if (in_KeyPressed(oo))
            frames = (frames + TOTAL_FRAMES - 1)%TOTAL_FRAMES;
        else if (in_KeyPressed(pp))
            frames = (frames + 1)%TOTAL_FRAMES;
#else
        frames++;
        old_angle = angle;
        angle = angle + dangle;
        if (angle == TOTAL_FRAMES - 1)
            dangle = -1;
        else if (angle == 0)
            dangle = 1;
        if (in_KeyPressed(qq))
            break;
#endif
    }
    en = clock();
    cls();
    printf("[-] Rendered %ld frames in %ld clock ticks\n", frames, en-st);
}
