06333333333333333333#include <LPC21xx.h>
#include "delay.h"
#include "Major_can.h"
#include "types.h"
#include "Major_lcd.h"

#include "pinconnectfunc.h"
#include "Major_Fuel.h"

/* -------- Defines -------- */
#define OFF   0
#define LEFT  1
#define RIGHT 2

/* -------- MMA7660 (Accelerometer) Defines -------- */
#define MMA7660_ADDR   0x4C
#define MMA7660_XOUT   0x00
#define MMA7660_YOUT   0x01
#define MMA7660_ZOUT   0x02
#define MMA7660_MODE   0x07
#define MMA7660_SR     0x08

#define CRASH_THRESHOLD 20   /* accelerometer counts, tune on hardware if needed */
#define IMPACT_DEBOUNCE 3    /* number of consecutive over-threshold reads before declaring crash */

volatile u32 percent=0;
f32 voltage;
u8 fuel_percent,i;
u32 level=0;

/* -------- Globals -------- */
volatile u8 indicator_status = OFF;
volatile u8 left_flag = 0;
volatile u8 right_flag = 0;
CANF txF;
CANF rxF;

/* -------- Airbag / accelerometer globals -------- */
u8  airbag_deployed = 0;
u16 impact_count = 0;

/* -------- Custom LCD chars -------- */
u8 left_arrow[8]  = {0x03,0x07,0x0F,0x1F,0x0F,0x07,0x03,0x01};
u8 right_arrow[8] = {0x18,0x1C,0x1E,0x1F,0x1E,0x1C,0x18,0x10};

/* -------- Prototypes -------- */
void LoadCustomChars(void);
void sendsignal(void);
void eint0_isr(void) __irq;
void eint1_isr(void) __irq;

/* -------- I2C0 prototypes (for MMA7660) -------- */
void I2C_Init(void);
void I2C_Start(void);
void I2C_Stop(void);
void I2C_Write(u8 data);
u8   I2C_Read(void);
void MMA_Write(u8 reg, u8 val);
u8   MMA_Read(u8 reg);
s8   MMA_GetAxis(u8 reg);
void MMA7660_Init(void);

