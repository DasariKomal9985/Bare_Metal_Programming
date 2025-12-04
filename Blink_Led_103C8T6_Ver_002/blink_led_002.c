#include <stdint.h>

/* RCC_APB2ENR: APB2 peripheral clock enable register
   Base: 0x40021000  
   Offset: 0x18  
   Address = 0x40021000 + 0x18 = 0x40021018 */
#define RCC_APB2ENR   *((volatile unsigned int *)0x40021018)

/* GPIOB_CRL: GPIOB configuration register (PB0–PB7)
   Base: 0x40010C00  
   Offset: 0x00  
   Address = 0x40010C00 + 0x00 = 0x40010C00 */
#define GPIOB_CRL     *((volatile unsigned int *)0x40010C00)

/* GPIOB_ODR: GPIOB output data register
   Base: 0x40010C00  
   Offset: 0x0C  
   Address = 0x40010C00 + 0x0C = 0x40010C0C */
#define GPIOB_ODR     *((volatile unsigned int *)0x40010C0C)

/* AFIO_MAPR: AFIO remap and debug configuration register
   Base: 0x40010000  
   Offset: 0x04  
   Address = 0x40010000 + 0x04 = 0x40010004 */
#define AFIO_MAPR     *((volatile unsigned int *)0x40010004)





/* ================================================================
   FUNCTION: initial()
   Purpose: Enable clocks + disable JTAG on PB4
   ================================================================*/
void initial(void)
{
    /* ============================================================
       ENABLE AFIO CLOCK  (RCC_APB2ENR bit 0)

       BIT MAP:
       ---------------------------------------------------------------
       | 31..16 IGNORE | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
       |                |ADC3|USART1|TIM8|SPI1|TIM1|ADC2|ADC1|IOPG|IOPF|IOPE|IOPD|IOPC|IOPB|IOPA|-|AFIO|
       ---------------------------------------------------------------
    ============================================================*/
    RCC_APB2ENR |= (1 << 0);



    /* ============================================================
       ENABLE GPIOB CLOCK  (RCC_APB2ENR bit 3)
    ============================================================*/
    RCC_APB2ENR |= (1 << 3);



    /* ============================================================
       DISABLE JTAG, KEEP SWD ACTIVE
       (AFIO_MAPR bits 26:24)
    ============================================================*/
    AFIO_MAPR &= ~(7 << 24);     // Clear SWJ_CFG bits
    AFIO_MAPR |=  (2 << 24);     // Set to 010 ? disable JTAG only
}





/* ================================================================
   FUNCTION: config()
   Purpose: Configure PB4 as General-Purpose Output Push-Pull
   ================================================================*/
void config(void)
{
    /* ============================================================
       CONFIGURE PB4 (GPIOB_CRL bits [19:16])

       MODE = 01 ? Output 10 MHz
       CNF  = 00 ? Push-Pull
    ============================================================*/
    GPIOB_CRL &= ~(0xF << 16);    
    GPIOB_CRL |=  (0x1 << 16);
}





/* ================================================================
   FUNCTION: delay(time)
   Purpose: adjustable software delay
   Input: time ? approximate delay units
   ================================================================*/
void delay(int time)
{
    /* ------------------------------------------------------------
       Basic software delay.
       Delay grows proportional to 'time' parameter.
    ------------------------------------------------------------*/
    for(int i = 0; i < time * 2000; i++); 
}





/* ================================================================
   FUNCTION: main()
   Purpose: Turn PB4 LED ON and OFF repeatedly
   ================================================================*/
int main()
{
    initial();
    config();

    while(1)
    {
        /* ========================================================
           LED ON  
           GPIOB_ODR bit 4 = 1

           BIT MAP:
           -------------------------------------------------------
           |15.......6| 5 | 4 | 3 | 2 | 1 | 0 |
           | unused   |PB5|PB4|PB3|PB2|PB1|PB0|
           -------------------------------------------------------
        ========================================================*/
        GPIOB_ODR |= (1 << 4);
        delay(1000);


        /* ========================================================
           LED OFF  
           GPIOB_ODR bit 4 = 0
        ========================================================*/
        GPIOB_ODR &= ~(1 << 4);
        delay(1000);
    }
}
