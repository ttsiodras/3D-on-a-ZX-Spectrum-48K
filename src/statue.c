#include <input.h>
#include <time.h>
#include <graphics.h>
#include <conio.h>

#define printInk(k)          printf("\x10%c", '0'+(k))
#define printPaper(k)        printf("\x11%c", '0'+(k))
#define ELEMENTS(x)          (sizeof(x)/sizeof(x[0]))

#define SCREEN_HEIGHT 192

/////////////////
// 3D projection
/////////////////

// compile-time created lookup table for Speccy's insane screen offsets
// (see codegen/tables_gen.py)
extern unsigned scr_ofs[192];

// compile-time created sin/cos lookup table
// (see codegen/tables_gen.py)
struct sincos_t {
    int si;
    int co;
};
extern struct sincos_t g_sincos[];

// compile-time created reciprocals lookup table
// (see codegen/recip_gen.py)
extern uint8_t recip_table[];

// compile-time created array of statue data
// (see codegen/points_gen.py)
extern int g_points_raw[];

// compile-time created count of points inside g_points_raw
extern int g_points_count;

// Inline Z80 assembly! After 4 decades, I "spoke" Z80 again :-)
// And it was worth it - frame rate went from 5.4 to 10.5 FPS.
// (re-visited 6 years later - got it up to 12.8 FPS :-)
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

    static int old_xx[256];
    static int old_yy[256];

    // The scaled sin/cos for the angle being rendered in this frame
    int msin = g_sincos[angle].si;
    int mcos = g_sincos[angle].co;
    for(unsigned i=0; i<g_points_count; i++) {

        // Clear old pixel
        unplot(old_xx[i], old_yy[i]);

        // Project to 2D. z88dk generated code speed is
        // greatly improved by inlining everything.
        int wxnew = g_points_raw[i*3] - mcos;
        int x = 128 + ((g_points_raw[i*3 + 2] + msin) / wxnew);
        int y = 96 - (g_points_raw[i*3 + 1] / wxnew);

        // Set new pixel
        plot(x, y);

        // Remember new pixel to be able to clear it in the next frame
        old_xx[i] = x;
        old_yy[i] = y;
    }
}

#else

// Inline assembly requires global symbols for visibility
// (the variables below are accessed in the Z80 asm code
//  via their C-mangled named (i.e. prefixed with '_').

// The buffer keeping the old frame's screen offset/pixel mask
unsigned char g_old_vram_offsets[3*256];

// There are global addresses that dont change while drawPoints runs;
// but they DO change per each call of drawPoints. To avoid wasting
// registers and/or stack to store them, we "burn" them inside the
// actual code of drawPoints. Self-modifying code to the rescue!
// Look for _self_modify_1 and _self_modify_2 labels inside the
// assembly code that follows.
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
    jmp real_logic

my_fast_div_recip_lib:
    bit 7, h   ; the reciprocal is never negative - its the distance
    jr z, is_positive  ; but the dividend can be negative
    xor a      ; A = 0
    sub l      ; A = -L, carry set if L != 0
    ld l, a    ; L done
    ld a, 0    ; A = 0 again
    sbc a, h   ; A = 0 - H - carry
    ld h, a    ; H done
    call l_fast_mulu_32_16x16
    ex de, hl  ; we get result in DE:HL, and want the upper 16 bits
    xor a      ; so we now need to turn the result, i.e. HL, negative
    sub l      ; just as above
    ld l, a
    ld a, 0
    sbc a, h
    ld h, a    ; returning result in HL
    ret
is_positive:
    extern l_fast_mulu_32_16x16
    call l_fast_mulu_32_16x16
    ex de, hl  ; we get result in DE:HL, and want the upper 16 bits
    ret        ; we return our (positive) HL

real_logic:
    extern l_fast_divs_16_16x16
    push bc
    push de
    push hl
    push af
    ld hl, _g_points_count
    ld b, (hl)
    ld hl, _g_old_vram_offsets
    exx
    ld hl, _g_points_raw
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
    cpl
    and (hl)
    ld (hl), a ; Clear pixel
    ex de, hl

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
    add hl, hl ; hl => 2*wxnew
    ld bc, hl
    ld hl, _recip_table
    add hl, bc
    ; Load Reciprocal into HL
    ld c, (hl)
    inc hl
    ld b, (hl) ; bc is 1/wxnew
    ld hl, bc ; hl is 1/wxnew
    ex de, hl ; hl has &old_Z, de has 1/wxnew
    push de ; stack = [1/wxnew]

    ;;;;;;;;;;;;;;;;;;;;;;
    ; Computing screen Y
    ;;;;;;;;;;;;;;;;;;;;;;

    ld e, (hl)
    inc hl
    ld d, (hl) ; de <= old_Z
    inc hl     ; hl <= &old_Y
    ld bc, hl  ; bc <= &old_Y
    ex de, hl  ; hl <= old_Z
    pop de     ; de <= 1/wxnew, stack = []
    push de    ; stack = [1/wxnew]
    push bc    ; stack has [&old_Y, 1/wxnew]
    ; call l_fast_divs_16_16x16 ; hl <= old_Z * 1/wxnew
    call my_fast_div_recip_lib ; hl <= old_Z * 1/wxnew
    ld de, hl  ; de = old_Z * 1/wxnew
    ld hl, 96
    or a ; clear carry
    sbc hl, de ; hl = 96 - (old_Z/wxnew), stack = [&old_Y, 1/wxnew]

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Check if new_Y is within bounds
    ; Reminder: stack = [&old_Y, 1/wxnew]
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    bit 7, h
    jr nz,bad_y      ; reject newY < 0
    ld a, l
    cp 191
    jr nc,bad_y      ; reject newY > 191

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; All good, compute the new_X
    ; Remember, formula is...
    ;
    ; x = 128 + ((old_Y+msin)/wxnew)
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; stack = [&old_Y, wxnew]

    pop hl    ; hl <= &old_Y, stack = [1/wxnew]
