#include <stdint.h>

/* ================================================================
   RCC REGISTERS
   These registers control peripheral clock enabling
   ================================================================*/

/* RCC_APB2ENR
   - Enables clocks for GPIO, ADC, AFIO, etc.
   - Address = 0x40021018 */
#define RCC_APB2ENR     (*(volatile uint32_t*)0x40021018)

/* RCC_APB1ENR
   - Enables clocks for USART2, TIM2, etc.
   - Address = 0x4002101C */
#define RCC_APB1ENR     (*(volatile uint32_t*)0x4002101C)

/* RCC_CFGR
   - Used to configure system clocks   RCC_CFGR = Clock Configuration Register
   - Here used to set ADC prescaler
   - Address = 0x40021004 
		This register controls:
														System clock source
														AHB / APB prescalers*/
#define RCC_CFGR        (*(volatile uint32_t*)0x40021004)


/* ================================================================
   GPIOA REGISTERS
   GPIOA used for:
   - PA2  -> USART2 TX
   - PA3  -> USART2 RX
   - PA4  -> ADC input
   ================================================================*/

/* GPIOA_CRL
   - Controls configuration of PA0 to PA7
   - Address = 0x40010800 */
#define GPIOA_CRL       (*(volatile uint32_t*)0x40010800)


/* ================================================================
   USART2 REGISTERS
   Used for serial communication with Tera Term
   ================================================================*/

/* USART2_SR  : Status register (TXE, RXNE flags) */
#define USART2_SR       (*(volatile uint32_t*)0x40004400)

/* USART2_DR  : Data register for transmit/receive */
#define USART2_DR       (*(volatile uint32_t*)0x40004404)

/* USART2_BRR : Baud rate register */
#define USART2_BRR      (*(volatile uint32_t*)0x40004408)

/* USART2_CR1 : Control register (TX, RX, USART enable) */
#define USART2_CR1      (*(volatile uint32_t*)0x4000440C)


/* ================================================================
   ADC1 REGISTERS
   Used to read analog voltage from potentiometer
   ================================================================*/

/* ADC1_SR   : Status register (EOC flag) */
#define ADC1_SR         (*(volatile uint32_t*)0x40012400)

/* ADC1_CR2  : Control register (ADON, CONT, CAL, SWSTART) */
#define ADC1_CR2        (*(volatile uint32_t*)0x40012408)

/* ADC1_SMPR2: Sample time register for channels 0–9 */
#define ADC1_SMPR2      (*(volatile uint32_t*)0x40012410)

/* ADC1_SQR3 : Regular sequence register */
#define ADC1_SQR3       (*(volatile uint32_t*)0x40012434)

/* ADC1_DR   : Data register (conversion result) */
#define ADC1_DR         (*(volatile uint32_t*)0x4001244C)


/* ================================================================
   GLOBAL VARIABLES
   ================================================================*/

/* Stores 12-bit ADC value (0–4095) */
uint16_t adc_val;

/* Buffer for UART transmission */
char msg[20];


/* ================================================================
   FUNCTION: delay()
   PURPOSE : Simple blocking software delay
   ================================================================*/
void delay(int t)
{
    /* Loop runs t × 1000 times (rough time delay) */
    for (int i = 0; i < t * 1000; i++);
}


/* ================================================================
   FUNCTION: UART2_Init()
   PURPOSE : Initialize USART2 for serial communication
   CONFIG  :
       Baud rate : 9600
       Data      : 8-bit
       Parity    : None
       Stop bits : 1
   ================================================================*/
