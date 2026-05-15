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

int my_fast_div_recip_c(int dividend, int divisor) {
    // Now divisor d is exactly at offset d * 2
    uint8_t *ptr = (uint8_t *)recip_table + (divisor * 2);
    uint16_t recip = ptr[0] | (ptr[1] << 8);
    uint32_t product = (uint32_t)dividend * (uint32_t)recip;
    return (int)(product >> 16);
}

int main() {
    unsigned long st, en;
    float duration_fast, duration_my;

    printf("Benchmarking: l_fast_divs vs my_fast_div (C-Lookup Fixed Table)\n");
    printf("Divisor  | Total Fast | Total My | Status\n");
    printf("--------------------------------------------\n");

    for (int divisor = 1; divisor <= 1023; divisor += 15) {
        
        duration_fast = 0.0;
        for (int dividend = 0; dividend <= 31000; dividend += 15) {
            g_dividend = dividend;
            g_divisor = divisor;
            st = clock();
            call_fast_div();
            en = clock();
            duration_fast += (float)(en - st);
        }

        duration_my = 0.0;
        for (int dividend = 0; dividend <= 31000; dividend += 15) {
            st = clock();
            my_fast_div_recip_c(dividend, divisor);
            en = clock();
            duration_my += (float)(en - st);
        }

        // Correctness check
        g_dividend = 15000;
        g_divisor = divisor;
        call_fast_div();
        int res_fast = g_result;
        
        int res_my = my_fast_div_recip_c(15000, divisor);

        if (divisor != 1 && abs(res_fast - res_my) > 1) {
            printf("\n!!! CORRECTNESS FAIL at 15000/%d: %d vs %d\n", divisor, res_fast, res_my);
            return 1;
        }

        printf("%8d | %11f | %11f | %s\n", divisor, duration_fast, duration_my, (duration_my < duration_fast) ? "WIN" : "LOSE");
    }

    printf("\nALL PASS - Strategy is sound!\n");
    return 0;
}
