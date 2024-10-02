/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stc15.h>

#ifdef BUZZER_PIN_X4
    #define BUZZER_PIN P3
    #define BUZZER_PIN_MASK (0x0F)
    #define INIT_BUZZER_PIN P3M0 |= BUZZER_PIN_MASK; P3M1 &= 0xF0

    static unsigned char _buzzer_high = 0;
#else
    #define BUZZER_PIN P3_3
    #define INIT_BUZZER_PIN P3M0 |= (1 << 3); P3M1 &= ~(1 << 3)
#endif

#ifdef STC8G1KXX
    #define TEST_PIN P5_4
    #define INIT_TEST_PIN P5M0 &= ~(1 << 4); P5M1 &= ~(1 << 4); P5_4 = 1
#else
    #define TEST_PIN P3_4
    #define INIT_TEST_PIN P3M0 &= ~(1 << 4); P3M1 &= ~(1 << 4); P3_4 = 1
#endif

static unsigned char _buzz = 0;
static volatile unsigned int _millis = 0;

inline void init_timer0(void) {
    PT0 = 1; /* Set Timer0 High Priority */
    AUXR &= ~0x80; /* Timer clock is 12T mode */
    TMOD &= 0xF0; /* Set timer work mode */

    unsigned int v = 0x10000 - (F_CPU / 12 / 8000); // 4000Hz buzz

    TL0 = v;
    TH0 = v >> 8;

    TF0 = 0; // Clear TF0 flag

    TR0 = 1; // Start Timer0
    ET0 = 1; // Enable Timer0 interrupt
}

void timer0_isr(void) __interrupt (TF0_VECTOR) {
#ifdef BUZZER_PIN_X4
    if (_buzz) {
        if (_buzzer_high) {
            BUZZER_PIN |= BUZZER_PIN_MASK;
        } else {
            BUZZER_PIN &= ~BUZZER_PIN_MASK;
        }
        _buzzer_high = !_buzzer_high;
    } else {
        BUZZER_PIN &= ~BUZZER_PIN_MASK;
        _buzzer_high = 0;
    }
#else
    if (_buzz) {
        BUZZER_PIN = !BUZZER_PIN;
    } else {
        BUZZER_PIN = 0;
    }
#endif
}

static void delay_ms(unsigned char ms)
{
    do {
        unsigned int i = F_CPU / 17000;
        while (--i);
    } while (--ms);
}

inline void buzz(unsigned char times) {
    for (unsigned char i = 0; i < times; i ++) {
        _buzz = 1;
        delay_ms(15);
        _buzz = 0;
        delay_ms(85);
    }
}

inline void sleep(void) {
    // put to sleep mode
    PCON = 0x02;

    // Waked up
    __asm
        nop
        nop
        nop
        nop
    __endasm;
}

static unsigned char rand8reg = 123;

static unsigned char rand(void) {
__asm
    mov    a, _rand8reg
    jnz    rand8b
    cpl    a
    mov    _rand8reg, a
rand8b:
    anl    a, #0b10111000
    mov    c, p
    mov    a, _rand8reg
    rlc    a
    mov    _rand8reg, a
__endasm;

    return rand8reg;
}

void main(void) {
    unsigned char count_down = 0;

    INIT_TEST_PIN;
    INIT_BUZZER_PIN;
    init_timer0();

    // wake every 16s
    WKTCL = 0xFE;
    WKTCH = 0xFF;

    // enable interrupts
    EA = 1;

    while (1) {
        if (TEST_PIN == 0 || count_down == 0) {
            unsigned char r = rand();
            unsigned char buzz_count = r % 4 + 1;
            if (buzz_count == 1) buzz_count = 3;
            buzz(buzz_count);
            // min: 64s (16s * 4), max: 1072s (16s * 67)
            count_down = 3 + r % 64;
        } else {
            count_down --;
        }

        sleep();
    }
}
