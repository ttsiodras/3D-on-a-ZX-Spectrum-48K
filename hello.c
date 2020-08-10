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
    unsigned char *addr;
    unsigned char mask;
} precomputed[TOTAL_FRAMES][sizeof(points)/sizeof(points[0])];

///////////////////////////////////////////////////////////////
// 3D projection - make a diagram or read any 3D graphics book.
///////////////////////////////////////////////////////////////

void precomputePoints(int angle)
{
#define SE (25L*256L + MAXX)
#define SC (20L*256L + MAXX)
#define WIDTH 256L
#define HEIGHT 192L

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

        int x = WIDTH/2L + (25L*wynew*(SE-SC)/(SE-wxnew))/256;
        int y = HEIGHT/2L + (25L*wz*(SE-SC)/(SE-wxnew))/256;

        precomputed[angle][pt].addr = zx_py2saddr(y) + (x>>3);
        precomputed[angle][pt].mask = 128 >> (x&7);
    }
}

void drawPoints(int angle, int old_angle)
{
    for(unsigned pt=0; pt<sizeof(points)/sizeof(points[0]); pt++) {
        // Clear old pixel
        
        *precomputed[old_angle][pt].addr &= ~precomputed[old_angle][pt].mask;

        // Set new pixel
        *precomputed[angle][pt].addr |= precomputed[angle][pt].mask;
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
    uint qq = in_LookupKey('q');
    printf("[-] Precomputing:\n", (int) angle);
    for(angle=0; angle<TOTAL_FRAMES; angle++) {
        gotoxy(0, 1);
        printf("[-] Frame %d/%d...\n", (int) angle, TOTAL_FRAMES);
        precomputePoints(angle);
    }
    gotoxy(0, 1);
    printf("[-] Frame %d/%d...\n", (int) angle, TOTAL_FRAMES);
    printf("[-] Rendering...\n");
    printf("[-] Q to quit...\n");
    angle = 0;
    dangle = 1;
    st = clock();
    while(1) {
        drawPoints(angle, old_angle);
        old_angle = angle;
        angle = angle + dangle;
        if (angle == TOTAL_FRAMES - 1)
            dangle = -1;
        else if (angle == 0)
            dangle = 1;
        if (in_KeyPressed(qq)) {
            while (in_KeyPressed(qq));
            break;
        }
        en = clock();
        frames++;
        if (frames == 0xF) {
            gotoxy(0, 4);
            printf("[-] %3.1f FPS\n", ((float)frames)/(((float)en-st)/50.0));
            frames = 0;
            st = clock();
        }
    }
}
