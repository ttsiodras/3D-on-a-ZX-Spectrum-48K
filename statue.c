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

/////////////////
// 3D projection
/////////////////

// Inline assembly requires global symbols for visibility
// (the variables below are accessed in the Z80 asm code
//  via their C-mangled named (i.e. prefixed with '_').

// The buffer keeping the old frame's screen offset/pixel mask
unsigned char g_old_vram_offsets[3*ELEMENTS(g_points)];

// Lookup table for Speccy's insane screen offsets
unsigned char *g_ofs[SCREEN_HEIGHT];

// Too few registers in the Z80! I needed some scratch space
// to hold the computed screen Y coordinate. IX/IY are too slow!
int g_new_y;

// Inline Z80 assembly! After 4 decades, I "spoke" Z80 again today :-)
// And it was worth it - frame rate went from 5.4 to 10.5 FPS.
//
// The Z80 C compilers are nowhere near as powerful as the GCC/Clang
// world. Read this to see why:
//
//    https://retrocomputing.stackexchange.com/questions/6095/
//
// ...ergo, the need for manually written ASM.
// Quite the puzzle, this was - but I pulled it off :-)

// #define IN_C
#ifdef IN_C

void drawPoints(int angle)
{
    ///////////////////////////////////////////////////////////////
    // Detailed explanation of the algorithm lives in the README.md
    // See "For math nerds" section for details.
    ///////////////////////////////////////////////////////////////

    static int old_xx[ELEMENTS(g_points)];
    static int old_yy[ELEMENTS(g_points)];

    // The scaled sin/cos for the angle being rendered in this frame
    int msin = g_sincos[angle].si;
    int mcos = g_sincos[angle].co;
    for(unsigned i=0; i<ELEMENTS(g_points); i++) {

        // Clear old pixel
        unplot(old_xx[i], old_yy[i]);

        // Project to 2D. z88dk generated code speed is
        // greatly improved by inlining everything.
        int wxnew = g_points[i][0]-mcos;
        int x = 128 + ((g_points[i][2]+msin)/wxnew);
        int y = 96 - (g_points[i][1]/wxnew);

        // Set new pixel
        plot(x, y);

        // Remember new pixel to be able to clear it in the next frame
        old_xx[i] = x;
        old_yy[i] = y;
    }
}

#else

extern char self_modify_1[2];
extern char self_modify_2[2];

