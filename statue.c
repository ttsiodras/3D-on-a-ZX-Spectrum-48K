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

#define TOTAL_FRAMES 72
#define TOTAL_POINTS (sizeof(points)/sizeof(points[0]))

void cls()
{
    memset((char *)16384, 0, 256*192/8);
    gotoxy(0,0);
}

// To fit in memory, we use 16 bits per pixel; the upper 13
// store the video RAM offset (0 - 0x1800) and the lower 3
// store the pixel offset within that byte (0-7).
unsigned precomputed[TOTAL_FRAMES][TOTAL_POINTS];

///////////////////////////////////////////////////////////////
// 3D projection.
///////////////////////////////////////////////////////////////
void precomputePoints(int angle)
{
#define SE (25L*256L + MAXX)
#define SC (20L*256L + MAXX)
#define WIDTH 256L
#define HEIGHT 192L

    long msin = sincos[angle].si;
    long mcos = sincos[angle].co;
    for(unsigned pt=0; pt<TOTAL_POINTS; pt++) {

        // Read the statue data.
        // They are scaled by 256 - you can think of them
        // as fixed point data, with 8.8 accuracy.
        int wx = points[pt][0];
        int wy = points[pt][1];
        int wz = points[pt][2];

        // Rotate the statue around the Z axis
        // Since sine and cosine are also 8.8 fixed point\
        // (i.e. scaled by 256), to get an 8.8 result
        // we need two things:
        //
        // - compute in long (8.8 x 8.8 => 16.16)
        // - scale down by 256, to truncate the result back into 8.8
        long wxnew = (mcos*wx - msin*wy)/256L;
        long wynew = (msin*wx + mcos*wy)/256L;

        // Now that we have the final X,Y,Z data,
        // project them to 2D.
        // Just look at the diagram in contrib/linear_algebra.png.
        int x = WIDTH/2L + (25L*wynew*(SE-SC)/(SE-wxnew))/256;
        int y = HEIGHT/2L + (25L*wz*(SE-SC)/(SE-wxnew))/256;

        // Store the final memory access coordinates in 16 bits/pixel:
        // - The offset within the 6K of video RAM, in the upper 13 bits
        // - The pixel (0-7) within that byte, in the lower 3 bits
        unsigned offset = zx_py2saddr(y) + (x>>3) - 0x4000;
        precomputed[angle][pt] = (offset << 3) | (x&7);
    }
}

//////////////////////////////////////////////////////////////
// Clear previous frames points, and draw new frame's points,
// using the precomputed video RAM offsets.
//////////////////////////////////////////////////////////////
void drawPoints(int angle, int old_angle)
{
    unsigned *old_data = &precomputed[old_angle][0];
    unsigned *new_data = &precomputed[angle][0];
    for(unsigned pt=0; pt<TOTAL_POINTS; pt++) {

        // Clear old pixel
        unsigned char *pixel = (unsigned char*)0x4000;
        pixel += ((*old_data & 0xFFF8) >> 3);
        unsigned char mask = 128 >> (*old_data++ & 7);
        *pixel &= ~mask;

        // Set new pixel
        pixel = (unsigned char*)0x4000;
        pixel += ((*new_data & 0xFFF8) >> 3);
        mask = 128 >> (*new_data++ & 7);
        *pixel |= mask;
    }
}

main()
{
    long frames = 0;
    long m = 0, st, total_clocks = 0;
    char angle, old_angle;

    cls();
    // Set everything to black
    zx_border(INK_BLACK);
    memset((void *)22528.0, 7, 768);
    printPaper(0);
    printInk(7);
    uint qq = in_LookupKey('q');
    printf("[-] Precomputing for %d points\n", TOTAL_POINTS);
    // Precompute the video RAM offsets and pixel coordinate
    // for each one of the rotation angles of the statue
    for(angle=0; angle<TOTAL_FRAMES; angle++) {
        gotoxy(0, 1);
        printf("[-] Frame %d/%d...\n", (int) angle, TOTAL_FRAMES);
        precomputePoints(angle);
    }
    gotoxy(0, 1);
    printf("[-] Frame %d/%d...\n", (int) angle, TOTAL_FRAMES);

    // Statistics  banner on the upper left, will show FPS
    printf("[-] Rendering...\n");
    printf("[-] Q to quit...\n");
    angle = 0;
    while(1) {
        st = clock();
        drawPoints(angle, old_angle);
        total_clocks += clock() - st;
        old_angle = angle;
        if (++angle == TOTAL_FRAMES)
            angle = 0;
        if (in_KeyPressed(qq)) {
            while (in_KeyPressed(qq));
            break;
        }
        frames++;
        if (0xF == (frames & 0xF)) {
            gotoxy(0, 4);
            printf("[-] %3.1f FPS\n", ((float)frames)/(((float)total_clocks)/CLOCKS_PER_SEC));
        }
    }
}
