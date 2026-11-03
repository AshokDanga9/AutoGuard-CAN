#include <LPC21xx.h>
#include "delay.h"
#include "Major_Can.h"
#include "types.h"
#include "Major_candefines.h"

#define OFF   0
#define LEFT  1
#define RIGHT 2

#define LED_START 23
#define LED_MASK  (0xFF << LED_START)

CANF rxF;

volatile u8 indicator = OFF;
volatile signed char pos = 0;
volatile u8 i;

void blink_left_step(void);
void blink_right_step(void);
u8 check_can(void);


int main(void)
{
    Init_CAN1();

    IODIR1 |= LED_MASK;
    IOSET1 = LED_MASK;       // All LEDs OFF

    while(1)
    {
        /* -------- CAN RX -------- */

        if (C1GSR & (1 << RBS_BIT))
        {
            CAN1_Rx(&rxF);

            if (rxF.ID == 1)
            {
                if (rxF.Data1 == 'L')
                {
                    indicator = LEFT;
                    pos = 7;
                }
                else if (rxF.Data1 == 'R')
                {
                    indicator = RIGHT;
                    pos = 0;
                }
                else if (rxF.Data1 == 'O')
                {
                    indicator = OFF;
                    pos = 0;
                    IOSET1 = LED_MASK;
                }
            }
        }

        /* -------- LED ACTION -------- */

        if (indicator == LEFT)
        {
            blink_left_step();

            for (i = 0; i < 50; i++)
            {
                delay_ms(1);

                if (check_can())
                    break;
            }
        }

        else if (indicator == RIGHT)
        {
            blink_right_step();

            for (i = 0; i < 50; i++)
            {
                delay_ms(1);

                if (check_can())
                    break;
            }
        }

        else
        {
            IOSET1 = LED_MASK;
        }
    }
}


/* -------- CAN CHECK DURING 50ms DELAY -------- */

u8 check_can(void)
{
    CANF tempF;

    if (C1GSR & (1 << RBS_BIT))
    {
        CAN1_Rx(&tempF);

        if (tempF.ID == 1)
        {
            if (tempF.Data1 == 'L')
            {
                indicator = LEFT;
                pos = 7;
            }
            else if (tempF.Data1 == 'R')
            {
                indicator = RIGHT;
                pos = 0;
            }
            else if (tempF.Data1 == 'O')
            {
                indicator = OFF;
                pos = 0;
                IOSET1 = LED_MASK;

                return 1;
            }
        }
    }

    return 0;
}


/* -------- RIGHT: P1.23 ? P1.30 -------- */

void blink_right_step(void)
{
    IOSET1 = LED_MASK;

    IOCLR1 = (1 << (LED_START + pos));

    pos++;

    if (pos >= 8)
        pos = 0;
}


/* -------- LEFT: P1.29 ? P1.23 ? P1.30 -------- */

void blink_left_step(void)
{
    IOSET1 = LED_MASK;

    if (pos == 0)
        pos = 7;
    else
        pos--;

    IOCLR1 = (1 << (LED_START + pos));
}