int main(void)
{
    s8 x,y,z;

    Init_CAN1();
//	delay_ms(100);
    InitLCD();
	//CharLCD('A');
	//I2C_Init();
	StoreCustCharFont();
	LoadCustomChars();

    /* -------- MMA7660 init -------- */
    MMA7660_Init();

    CmdLCD(0x80);
    StrLCD(" <<< DASH BOARD >>> ");
    CmdLCD(0xC0);
    StrLCD("Indicator ");

    /* Show steady <> initially */
    CmdLCD(0xC0 + 13);
    CharLCD(5);      // <
    CmdLCD(0xC0 + 15);
    CharLCD(6);      // >

    /* -------- EINT config -------- */
    CfgPinFunc(0,1,3);   // p0.01->EINT0
    CfgPinFunc(0,14,2);   // p0.14-> EINT1

	/* Edge triggered interrupt */
    EXTMODE |= (1 << 0) | (1 << 1);

    /* Falling edge */
    EXTPOLAR &= ~((1 << 0) | (1 << 1));

    /* Clear pending interrupts */
    EXTINT = (1 << 0) | (1 << 1);

    /*  VIC configuration  */

    /* EINT0 = interrupt number 14 */
    VICVectCntl0 = (1<<5) | 14;
    VICVectAddr0 = (u32)eint0_isr;

	/* EINT1 = interrupt number 15 */
    VICVectCntl1 = (1<<5) | 15;
    VICVectAddr1 = (u32)eint1_isr;

	/* Enable EINT0 and EINT1 */
    VICIntEnable = (1<<14) | (1<<15);

  while(1)
    {
		if(left_flag)
        {
            left_flag = 0;

            /*
             * If LEFT is already ON:
             *      LEFT -> OFF
             *
             * Otherwise:
             *      OFF/RIGHT -> LEFT
             */

            indicator_status =
                (indicator_status == LEFT) ? OFF : LEFT;

            sendsignal();
        }

		/* RIGHT BUTTON EVENT */
        if(right_flag)
        {
            right_flag = 0;

            /*
             * If RIGHT is already ON:
             *      RIGHT -> OFF
             *
             * Otherwise:
             *      OFF/LEFT -> RIGHT
             */

            indicator_status =
                (indicator_status == RIGHT) ? OFF : RIGHT;

            sendsignal();
        }

        /* LEFT indicator blinking */
        if(indicator_status == LEFT)
        {
			//sendsignal();
            CmdLCD(0xC0 + 13);
            CharLCD(5);          // <
            delay_ms(300);
            CmdLCD(0xC0 + 13);
            CharLCD(' ');

            CmdLCD(0xC0 + 15);
            CharLCD(6);          // > steady
        }

        /* RIGHT indicator blink */
        else if(indicator_status == RIGHT)
        {
			//sendsignal();
            CmdLCD(0xC0 + 13);
            CharLCD(5);          // < steady

            CmdLCD(0xC0 + 15);
            CharLCD(6);          // >
            delay_ms(300);
            CmdLCD(0xC0 + 15);
            CharLCD(' ');
        } 

        /* OFF state */
        else
        {
			//sendsignal();
            CmdLCD(0xC0 + 13);
            CharLCD(5);
            CmdLCD(0xC0 + 15);
            CharLCD(6);
        }
		/*CmdLCD(0xC0 + 18);
            CharLCD(txF.Data1);    */

        delay_ms(100);

        /* -------- MMA7660 Accident / Airbag Detection -------- */
        x = MMA_GetAxis(MMA7660_XOUT);
        y = MMA_GetAxis(MMA7660_YOUT);
        z = MMA_GetAxis(MMA7660_ZOUT);

        if( (x > CRASH_THRESHOLD || x < -CRASH_THRESHOLD) ||
            (y > CRASH_THRESHOLD || y < -CRASH_THRESHOLD) ||
            (z > CRASH_THRESHOLD || z < -CRASH_THRESHOLD) )
        {
		
           /* if(impact_count < IMPACT_DEBOUNCE) 
			impact_count++;
		 	 */
           /*/ if(impact_count >= IMPACT_DEBOUNCE)
            {
				 CmdLCD(0x94+15);
				 impact_count = 0;
                //StrLCD("FUEL: ");
                IntLCD(impact_count);*/
                airbag_deployed = 1;
            //}
        }
		else
			airbag_deployed=0;
        CmdLCD(0xD4);
        if(airbag_deployed)
        {
           //impact_count = 0;
		   //airbag_deployed=0;
		    StrLCD("AIRBAG: OPENED   ");
        }
        else
        {
            StrLCD("SAFE CONDITION   ");
        }

      if (C1GSR & (1 << 0))
      {
        /****FUEL Node****/
        CAN1_Rx(&rxF);

        /*FUEL message */
        if(rxF.ID == 2)
        {

                percent =rxF.Data1;
			 /*
                CmdLCD(0x94);
                StrLCD("FUEL: ");
                IntLCD(percent);
                StrLCD("%");
			 */
			 	CmdLCD(0x94);
				StrLCD("FUEL: ");
				if(percent<10)
				{
					CharLCD(' ');
					CharLCD(' ');
					IntLCD(percent);
				}
				else if(percent<100)
				{
					CharLCD(' ');
					IntLCD(percent);
				}
				else
				{
					IntLCD(percent);  /* 100 */
				}
				StrLCD("% ");


                /*if(percent <= 0){
                         level=0;
                }
                else if(percent <= 25){
                     level=1 ;
                }
                else if(percent <= 50){
                      level=2;
                }
                else if(percent <= 75){
                       level=3;
                }
                else {
                        level=4;
                }

                CmdLCD(0x94+13);
                CharLCD(level);*/
        }		 
        }

        delay_ms(200);
    }
}