void UART2_Init(void)
{
    /* Enable GPIOA clock (for PA2, PA3) */
	
/*BIT MAP:
-----------------------------------------------------------------------
| 31..16 IGNORE | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
-----------------------------------------------------------------------
|               |ADC3|USART1|TIM8|SPI1|TIM1|ADC2|ADC1|IOPG|IOPF|IOPE|IOPD|IOPC|IOPB|IOPA| - |AFIO|
-----------------------------------------------------------------------
*/

    RCC_APB2ENR |= (1 << 2);
/*BIT MAP:
-----------------------------------------------------------------------
| 31..24 IGNORE | 23 | 22 | 21 | 20 | 19 | 18 | 17 | 16 | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7..0 |
-----------------------------------------------------------------------
|                |DAC |PWR |BKP |CAN |  - |USB |USART2|USART3|TIM7|TIM6|TIM5|TIM4|TIM3|TIM2| WWDG |  -  |
-----------------------------------------------------------------------*/

    /* Enable USART2 clock */
    RCC_APB1ENR |= (1 << 17);

    /* ------------------------------------------------------------
       PA2 -> USART2_TX
       MODE = 11 (50 MHz output)
       CNF  = 10 (Alternate Function Push-Pull)
    ------------------------------------------------------------ */
    GPIOA_CRL &= ~(0xF << 8);
    GPIOA_CRL |=  (0xB << 8);

    /* ------------------------------------------------------------
       PA3 -> USART2_RX
       MODE = 00
       CNF  = 01 (Floating input)
    ------------------------------------------------------------ */
    GPIOA_CRL &= ~(0xF << 12);
    GPIOA_CRL |=  (0x4 << 12);

    /* ------------------------------------------------------------
       Baud Rate Calculation:
       USARTDIV = PCLK1 / (16 × Baudrate)
       PCLK1   = 36 MHz
       Baud    = 9600

       USARTDIV = 36,000,000 / (16 × 9600)
                ˜ 234.375

       Mantissa = 234
       Fraction = 0.375 × 16 ˜ 6

       BRR = (234 << 4) | 6 = 0xEA6
    ------------------------------------------------------------ */
    USART2_BRR = 0xEA6;

    /* Enable USART */
    USART2_CR1 |= (1 << 13);

    /* Enable Transmitter */
    USART2_CR1 |= (1 << 3);

    /* Enable Receiver */
    USART2_CR1 |= (1 << 2);
}


/* ================================================================
   FUNCTION: UART2_SendChar()
   PURPOSE : Send one character over USART2
   ================================================================*/
void UART2_SendChar(char c)
{
    /* Wait until transmit buffer is empty */
    while (!(USART2_SR & (1 << 7)));

    /* Load data into data register */
    USART2_DR = c;
}


/* ================================================================
   FUNCTION: UART2_SendString()
   PURPOSE : Send string over USART2
   ================================================================*/
void UART2_SendString(char *s)
{
    /* Send characters one by one */
    while (*s)
        UART2_SendChar(*s++);
}


/* ================================================================
   FUNCTION: ADC_Init()
   PURPOSE : Initialize ADC1 to read PA4 (Potentiometer)
   MODE    : Continuous conversion + polling
   ================================================================*/
void ADC_Init(void)
{
    /* Enable GPIOA clock */
    RCC_APB2ENR |= (1 << 2);

    /* Enable ADC1 clock */
    RCC_APB2ENR |= (1 << 9);

    /* ------------------------------------------------------------
       Set ADC prescaler
       ADCCLK = PCLK2 / 6 = 72 / 6 = 12 MHz
       (ADC clock must be <= 14 MHz)
    ------------------------------------------------------------ */
    RCC_CFGR &= ~(3 << 14);
    RCC_CFGR |=  (2 << 14);

    /* ------------------------------------------------------------
       Configure PA4 as Analog Input
       MODE = 00
       CNF  = 00
    ------------------------------------------------------------ */
    GPIOA_CRL &= ~(0xF << 16);

    /* ------------------------------------------------------------
       Set sample time for channel 4
       239.5 cycles for better accuracy
    ------------------------------------------------------------ */
    ADC1_SMPR2 |= (7 << 12);

    /* Select ADC channel 4 */
    ADC1_SQR3 = 4;

    /* Enable software trigger */
    ADC1_CR2 |= (1 << 20);

    /* Enable continuous conversion mode */
    ADC1_CR2 |= (1 << 1);

    /* Turn ON ADC */
    ADC1_CR2 |= (1 << 0);
    delay(10);

    /* Reset calibration */
    ADC1_CR2 |= (1 << 3);
    while (ADC1_CR2 & (1 << 3));

    /* Start calibration */
    ADC1_CR2 |= (1 << 2);
    while (ADC1_CR2 & (1 << 2));

    /* Start ADC conversions */
    ADC1_CR2 |= (1 << 0);
}