void drawPoints(int angle)
{
    ///////////////////////////////////////////////////////////////
    // Detailed explanation of the algorithm lives in the README.md
    // See "For math nerds" section for details.
    ///////////////////////////////////////////////////////////////

    // The scaled sin/cos for the angle being rendered in this frame
    *(int*) &self_modify_1[1] = g_sincos[angle].co;
    *(int*) &self_modify_2[1] = g_sincos[angle].si;
#asm
    extern l_fast_divs_16_16x16
    push bc
    push de
    push hl
    push af
    ld b, 153 ; ELEMENTS(g_points)
    ld hl, _g_old_vram_offsets
    exx
    ld hl, _g_points
    exx
loop_point:
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Clear the old frames pixel
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ld e, (hl)
    inc hl
    ld d, (hl) ; DE is now pointing to old pixel offset
    inc hl
    ld a, (hl) ; A is now the old pixel mask (e.g. 64)
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
_self_modify_1:
    ld bc, 0xDEAD ; this gets assigned to g_mcos on every function call
    sbc hl, bc ; hl is now old_X-mcos => wxnew
    ex de, hl ; hl has &old_Z, de has wxnew
    push de ; stack = [wxnew]

    ;;;;;;;;;;;;;;;;;;;;;;
    ; Computing screen Y
    ;;;;;;;;;;;;;;;;;;;;;;

    ld e, (hl)
    inc hl
    ld d, (hl) ; de <= old_Z
    inc hl     ; hl <= &old_Y
    ld bc, hl  ; bc <= &old_Y
    ld hl, de  ; hl <= old_Z
    pop de     ; de <= wxnew, stack = []
    push de    ; stack = [wxnew]
    push bc    ; stack has [&old_Y, wxnew]
    call l_fast_divs_16_16x16 ; hl <= old_Z / wxnew
    ld de, hl  ; de = old_Z / wxnew
    ld hl, 96
    or a ; clear carry
    sbc hl, de ; hl = 96 - (old_Z/wxnew), stack = [&old_Y, wxnew]
    ld (_g_new_y), hl    ; stack = [&old_Y, wxnew]

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Check if g_new_Y is within bounds
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; stack = [&old_Y, wxnew]

    ld a, h
    and 0x80 ; negative
    jnz bad_y
    ld a, l
    sbc 191  ; 128+64 = 192, Speccys screen height...
    jnc bad_y

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; All good, compute the new_X
    ; Remember, formula is...
    ;
    ; x = 128 + ((old_Y+msin)/wxnew)
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; stack = [&old_Y, wxnew]

    pop hl    ; hl <= &old_Y, stack = [wxnew]
_self_modify_2:
    ld bc, 0xDEAD ; msin 
    ld e, (hl)
    inc hl
    ld d, (hl) ; de <= old_Y
    inc hl     ; hl <= &g_points[i+1]
    ex de, hl  ; hl <= old_Y, de <= &g_points[i+1]
    add hl, bc ; hl <= old_Y + msin
    ld bc, de  ; bc <= &g_points[i+1]
    pop de     ; de <= wxnew, stack = []
    push bc    ; stack = [&g_points[i+1]]
    call l_fast_divs_16_16x16 ; hl = hl/de
    ld de, 128
    adc hl, de ; hl <= new_X
    ld bc, hl  ; bc <= new_X

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Compute the screen offset
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    ; stack = [&g_points[i+1]]

    ld hl, (_g_new_y)

    ; I used to call zx_py2saddr here
    ; but a lookup table is 10 per cent faster
    add hl, hl   ; 2*g_new_y since we are indexing array of 16bit offsets
    ld de, hl
    ld hl, _g_ofs  ; base of the array
    add hl, de
    ld e, (hl)
    inc hl
    ld d, (hl)
    ld hl, de    ; screen offset = g_ofs[g_new_y]

    ; stack = [&g_points[i+1]]

    ; add x >> 3
    ld a, c      ; a <= new_X
    rra          ; rra is the fastest shift in the block
    rra          ; but it has the nasty side-effect
    rra          ; of shifting in the carry from the left
    and 0x1f     ; so ignore the upper 3 bits
    add l        ; and just add the last 5 from L
    ld l, a      ; hl <= g_ofs[g_new_Y] + new_X >> 3
    ld a, c      ; a <= new_X
    ld de, hl    ; de <= g_ofs[g_new_Y] + new_X >> 3
    pop hl       ; hl <= &g_points[i+1], stack = []
                 ; stack is clean, so we can now
                 ; communicate past the EXX,
                 ; shove values in the stack:
    push de      ; stack = [g_ofs[g_new_Y] + new_X >> 3]
    push af      ; stack = [new_X, g_ofs[g_new_Y] + new_X >> 3]

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Switch back to normal register set
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    exx ; hl back to g_old_vram_offsets

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Store the offset and mask for the new pixel
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    pop af      ; a = new_X  , stack = [g_ofs[g_new_Y] + new_X >> 3]
    pop de      ; de = g_ofs[g_new_Y] + new_X >> 3,  stack = []
    ld (hl), e  ; store computed screen offset into g_old_vram_offsets
    inc hl
    ld (hl), d
    inc hl      ; new offset written into g_old_vram_offsets

    ;;;;;;;;;;;;;;;;;;
    ; Compute the mask
    ;;;;;;;;;;;;;;;;;;

    push bc     ; stack = [counter of the 153 points loop]

    ; Use a small lookup table to avoid this loop:
    ;
    ;     and 7       ; new_X & 7
    ;     ld b, a
    ;     ld a, 128
    ;     jz write_mask
    ; shift_loop:
    ;     rra         ; 128 >> (new_X & 7)
    ;     djnz shift_loop
    ; write_mask:
    ;     ...
    ; 2% speedup

    and  7  
    extern _mask_table
    ld   b, _mask_table >> 8
    ld   c, a
    ld   a, (bc)

    ld (hl), a  ; store the mask into g_old_vram_offsets
    inc hl      ; move pointer to next slot of g_old_vram_offsets
    push hl     ; stack = [ptr to next slot of g_old_vram_offsets, counter of 153 pts loop]
    ld hl, de   ; Write new pixel to screen!
    or (hl)
    ld (hl), a
    pop hl      ; hl <= ptr to new slot of g_old_vram_offsets, stack = [counter of 153pts loop]
    pop bc      ; bc <= counter of 153 points loop

loop_closing:
    dec b
    jnz loop_point
    pop af
    pop hl
    pop de
    pop bc
    ret

bad_y:
    ; stack = [&old_Y, wxnew, mcos]
    pop hl         ; hl <= &old_Y, stack = [wxnew]
    pop bc         ; popping of (useless) wxnew, stack = []
    inc hl
    inc hl         ; hl now points to &g_points[i+1]

    exx            ; back to normal register set
                   ; so hl  back to g_old_vram_offsets
                   ; write magic ZERO value that signifies BAD PIXEL

    inc hl         ; no need to write any offset,
    inc hl         ; since...
    ld (hl), 0x00  ; When we NOT 0 and get 0xFF in the pixel clear logic,
    inc hl         ; we will AND with it, leaving the original value untouched!
    jmp loop_closing
#endasm
}

