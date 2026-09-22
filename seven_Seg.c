#include <lpc214x.h>

/*
 * Two-digit multiplexed common-cathode seven-segment display
 *
 * Segment wiring:
 * P0.0 = A, P0.1 = B, P0.2 = C, P0.3 = D
 * P0.4 = E, P0.5 = F, P0.6 = G, P0.7 = DP
 *
 * Digit wiring:
 * P0.8 = Left/Tens digit
 * P0.9 = Right/Units digit
 *
 * The Proteus circuit drives the digit common-cathode pins directly,
 * therefore a LOW selects a digit.
 * Segment outputs are active HIGH.
 */

#define SEGMENT_MASK              0x000000FFUL
#define TENS_SELECT               (1UL << 8)
#define UNITS_SELECT              (1UL << 9)
#define DIGIT_MASK                (TENS_SELECT | UNITS_SELECT)
#define DISPLAY_MASK              (SEGMENT_MASK | DIGIT_MASK)

#define DIGIT_DWELL_MS            2U
#define REFRESH_CYCLES_PER_COUNT  250U

/* Common-cathode codes in the order DP-G-F-E-D-C-B-A */
static const unsigned char segment_code[10] =
{
    0x3F,    /* 0 */
    0x06,    /* 1 */
    0x5B,    /* 2 */
    0x4F,    /* 3 */
    0x66,    /* 4 */
    0x6D,    /* 5 */
    0x7D,    /* 6 */
    0x07,    /* 7 */
    0x7F,    /* 8 */
    0x6F     /* 9 */
};

static void timer0_init(void);
static void delay_ms(unsigned int milliseconds);
static void show_digit(unsigned char pattern,
                       unsigned long digit_select);
static void display_number(unsigned int number);


/* Initialize Timer 0 */
static void timer0_init(void)
{
    /*
     * The supplied Keil Startup.s uses a 12 MHz crystal and PLL M=5,
     * giving CCLK = 60 MHz.
     *
     * VPBDIV = 0 makes PCLK = CCLK / 4 = 15 MHz.
     *
     * A prescaler of 14999 therefore makes T0TC increment every 1 ms.
     */

    VPBDIV = 0x00;

    T0TCR  = 0x02;       /* Reset Timer 0 */
    T0CTCR = 0x00;       /* Timer mode */
    T0PR   = 14999;
    T0TC   = 0;
    T0IR   = 0xFF;       /* Clear pending Timer 0 flags */
    T0TCR  = 0x01;       /* Start Timer 0 */
}


/* Delay in milliseconds using Timer 0 */
static void delay_ms(unsigned int milliseconds)
{
    unsigned long start;

    start = T0TC;

    while ((T0TC - start) < (unsigned long)milliseconds)
    {
        /* Busy-wait while Timer 0 provides a calibrated time base */
    }
}


/* Display one digit */
static void show_digit(unsigned char pattern,
                       unsigned long digit_select)
{
    /*
     * Blank both digits before changing segments
     * to prevent ghosting.
     */
    IO0SET = DIGIT_MASK;
    IO0CLR = SEGMENT_MASK;

    IO0SET = (unsigned long)pattern;
    IO0CLR = digit_select;          /* Active-LOW digit enable */

    delay_ms(DIGIT_DWELL_MS);

    IO0SET = DIGIT_MASK;            /* Blank display between digits */
}


/* Display a two-digit number */
static void display_number(unsigned int number)
{
    unsigned int tens;
    unsigned int units;

    tens  = number / 10U;
    units = number % 10U;

    show_digit(segment_code[tens], TENS_SELECT);
    show_digit(segment_code[units], UNITS_SELECT);
}


int main(void)
{
    unsigned int count;
    unsigned int refresh;

    /*
     * P0.0-P0.9 are GPIO.
     * Preserve the function of all other Port 0 pins.
     */
    PINSEL0 &= ~0x000FFFFFUL;

    IO0DIR |= DISPLAY_MASK;

    /* Start with all segments OFF and both digits disabled */
    IO0CLR = SEGMENT_MASK;
    IO0SET = DIGIT_MASK;

    timer0_init();

    while (1)
    {
        for (count = 0; count <= 99U; count++)
        {
            /*
             * 250 x (2 ms tens + 2 ms units)
             * is approximately 1 second.
             */
            for (refresh = 0;
                 refresh < REFRESH_CYCLES_PER_COUNT;
                 refresh++)
            {
                display_number(count);
            }
        }
    }
}