/* ================================================================
   FUNCTION: ADC_Read()
   PURPOSE : Read ADC value using polling
   ================================================================*/
uint16_t ADC_Read(void)
{
    /* Wait until conversion complete (EOC = 1) */
    while (!(ADC1_SR & (1 << 1)));

    /* Read converted value */
    return ADC1_DR;
}


/* ================================================================
   FUNCTION: int_to_str()
   PURPOSE : Convert integer to ASCII string
   ================================================================*/
void int_to_str(uint16_t val, char *buf)
{
    int i = 0;
    int j = 0;
    char temp[6];

    if (val == 0)
        buf[i++] = '0';
    else
    {
        /* Extract digits */
        while (val)
        {
            temp[j++] = (val % 10) + '0';
            val /= 10;
        }

        /* Reverse digits */
        while (j)
            buf[i++] = temp[--j];
    }

    /* Append newline */
    buf[i++] = '\r';
    buf[i++] = '\n';
    buf[i] = '\0';
}


/* ================================================================
   MAIN FUNCTION
   PURPOSE : Continuously read ADC value and print on Tera Term
   ================================================================*/
int main(void)
{
    /* Initialize UART and ADC */
    UART2_Init();
    ADC_Init();

    /* Send header message */
    UART2_SendString("ADC Pot Value:\r\n");

    while (1)
    {
        /* Read potentiometer value */
        adc_val = ADC_Read();

        /* Convert ADC value to string */
        int_to_str(adc_val, msg);

        /* Send value to Tera Term */
        UART2_SendString(msg);

        /* Delay between readings */
        delay(500);
    }
}







