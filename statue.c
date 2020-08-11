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

unsigned char g_old_vram_offsets[3*ELEMENTS(points)];
int msin;
int mcos;
int scratch;
int new_x;
int new_y;

#define SE (256+MAXX/16)

void drawPoints(int angle)
{
    uchar *dest;
    msin = sincos[angle].si;
    mcos = sincos[angle].co;
#asm
    push bc
    push de
    push hl
    push af
    ld b, 153
    ld hl, _g_old_vram_offsets
    exx
    ld hl, _points
    ld bc, (_mcos)
    exx
loop_point:
    ld e, (hl)
    inc hl
    ld d, (hl) ; DE is now pointing to old pixel's offset
    inc hl
    ld a, (hl) ; A is now the old pixel's mask (e.g. 64)
    inc hl
    xor 255
    ld c, a
    ld a, (de)
    and c
    ld (de), a ; Clear pixel

    ; OK, now to set the new pixel
    exx ; hl now points to the ... points
    ld e, (hl)
    inc hl
    ld d, (hl) ; de has points[i][0]
    ex de, hl ; hl has points[i][0], de has &points[i][1]
    or a ; clear carry
    sbc hl, bc ; hl is now points[i][0] - mcos = wxnew
    ex de, hl ; hl has &points[i][1], de has wxnew

    push bc ; save bc (mcos)
    push de ; save de (wxnew)
    ld bc, (_msin)
    ld e, (hl)
    inc hl
    ld d, (hl) ; de has points[i][1]
    inc hl     ; hl has &points[i][2]
    ld (_scratch), hl
    ld hl, de  ; hl has points[i][1]
    add hl, bc ; hl = points[i][1] + msin
    pop de     ; de = wxnew
    push de    ; save wxnew in stack, we will need it again
    call l_div ; hl = hl/de
    ld de, 128
    adc hl, de
    ld (_new_x), hl
    pop de ; de is wxnew again

    ld hl, (_scratch)





    pop de ; de was wxnew again
    pop bc ; bc is now mcos again

    exx ; back to normal register set
    dec b
    jnz loop_point
    pop af
    pop hl
    pop de
    pop bc
#endasm
    dest = &g_old_vram_offsets[0];
    for(unsigned i=0; i<ELEMENTS(points); i++) {

        // Project to 2D. z88dk generated code speed is
        // greatly improved by inlining everything.
        int wxnew = points[i][0]-mcos;
        int x = 128 + ((points[i][1]+msin)/wxnew);
        int y = 96 - (points[i][2]/wxnew);
        if (y<0 || y>191) {
            *dest++ = 0x00;
            *dest++ = 0x40;
            *dest++ = 0x80;
            continue;
        }

        // Set new pixel
        uchar *offset = zx_py2saddr(y) + (x>>3);
        uchar mask = 128 >> (x&7);
        *offset |= mask;

        // Remember new pixel to be able to clear it in the next frame
        *dest++ = ((unsigned)offset) & 0xFF;
        *dest++ = (((unsigned)offset) & 0xFF00) >> 8;
        *dest++ = mask;
    }
}

main()
{
    long frames = 0;
    long m = 0, st, en, total_clocks = 0;

    //////////////
    // Banner info
    //////////////
    cls();
    zx_border(INK_BLACK);
    memset((void *)22528.0, 7, 768);
    printPaper(0);
    printInk(3);
    printf("[-] %d points...\n", ELEMENTS(points));
    printf("[-] Rendering...\n");
    printf("[-] Q to quit...\n");

    ///////////////////////////////////////////////////
    // To make results fit in 16 bit, and avoid scaling
    // in the drawPoints loop, I precompute here all that
    // I can move out of the inner loop in drawPoints.
    // Yes, lots of magic constants :-)
    ///////////////////////////////////////////////////
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

    // Q will quit.
    uint qq = in_LookupKey('q');

    // Here we go...
    while(1) {
        if (in_KeyPressed(qq))
            break;
        // Rotate by 5 degrees on each iteration (360/72)
        st = clock();
        drawPoints((int)(frames%72L));
        en = clock();
        total_clocks += (en-st);
        // Update FPS info.
        frames++;
        if (0xF == (frames & 0xF)) {
            gotoxy(0, 3);
            printInk(3);
            printf("[-] %3.1f FPS\n", ((float)frames)/(((float)total_clocks)/CLOCKS_PER_SEC));
	}
    }
}
