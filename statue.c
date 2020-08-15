#include <input.h>
#include <time.h>
#include <graphics.h>
#include <conio.h>

#include "sincos.h"
#include "statue.h"

#define printInk(k)          printf("\x10%c", '0'+(k))
#define printPaper(k)        printf("\x11%c", '0'+(k))
#define ELEMENTS(x)          (sizeof(x)/sizeof(x[0]))

#define SCREEN_HEIGHT 192

// The angle of rotation of the eye around the Z-axis.
// Goes from 0 up to 71 for a full circle
// (see lookup table inside sincos.h).
static int angle = 0;

void cls()
{
    memset((char *)16384, 0, 256U*SCREEN_HEIGHT/8);
    gotoxy(0,0);
}

/////////////////
// 3D projection 
/////////////////

// Inline assembly requires global symbols for visibility
// (the variables below are accessed in the Z80 asm code
//  via their C-mangled named (i.e. prefixed with '_').

// The buffer keeping the old frame's screen offset/pixel mask
unsigned char g_old_vram_offsets[3*ELEMENTS(points)];

// Lookup table for Speccy's insane screen offsets
unsigned char *ofs[SCREEN_HEIGHT];

// The scaled sin/cos for the angle being rendered in this frame
int msin, mcos;

// Too few registers in the Z80! I needed some scratch space
// to hold the computed screen Y coordinate.
int new_y;

// Inline Z80 assembly! After 4 decades, I "spoke" Z80 again today :-)
// And it was worth it - frame rate went from 5.4 to 10.1 FPS.
//
// The Z80 C compilers are nowhere near as powerful as the GCC/Clang
// world. Read this to see why:
//
//    https://retrocomputing.stackexchange.com/questions/6095/
//
// ...ergo, the need for manually written ASM.
// Quite the puzzle, this was - but I pulled it off :-)
//
// Here's the original C code, for reference:
//
// void drawPoints_in_C(int angle)
// {
//     static int old_xx[ELEMENTS(points)];
//     static int old_yy[ELEMENTS(points)];
// 
//     int msin = sincos[angle].si;
//     int mcos = sincos[angle].co;
//     for(unsigned i=0; i<ELEMENTS(points); i++) {
// 
//         // Clear old pixel
//         unplot_callee(old_xx[i], old_yy[i]);
// 
//         // Project to 2D. z88dk generated code speed is
//         // greatly improved by inlining everything.
//         int wxnew = points[i][0]-mcos;
//         int x = 128 + ((points[i][2]+msin)/wxnew);
//         int y = 96 - (points[i][1]/wxnew);
// 
//         // Set new pixel
//         plot_callee(x, y);
// 
//         // Remember new pixel to be able to clear it in the next frame
//         old_xx[i] = x;
//         old_yy[i] = y;
//     }
// }