/*
Explanation:

#define RCC_CFGR        (*(volatile uint32_t*)0x40021004)


RCC_CFGR = Clock Configuration Register
This register controls:
System clock source
AHB / APB prescalers

In STM32F103, ADC clock is derived from PCLK2.
IMPORTANT hardware rule
ADC clock must be = 14 MHz
Your MCU normally runs:
System clock = 72 MHz
PCLK2 = 72 MHz
If ADC directly uses 72 MHz ADC malfunctions

 Solution  Divide ADC clock
ADC prescaler bits are in RCC_CFGR:
RCC_CFGR bits [15:14] ? ADCPRE
| Bits | Meaning                |
| ---- | ---------------------- |
| 00   | PCLK2 / 2              |
| 01   | PCLK2 / 4              |
| 	10 | **PCLK2 / 6 (12 MHz)** |
| 11   | PCLK2 / 8              |










#define GPIOA_CRL (*(volatile uint32_t*)0x40010800)
GPIOA_CRL = GPIO Port A Configuration Register Low
Mode & configuration of:
PA0 to PA7

PA2 USART2_TX
PA3 USART2_RX
 PA4  ADC input
 
ADC input pin must be configured as ANALOG.
For ADC operation, GPIO must be in analog input mode.
| Register    | Pins       |
| ----------- | ---------- |
|   GPIOx_CRL | PA0 – PA7  |
| GPIOx_CRH   | PA8 – PA15 |







#define USART2_SR       (*(volatile uint32_t*)0x40004400)
USART2_SR — STATUS REGISTER 
USART needs a way to talk back to software.

Without this:
Data may be lost
Bytes overwritten
Corrupted output

| Bit | Flag | Meaning               |
| --- | ---- | --------------------- |
| 7   | TXE  | Transmit buffer empty |
| 6   | TC   | Transmission complete |
| 5   | RXNE | Received data ready   |
| 3   | ORE  | Overrun error         |

#define USART2_DR       (*(volatile uint32_t*)0x40004404)
USART2_DR — DATA REGISTER ??

Why this register exists
This is where data physically moves.
Write to DR ? transmit byte
Read from DR ? receive byte


#define USART2_BRR      (*(volatile uint32_t*)0x40004408)
USART2_BRR — BAUD RATE REGISTER ??

Why this register exists
UART communication depends on timing.
BRR tells USART:
“How fast should I send each bit?”

#define USART2_CR1 (*(volatile uint32_t*)0x4000440C)
USART2_CR1 — CONTROL REGISTER ??
| Bit | Name | Purpose            |
| --- | ---- | ------------------ |
| 13  | UE   | Enable USART       |
| 3   | TE   | Enable transmitter |
| 2   | RE   | Enable receiver    |







#define ADC1_SR         (*(volatile uint32_t*)0x40012400)
#define ADC1_CR2        (*(volatile uint32_t*)0x40012408)
#define ADC1_SMPR2      (*(volatile uint32_t*)0x40012410)
#define ADC1_SQR3       (*(volatile uint32_t*)0x40012434)
#define ADC1_DR         (*(volatile uint32_t*)0x4001244C)

CR2   ? makes ADC run
SMPR2 ? improves signal quality
SQR3  ? chooses input pin
SR    ? tells "done"
DR    ? gives result

ADC hardware works like this internally:

Analog Pin ? Sampling ? Conversion ? Result Register
                    ?
            Configuration & Control
						

ADC1_CR2 — CONTROL REGISTER (ADC ENGINE) ??

| Bit | Name    | Purpose           |
| --- | ------- | ----------------- |
| 0   | ADON    | Enable ADC        |
| 1   | CONT    | Continuous mode   |
| 2   | CAL     | Start calibration |
| 3   | RSTCAL  | Reset calibration |
| 20  | EXTTRIG | Enable trigger    |
| 22  | SWSTART | Start conversion  |


 ADC1_SQR3 — CHANNEL SELECTION / Sequence Register ??  ADC1_IN4
| Register | Purpose                    |
| -------- | -------------------------- |
| SQR3     | Conversions 1–6            |
| SQR2     | Conversions 7–12           |
| SQR1     | Conversions 13–16 + length |



ADC1_SMPR2 — SAMPLE TIME REGISTER ?

ADC must know:
“How long should I sample the voltage before converting?”
Sampling too fast = noise
Sampling too slow = delay\

| Channel Range | Register    |
| ------------- | ----------- |
| **0–9**       | ? **SMPR2** |
| 10–17         | SMPR1       |



| Bit | Name     | Description                     |
| --- | -------- | ------------------------------- |
| 15  | ADC3     | ADC3 clock enable               |
| 14  | USART1   | USART1 clock enable             |
| 13  | TIM8     | Timer 8 clock enable            |
| 12  | SPI1     | SPI1 clock enable               |
| 11  | TIM1     | Timer 1 clock enable            |
| 10  | ADC2     | ADC2 clock enable               |
| 9   | ADC1     | ADC1 clock enable               |
| 8   | IOPG     | GPIOG clock enable              |
| 7   | IOPF     | GPIOF clock enable              |
| 6   | IOPE     | GPIOE clock enable              |
| 5   | IOPD     | GPIOD clock enable              |
| 4   | IOPC     | GPIOC clock enable              |
| 3   | IOPB     | GPIOB clock enable              |
| ? 2 | **IOPA** | **GPIOA clock enable** ?        |
| 1   | –        | Reserved                        |
| 0   | AFIO     | Alternate function clock enable |*/