_self_modify_2:
    ld bc, 0xDEAD ; msin
    ld e, (hl)
    inc hl
    ld d, (hl) ; de <= old_Y
    inc hl     ; hl <= &g_points[i+1]
    ex de, hl  ; hl <= old_Y, de <= &g_points[i+1]
    add hl, bc ; hl <= old_Y + msin
    ld bc, de  ; bc <= &g_points[i+1]
    pop de     ; de <= 1/wxnew, stack = []
    push bc    ; stack = [&g_points[i+1]]
    push af    ; save new_y (i.e. A)
    ; call l_fast_divs_16_16x16 ; hl = hl/de
    call my_fast_div_recip_lib ; hl = hl/de
    pop af     ; restore new_y (i.e. A)
    ld de, 128
    add hl, de ; hl <= new_X
    ld bc, hl  ; bc <= new_X

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Compute the screen offset
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    ; stack = [&g_points[i+1]]

    ; I used to call zx_py2saddr here, but a lookup table is 10 per cent faster
    ; ...and no, inlining it doesnt help either:
    ; ld l, a
    ; ld a, l
    ; and $07
    ; ld h, a
    ; ld a, l
    ; and $c0
    ; rra
    ; inc a
    ; rrca
    ; rrca
    ; or h
    ; ld h, a
    ; ld a, l
    ; add a
    ; add a
    ; and $e0
    ; ld l, a

    sla a         ; 2x new_y but may not fit and trigger carry
    ld e, a       ; d is still 0 see assignement to 128 above so use the carry
    rl d          ; to make DE to be 2x new_y
    ld hl, _scr_ofs
    add hl, de
    ld e, (hl)
    inc hl
    ld d, (hl)    ; screen offset in de = scr_ofs[g_new_y]

    ; stack = [&g_points[i+1]]

    ; add x >> 3
    ld a, c      ; a <= new_X
    rra          ; rra is the fastest shift in the block
    rra          ; but it has the nasty side-effect
    rra          ; of shifting in the carry from the left
    and 0x1f     ; so ignore the upper 3 bits
    add e        ; and just add the last 5 from E
    ld e, a      ; DE <= scr_ofs[g_new_Y] + new_X >> 3
    ld a, c      ; a <= new_X
    pop hl       ; hl <= &g_points[i+1], stack = []
                 ; stack is clean, so we can now
                 ; communicate past the EXX,
                 ; shove values in the stack:
    push de      ; stack = [scr_ofs[g_new_Y] + new_X >> 3]
    push af      ; stack = [new_X, scr_ofs[g_new_Y] + new_X >> 3]

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Switch back to normal register set
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    exx ; hl back to g_old_vram_offsets

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; Store the offset and mask for the new pixel
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    pop af      ; a = new_X  , stack = [scr_ofs[g_new_Y] + new_X >> 3]
    pop de      ; de = scr_ofs[g_new_Y] + new_X >> 3,  stack = []
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
    ;
    ; lookup table gives 2% speedup over the above

    and  7
    extern _mask_table
    ld   b, _mask_table >> 8
    ld   c, a
    ld   a, (bc)

    ld (hl), a  ; store the mask into g_old_vram_offsets
    inc hl      ; move pointer to next slot of g_old_vram_offsets
    ex de, hl   ; Write new pixel to screen! Bring screen ptr back to hl
    or (hl)
    ld (hl), a
    ex de, hl   ; hl <= g_old_vram_offsets
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
    ; stack = [&old_Y, 1/wxnew]
    pop hl         ; hl <= &old_Y, stack = [1/wxnew]
    pop bc         ; popping of (useless) 1/wxnew, stack = []
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
    unsigned long m = 0, st, en;
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
    printf("%d points\n", g_points_count);
    printf("Press Q to quit\n");

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
    zx_border(INK_WHITE);
    memset((void *)22528.0, 0x38, 768);
    return 0;
}
