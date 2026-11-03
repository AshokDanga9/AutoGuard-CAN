#include<LPC21XX.h>
#include"types.h"
#include"Major_adc_defines.h"
#include"Major_adc.h"
#include"Major_fuel.h"
#include"delay.h"
#include"Major_Can.h"
#include"Major_candefines.h"
#include"Major_lcd.h"

f32 voltage;
u32 adcdval;
u8 fuel;

CANF txF;

int main(void)
{
    InitLCD();
    Init_CAN1();

    /* P0.27 = AIN0 = CH0 */
    initadc(0);

    txF.ID = 2;
    txF.BFV.RTR = 0;
    txF.BFV.DLC = 4;

    while(1)
    {
        /* Read fuel sensor from P0.27 / AIN0 */
        Read_ADC(0, &voltage, &adcdval);

        /* ADC value 0-1023 converted to 0-100% */
        fuel = (adcdval * 100) / 1023;

        if(fuel > 100)
            fuel = 100;

        /* Display on LCD */
        CmdLCD(0x01);
        CmdLCD(0x80);
        StrLCD("Fuel:");
        IntLCD(fuel);
        CharLCD('%');

        /* Send fuel percentage through CAN */
        txF.Data1 = fuel;
        txF.Data2 = 0;

        CAN1_Tx(txF);

        delay_ms(100);
    }
}