void drawPoints(int angle)
{
    msin = sincos[angle].si;
    mcos = sincos[angle].co;
#asm
    extern l_fast_divs_16_16x16
    push bc
    push de
    push hl
    push af
    ld b, 153 ; ELEMENTS(points)
    ld hl, _g_old_vram_offsets
    exx
    ld hl, _points
    ld bc, (_mcos)
    exx
loop_point:
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Clear the old frames pixel
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ld e, (hl)
    inc hl
    ld d, (hl) ; DE is now pointing to old pixel's offset
    inc hl
    ld a, (hl) ; A is now the old pixel's mask (e.g. 64)
    dec hl
    dec hl ; hl back to g_old_vram_offsets[pt*3], to write new values into
    ex de, hl
    xor 255
    and (hl)
    ld (hl), a ; Clear pixel
    ld hl, de

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Switch to helper register set
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    exx ; hl now set to &points[i][0]

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;
    ; Compute and set the new pixel via...
    ;
    ; wxnew = old_X-mcos
    ; y = 96 - (old_Z/wxnew)
    ; x = 128 + ((old_Y+msin)/wxnew)
    ;
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; First, compute the common dividend (wxnew)
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    ld e, (hl)
    inc hl
    ld d, (hl) ; de has old_X
    inc hl
    ex de, hl ; hl has old_X, de has &old_Z
    or a ; clear carry
    sbc hl, bc ; hl is now old_X-mcos => wxnew
    ex de, hl ; hl has &old_Z, de has wxnew
    push bc ; stack = [mcos]
    push de ; stack = [wxnew, mcos]

    ;;;;;;;;;;;;;;;;;;;;;;
    ; Computing screen Y
    ;;;;;;;;;;;;;;;;;;;;;;

    ld e, (hl)
    inc hl
    ld d, (hl) ; de <= old_Z
    inc hl     ; hl <= &old_Y
    ld bc, hl  ; bc <= &old_Y
    ld hl, de  ; hl <= old_Z
    pop de     ; de <= wxnew, stack = [mcos]
    push de    ; stack = [wxnew, mcos]
    push bc    ; stack has [&old_Y, wxnew, mcos]
    call l_fast_divs_16_16x16 ; hl <= old_Z / wxnew
    ld de, hl  ; de = old_Z / wxnew
    ld hl, 96
    or a ; clear carry
    sbc hl, de ; hl = 96 - (old_Z/wxnew), stack = [&old_Y, wxnew, mcos]
    ld (_new_y), hl    ; stack = [&old_Y, wxnew, mcos]

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Check if new_Y is within bounds
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; stack = [&old_Y, wxnew, mcos]

    ld a, h
    and 0x80 ; negative
    jnz bad_y
    ld de, 192 ; SCREEN_HEIGHT
    or a ; clear carry
    sbc hl, de ; larger than 191
    jnc bad_y

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; All good, compute the new_X
    ; Remember, formula is...
    ;
    ; x = 128 + ((old_Y+msin)/wxnew)
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; stack = [&old_Y, wxnew, mcos]

    pop hl    ; hl <= &old_Y, stack = [wxnew, mcos]
    ld bc, (_msin)
    ld e, (hl)
    inc hl
    ld d, (hl) ; de <= old_Y
    inc hl     ; hl <= &points[i+1]
    ex de, hl  ; hl <= old_Y, de <= &points[i+1]
    add hl, bc ; hl <= old_Y + msin
    ld bc, de  ; bc <= &points[i+1]
    pop de     ; de <= wxnew, stack = [mcos]
    push bc    ; stack = [&points[i+1], mcos]
    call l_fast_divs_16_16x16 ; hl = hl/de
    ld de, 128
    adc hl, de ; hl <= new_X
    ld bc, hl  ; bc <= new_X

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Compute the screen offset
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    ; stack = [&points[i+1], mcos]

    ld hl, (_new_y)

    ; I used to call zx_py2saddr here
    ; but a lookup table is 10 per cent faster
    add hl, hl   ; 2*new_y since we are indexing array of 16bit offsets
    ld de, hl
    ld hl, _ofs  ; base of the array
    add hl, de
    ld e, (hl)
    inc hl
    ld d, (hl)
    ld hl, de    ; screen offset = ofs[new_y]

    ; stack = [&points[i+1], mcos]

    ; add x >> 3
    ld a, c      ; a <= new_X
    rra          ; rra is the fastest shift in the block
    rra          ; but it has the nasty side-effect
    rra          ; of shifting in the carry from the left
    and 0x1f     ; so ignore the upper 3 bits
    add l        ; and just add the last 5 to L
    ld l, a      ; hl <= ofs[new_Y] + new_X >> 3
    ld a, c      ; a <= new_X
    ld de, hl    ; de <= ofs[new_Y] + new_X >> 3
    pop hl       ; hl <= &points[i+1], stack = [mcos]
    pop bc       ; bc is now mcos again, stack is clean
                 ; to communicate past the EXX,
                 ; shove values in the stack:
    push de      ; stack = [ofs[new_Y] + new_X >> 3]
    push af      ; stack = [new_X, ofs[new_Y] + new_X >> 3]

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Switch back to normal register set
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    exx ; hl back to g_old_vram_offsets

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Store the offset and mask for the new pixel
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    pop af      ; a = new_X
    pop de      ; de = ofs[new_Y] + new_X >> 3
    ld (hl), e  ; store computed screen offset into g_old_vram_offsets
    inc hl
    ld (hl), d
    inc hl      ; new offset written into g_old_vram_offsets

    ;;;;;;;;;;;;;;;;;;
    ; Compute the mask
    ;;;;;;;;;;;;;;;;;;
    push bc     ; store the counter of the 153 points
    and 7       ; new_X & 7
    ld b, a
    ld a, 128
    jz write_mask
shift_loop:
    rra         ; 128 >> (new_X & 7)
    djnz shift_loop

write_mask:
    ld (hl), a  ; store the mask into g_old_vram_offsets
    inc hl      ; move pointer to next slot of g_old_vram_offsets
    push hl
    ld hl, de   ; Write new pixel to screen!
    or (hl)
    ld (hl), a
    pop hl      ; hl <= g_old_vram_offsets, new slot
    pop bc      ; bc <= counter of 153 points
    jmp loop_closing

bad_y:
    ; stack = [&old_Y, wxnew, mcos]
    pop hl ; hl <= &old_Y, stack = [wxnew, mcos]
    pop bc ; useless popping of wxnew, stack = [mcos]
    pop bc ; bc is now mcos again, stack is empty
    inc hl
    inc hl ; hl now points to &points[i+1]

    exx ; back to normal register set
        ; so hl  back to g_old_vram_offsets
        ; write magic value that signifies BAD PIXEL

    inc hl         ; no need to write any offset,
    inc hl         ; since...
    ld (hl), 0x00  ; When we NOT 0 and get 0xFF in the pixel clear logic,
    inc hl         ; we will AND with it, leaving the original value untouched!

loop_closing:
    dec b
    jnz loop_point
    pop af
    pop hl
    pop de
    pop bc
#endasm
}

main()
{
    long frames = 0;
    long m = 0, st, en, total_clocks = 0;
    unsigned i = 0;

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
    #define SE (256+MAXX/16)

    for(i=0; i<ELEMENTS(points); i++) {
        int help;
        points[i][0] /= 14;
        points[i][0] = SE-points[i][0];
        points[i][1] /= 9;
        points[i][1] <<= 6;
        points[i][2] /= 9;
        points[i][2] <<= 6;

        // Another speed improvement comes from this:
        help = points[i][1];
        points[i][1] = points[i][2];
        points[i][2] = help;
        // Orientation in points array is now X, Z, Y.
        // This allows us to compute wxnew and new_Y first.
        // We can then forgo the computation of the new_X,
        // if the new_Y is out of bounds.
        //
        // Early abort=>speedup!
    }
    for(i=0; i<ELEMENTS(sincos); i++) {
        sincos[i].si /= 3;
        sincos[i].si <<= 6;
        sincos[i].co /= 3;
    }
    for(i=0; i<SCREEN_HEIGHT; i++)
        ofs[i] = zx_py2saddr(i);

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
            printf("[-] %3.1f FPS \n", ((float)frames)/(((float)total_clocks)/CLOCKS_PER_SEC));
	}
    }
    return 0;
}
