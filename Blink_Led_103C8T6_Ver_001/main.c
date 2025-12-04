#include <stdint.h>

/* Basic register references */
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

       OPERATION:
       (1 << 0)
       - Sets AFIOEN = 1
       - Enables AFIO clock

       WHY?
       AFIO is required to modify SWJ_CFG in AFIO_MAPR.
    ============================================================*/
    RCC_APB2ENR |= (1 << 0);




    /* ============================================================
       ENABLE GPIOB CLOCK  (RCC_APB2ENR bit 3)

       BIT MAP:
       ---------------------------------------------------------------
       | 31..16 IGNORE |15|14|13|12|11|10|9|8|7|6|5|4| 3 | 2 |1|0|
       |                | | | | | | | | | | | | |IOPB|IOPA|-|AFIO|
       ---------------------------------------------------------------

       OPERATION:
       (1 << 3)
       - Sets IOPBEN = 1
       - Enables GPIOB port clock

       WHY?
       PB4 will NOT work unless GPIOB clock is ON.
    ============================================================*/
    RCC_APB2ENR |= (1 << 3);




    /* ============================================================
       DISABLE JTAG, KEEP SWD ACTIVE  (AFIO_MAPR bits 26:24)

       SWJ_CFG BIT MAP:
       -----------------------------------------------------------
       | 26 | 25 | 24 |
       |SWJ2|SWJ1|SWJ0|
       -----------------------------------------------------------

       VALUES:
       -----------------------------------------------------------
       | 000 | Full JTAG + SWD             | PB3, PB4, PB5 BUSY      |
       | 010 | JTAG Disabled, SWD Enabled  | PB4 FREE                |
       | 100 | JTAG+SWD Disabled           | NO DEBUG                |
       -----------------------------------------------------------

       PB4 = JTRST (JTAG reset pin)
       MUST disable JTAG to use PB4 as GPIO.

       STEP 1: CLEAR SWJ_CFG bits
       (~(7 << 24))
       -----------------------------------------------------------
       | 26 | 25 | 24 |
       |  0 |  0 |  0 |
       -----------------------------------------------------------

       STEP 2: SET SWJ_CFG = 010
       (2 << 24)
       -----------------------------------------------------------
       | 26 | 25 | 24 |
       |  0 |  1 |  0 |
       -----------------------------------------------------------
    ============================================================*/
    AFIO_MAPR &= ~(7 << 24);
    AFIO_MAPR |=  (2 << 24);
}






/* ================================================================
   FUNCTION: config()
   Purpose: Configure PB4 as General-Purpose Output Push-Pull
   ================================================================*/
void config(void)
{
    /* ============================================================
       CONFIGURE PB4 (GPIOB_CRL bits [19:16])

       CRL PIN BIT MAP:
       -----------------------------------------------------------
       |19|18|17|16|  PB4 config
       |CN1|CN0|MD1|MD0|
       -----------------------------------------------------------

       DESIRED CONFIG:
       MODE = 01  Output 10 MHz  
       CNF  = 00  Push-Pull  

       Combined 4-bit code:
       0 0 0 1  = 0x1

       STEP 1: CLEAR BITS
       ~(0xF << 16)
       -----------------------------------------------------------
       |19 18 17 16|
       | 0  0  0  0|  clear old config
       -----------------------------------------------------------

       STEP 2: WRITE NEW CONFIG
       (0x1 << 16)
       -----------------------------------------------------------
       |19 18 17 16|
       | 0  0  0  1|  MODE=01, CNF=00
       -----------------------------------------------------------
    ============================================================*/
    GPIOB_CRL &= ~(0xF << 16);
    GPIOB_CRL |=  (0x1 << 16);
}






/* ================================================================
   FUNCTION: delay()
   Purpose: simple software delay loop
   ================================================================*/
void delay(void)
{
    /* ------------------------------------------------------------
       Simple for-loop delay.
       No registers used ? No bit tables required.
    ------------------------------------------------------------*/
    for(int i = 200000; i > 0; i--);
}






/* ================================================================
   FUNCTION: main()
   Purpose: Toggle PB4 continuously
   ================================================================*/
int main()
{
    /* ============================================================
       INIT PB4 SYSTEM:
       - Enable AFIO
       - Enable GPIOB
       - Disable JTAG
       - Free PB4 as GPIO
    ============================================================*/
    initial();

    /* ============================================================
       CONFIG PB4 GPIO MODE:
       - Output
       - Push-Pull
       - 10 MHz
    ============================================================*/
    config();

    while(1)
    {
        /* ========================================================
           TOGGLE PB4 (GPIOB_ODR bit 4)

           ODR BIT MAP:
           -------------------------------------------------------
           |15.......6| 5 | 4 | 3 | 2 | 1 | 0 |
           | unused   |PB5|PB4|PB3|PB2|PB1|PB0|
           -------------------------------------------------------

           OPERATION: XOR
           GPIOB_ODR ^= (1 << 4)

           If PB4 = 0 ? becomes 1  
           If PB4 = 1 ? becomes 0  
        ========================================================*/
        GPIOB_ODR ^= (1 << 4);

        delay();
    }
}