#endif

main()
{
    long frames = 0;
    long m = 0, st, en;
    float total_clocks = 0.;
    unsigned i = 0;

    //////////////
    // Banner info
    //////////////
    memset((char *)16384, 0, 256U*SCREEN_HEIGHT/8);
    gotoxy(0,0);

    zx_border(INK_BLACK);
    memset((void *)22528.0, 7, 768);
    printPaper(0);
    printInk(3);
    printf("%d points\n", ELEMENTS(g_points));
    printf("Press Q to quit\n");

    ///////////////////////////////////////////////////
    // To make results fit in 16 bit, and avoid scaling
    // in the drawPoints loop, I precompute here all that
    // I can move out of the inner loop in drawPoints.
    // Yes, lots of magic constants :-)
    ///////////////////////////////////////////////////
    #define SE (256+MAXX/16)

    for(i=0; i<ELEMENTS(g_points); i++) {
        int help;
        g_points[i][0] /= 14;
        g_points[i][0] = SE-g_points[i][0];
        g_points[i][1] /= 9;
        g_points[i][1] <<= 6;
        g_points[i][2] /= 9;
        g_points[i][2] <<= 6;

        // Another speed improvement comes from this:
        help = g_points[i][1];
        g_points[i][1] = g_points[i][2];
        g_points[i][2] = help;
        // Orientation in points array is now X, Z, Y.
        // This allows us to compute wxnew and g_new_Y first.
        // We can then forgo the computation of the new_X,
        // if the g_new_Y is out of bounds.
        //
        // Early abort=>speedup!
    }
    for(i=0; i<ELEMENTS(g_sincos); i++) {
        g_sincos[i].si /= 3;
        g_sincos[i].si <<= 6;
        g_sincos[i].co /= 3;
    }
    for(i=0; i<SCREEN_HEIGHT; i++)
        g_ofs[i] = zx_py2saddr(i);

    // Q will quit.
    uint qq = in_LookupKey('q');

    // Here we go... start the clock!
    st = clock();

    while(1) {
        if (in_KeyPressed(qq))
            break;

        // Rotate by 5 degrees on each iteration (360/72)
        // frames modulo 72 is the angle of camera rotation movement;
        // i.e. the angle of rotation of the eye around the Z-axis.
        // Goes from 0 up to 71 for a full circle
        // (see lookup table inside sincos.h).
        drawPoints((int)(frames%72L));

        if (0x3F == (++frames & 0x3F)) {
            // Update FPS info.
            gotoxy(0, 2);
            // printInk(3);
            en = clock();
            total_clocks += (en-st);
            st = en;
            printf("%ld frames\n%3.1f FPS \n", frames, ((float)frames)/(total_clocks/CLOCKS_PER_SEC));
	}
    }
    return 0;
}
