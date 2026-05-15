#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

extern uint8_t recip_table[];

int g_dividend;
int g_divisor;
int g_result;

void call_fast_div() {
#asm
    push bc
    push de
    push hl
    push af

    ld hl, _g_dividend
    ld a, (hl)
    ld c, a
    inc hl
    ld a, (hl)
    ld b, a

    ld hl, _g_divisor
    ld a, (hl)
    ld e, a
    inc hl
    ld a, (hl)
    ld d, a

    ld h, b
    ld l, c

    extern l_fast_divs_16_16x16
    call l_fast_divs_16_16x16

    push hl
    ld hl, _g_result 
    pop de
    ld (hl), e
    inc hl
    ld (hl), d

    pop af
    pop hl
    pop de
    pop bc
#endasm
}

void my_fast_div_recip_lib() {
#asm
    push bc
    push de
    push hl
    push af

    ; 1 Calculate Offset: divisor * 2
    ld hl, _g_divisor
    ld e, (hl)
    inc hl
    ld d, (hl)
    ex de, hl
    add hl, hl
    ex de, hl ; de is 2 * divisor

    ; 2 Add base address of table
    ld hl, _recip_table
    add hl, de

    ; 3 Load Reciprocal into HL
    ld e, (hl)
    inc hl
    ld d, (hl) ; de is reciprocal

    ; 4 Load Dividend into hl
    ld hl, _g_dividend
    ld a, (hl)
    inc hl
    ld h, (hl)
    ld l, a

    ; 5 Call l_fast_mulu_32_16x16
    extern l_fast_mulu_32_16x16
    call l_fast_mulu_32_16x16
    
    ; 6 Store high word (DE) into g_result
    ld hl, _g_result
    ld (hl), e
    inc hl
    ld (hl), d

    pop af
    pop hl
    pop de
    pop bc
#endasm
}

int main() {
    unsigned long st, en;
    float duration_fast, duration_my;

    // --- TABLE VERIFICATION ---
    // printf("Table Verification:\n");
    // for (int d = 16; d <= 16; d++) {
    //     uint8_t *ptr = (uint8_t *)recip_table + (d * 2);
    //     uint16_t val = ptr[0] | (ptr[1] << 8);
    //     printf("d=%3d: 0x%04x (%d)\n", d, val, val);
    // }
    // printf("--------------------------------------------\n");

    printf("Benchmarking: l_fast_divs vs my_fast_div (Lib-Mul)\n");
    printf("Divisor  | Total Fast | Total My | Status\n");
    printf("--------------------------------------------\n");

    int res_fast, res_my;

    for (int divisor = 2; divisor <= 1023; divisor += 15) {
        duration_fast = 0.0;
        duration_my = 0.0;
        for (int dividend = 0; dividend <= 31000; dividend += 15) {
            g_dividend = dividend;
            g_divisor = divisor;
            st = clock();
            call_fast_div();
            en = clock();
            duration_fast += (float)(en - st);
            res_fast = g_result;
            st = clock();
            my_fast_div_recip_lib();
            en = clock();
            res_my = g_result;
            duration_my += (float)(en - st);
            int delta = abs(res_fast - res_my);
            if (delta > 1) {
                printf("\n!!! CORRECTNESS FAIL at 15000/%u: %u vs %u\n", divisor, res_fast, res_my);
                return 1;
            }
        }
        float t_delta = 100.0*(duration_fast - duration_my)/duration_fast;;
        printf("%8d | %11f | %11f | %s (pct:%3d)\n", divisor, duration_fast, duration_my, (duration_my < duration_fast) ? "WIN" : "LOSE", 
            (int)t_delta);
    }

    printf("\nALL PASS - Library multiplication is sound!\n");
    return 0;
}