/* EINT0 : LEFT BUTTON */

void eint0_isr(void) __irq
{
    

    left_flag = 1;

    /* Clear EINT0 interrupt */
    EXTINT = (1 << 0);

    /* End of ISR */
    VICVectAddr = 0;
}


/* EINT1 : RIGHT BUTTON */

void eint1_isr(void) __irq
{
    right_flag = 1;

    /* Clear EINT1 interrupt */
    EXTINT = (1 << 1);

    /* End of ISR */
    VICVectAddr = 0;
}


/* CAN TRANSMISSION */

void sendsignal(void)
{
    txF.ID = 1;

    txF.BFV.RTR = 0;
    txF.BFV.DLC = 1;

    if(indicator_status == LEFT)
    {
        txF.Data1 = 'L';
    }
    else if(indicator_status == RIGHT)
    {
        txF.Data1 = 'R';
    }
    else
    {
        txF.Data1 = 'O';
    }

    CAN1_Tx(txF);
}
       
/* -------- Load CGRAM arrows -------- */
void LoadCustomChars(void)
{
    u8 i;

    CmdLCD(0x68);              // char 0 = <
    for(i=0;i<8;i++) CharLCD(left_arrow[i]);

    CmdLCD(0x70);              // char 1 = >
    for(i=0;i<8;i++) CharLCD(right_arrow[i]);
}

/* ======================================================
   MMA7660 accelerometer driver (I2C0, P0.2=SCL0, P0.3=SDA0)
   ====================================================== */

void I2C_Init(void)
{
    PINSEL0 |= 0x00000050;   /* P0.2 = SCL0, P0.3 = SDA0 */
    I2SCLH = 75;
    I2SCLL = 75;
    I2CONSET = 0x40;          /* enable I2C */
}													                                                                                                                                                  

void I2C_Start(void)
{
    I2CONSET = 0x20;              /* STA */
    I2CONCLR=0X08;
	while(!(I2CONSET & 0x08));    /* wait SI */
    I2CONCLR = 0x20;              /* clear STA */
}

void I2C_Stop(void)
{
    I2CONSET = 0x10;   /* STO */
    I2CONCLR = 0x08;   /* clear SI */
}

void I2C_Write(u8 data)
{
    I2DAT = data;
    I2CONCLR = 0x08;
    while(!(I2CONSET & 0x08));
}

u8 I2C_Read(void)
{
    I2CONCLR = 0x0C;              /* clear SI, NACK (single/last byte read) */
    while(!(I2CONSET & 0x08));
    return I2DAT;
}

void MMA_Write(u8 reg, u8 val)
{
    I2C_Start();
    I2C_Write(MMA7660_ADDR << 1);
    I2C_Write(reg);
    I2C_Write(val);
    I2C_Stop();
}

u8 MMA_Read(u8 reg)
{
    u8 val;

    I2C_Start();
    I2C_Write(MMA7660_ADDR << 1);
    I2C_Write(reg);

    I2C_Start();
    I2C_Write((MMA7660_ADDR << 1) | 1);
    val = I2C_Read();
    I2C_Stop();

    return val;
}

s8 MMA_GetAxis(u8 reg)
{
    u8 val = MMA_Read(reg);

    if(val & 0x40) return 0;         /* alert bit set -> data not valid, treat as no movement */
    if(val & 0x20) val |= 0xC0;      /* sign-extend 6-bit value to 8-bit */

    return (s8)val;
}

void MMA7660_Init(void)
{
    I2C_Init();
    delay_ms(20);

    MMA_Write(MMA7660_MODE, 0x00);  /* standby to configure */
    delay_ms(20);
    MMA_Write(MMA7660_SR, 0x00);    /* 120 samples/sec */
    delay_ms(20);
    MMA_Write(MMA7660_MODE, 0x01);  /* active mode */
    delay_ms(20);
}
