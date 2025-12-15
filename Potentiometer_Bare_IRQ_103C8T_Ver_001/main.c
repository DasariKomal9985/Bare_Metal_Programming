#include <stdint.h>

/* ================================================================
   STM32F103 – ADC INTERRUPT BASED POTENTIOMETER READING
   ------------------------------------------------
   • ADC1 reads potentiometer on PA4
   • ADC conversion complete generates interrupt
   • ISR stores ADC value
   • Main loop maps ADC value (0–4095) → (0–100)
   • UART2 prints raw & mapped values
   ================================================================*/


/* ================================================================
   RCC (RESET AND CLOCK CONTROL) REGISTERS
   ------------------------------------------------
   Used to enable clocks for GPIO, ADC, USART
   ================================================================*/
#define RCC_APB2ENR     (*(volatile uint32_t*)0x40021018)
#define RCC_APB1ENR     (*(volatile uint32_t*)0x4002101C)
#define RCC_CFGR        (*(volatile uint32_t*)0x40021004)


/* ================================================================
   GPIOA REGISTERS
   ------------------------------------------------
   • PA2 → USART2_TX
   • PA3 → USART2_RX
   • PA4 → ADC input (analog)
   ================================================================*/
#define GPIOA_CRL       (*(volatile uint32_t*)0x40010800)


/* ================================================================
   USART2 REGISTERS (UART COMMUNICATION)
   ================================================================*/
#define USART2_SR       (*(volatile uint32_t*)0x40004400)
#define USART2_DR       (*(volatile uint32_t*)0x40004404)
#define USART2_BRR      (*(volatile uint32_t*)0x40004408)
#define USART2_CR1      (*(volatile uint32_t*)0x4000440C)


/* ================================================================
   ADC1 REGISTERS
   ================================================================*/
#define ADC1_SR         (*(volatile uint32_t*)0x40012400)
#define ADC1_CR1        (*(volatile uint32_t*)0x40012404)
#define ADC1_CR2        (*(volatile uint32_t*)0x40012408)
#define ADC1_SMPR2      (*(volatile uint32_t*)0x40012410)
#define ADC1_SQR3       (*(volatile uint32_t*)0x40012434)
#define ADC1_DR         (*(volatile uint32_t*)0x4001244C)


/* ================================================================
   NVIC REGISTERS
   ------------------------------------------------
   Used to enable interrupt line to CPU
   ================================================================*/
#define NVIC_ISER0      (*(volatile uint32_t*)0xE000E100)


/* ================================================================
   GLOBAL VARIABLES
   ================================================================*/
volatile uint16_t adc_val = 0;      // Updated inside ADC ISR
volatile uint16_t mapped_val = 0;   // Processed in main loop
char msg[20];                       // UART message buffer


/* ================================================================
   SIMPLE SOFTWARE DELAY
   ------------------------------------------------
   Blocking delay (not accurate, used for demo)
   ================================================================*/
void delay(int t)
{
    for (volatile int i = 0; i < t * 1000; i++);
}


/* ================================================================
   MAP ADC VALUE (0–4095) → PERCENTAGE (0–100)
   ================================================================*/
uint16_t map_adc_to_percent(uint16_t adc)
{
    return (adc * 100) / 4095;
}


/* ================================================================
   UART2 INITIALIZATION
   ------------------------------------------------
   • Baud rate : 9600
   • PA2 → TX, PA3 → RX
   ================================================================*/
void UART2_Init(void)
{
    /* Enable clocks */
    RCC_APB2ENR |= (1 << 2);      // GPIOA
    RCC_APB1ENR |= (1 << 17);     // USART2

    /* PA2 → TX (AF Push-Pull, 50 MHz) */
    GPIOA_CRL &= ~(0xF << 8);
    GPIOA_CRL |=  (0xB << 8);

    /* PA3 → RX (Floating input) */
    GPIOA_CRL &= ~(0xF << 12);
    GPIOA_CRL |=  (0x4 << 12);

    /* Baud rate = 9600 (PCLK1 = 36 MHz) */
    USART2_BRR = 0xEA6;

    /* Enable USART, TX, RX */
    USART2_CR1 |= (1 << 13) | (1 << 3) | (1 << 2);
}


/* ================================================================
   UART TRANSMIT FUNCTIONS
   ================================================================*/
void UART2_SendChar(char c)
{
    while (!(USART2_SR & (1 << 7)));   // Wait until TX buffer empty
    USART2_DR = c;
}

void UART2_SendString(char *s)
{
    while (*s)
        UART2_SendChar(*s++);
}


/* ================================================================
   INTEGER TO STRING CONVERSION
   ================================================================*/
void int_to_str(uint16_t val, char *buf)
{
    int i = 0, j = 0;
    char temp[6];

    if (val == 0)
        buf[i++] = '0';
    else
    {
        while (val)
        {
            temp[j++] = (val % 10) + '0';
            val /= 10;
        }
        while (j)
            buf[i++] = temp[--j];
    }

    buf[i++] = '\r';
    buf[i++] = '\n';
    buf[i] = '\0';
}


/* ================================================================
   PRINT ADC VALUES OVER UART
   ================================================================*/
void print_adc_values(uint16_t raw, uint16_t mapped)
{
    UART2_SendString("Raw value   : ");
    int_to_str(raw, msg);
    UART2_SendString(msg);

    UART2_SendString("Mapped value: ");
    int_to_str(mapped, msg);
    UART2_SendString(msg);

    UART2_SendString("--------------------\r\n");
}


/* ================================================================
   ADC INITIALIZATION – INTERRUPT MODE
   ------------------------------------------------
   • Channel 4 (PA4)
   • Continuous conversion
   • End-Of-Conversion interrupt
   ================================================================*/
void ADC_Init(void)
{
    /* Enable clocks */
    RCC_APB2ENR |= (1 << 2);   // GPIOA
    RCC_APB2ENR |= (1 << 9);   // ADC1

    /* ADC clock = PCLK2 / 6 = 12 MHz */
    RCC_CFGR &= ~(3 << 14);
    RCC_CFGR |=  (2 << 14);

    /* PA4 as analog input */
    GPIOA_CRL &= ~(0xF << 16);

    /* Sample time = 239.5 cycles (better accuracy) */
    ADC1_SMPR2 |= (7 << 12);

    /* Select ADC channel 4 */
    ADC1_SQR3 = 4;

    /* Enable ADC End-Of-Conversion interrupt */
    ADC1_CR1 |= (1 << 5);

    /* Enable ADC interrupt in NVIC (IRQ18) */
    NVIC_ISER0 |= (1 << 18);

    /* Continuous conversion mode */
    ADC1_CR2 |= (1 << 1);

    /* -------- STM32F1 ADC START SEQUENCE -------- */

    /* Wake up ADC */
    ADC1_CR2 |= (1 << 0);
    delay(10);

    /* Reset calibration */
    ADC1_CR2 |= (1 << 3);
    while (ADC1_CR2 & (1 << 3));

    /* Start calibration */
    ADC1_CR2 |= (1 << 2);
    while (ADC1_CR2 & (1 << 2));

    /* Start conversion (ADON again) */
    ADC1_CR2 |= (1 << 0);
}


/* ================================================================
   ADC INTERRUPT SERVICE ROUTINE (ISR)
   ------------------------------------------------
   Triggered automatically when:
   • ADC conversion completes
   • EOC flag is set
   ================================================================*/
void ADC1_2_IRQHandler(void)
{
    if (ADC1_SR & (1 << 1))   // EOC flag
    {
        adc_val = ADC1_DR;   // Read ADC result (clears EOC)
        ADC1_SR &= ~(1 << 1);
    }
}


/* ================================================================
   MAIN FUNCTION
   ================================================================*/
int main(void)
{
    UART2_Init();    // Initialize UART
    ADC_Init();      // Initialize ADC + interrupt

    UART2_SendString("ADC Pot Value (Interrupt Mode):\r\n");

    while (1)
    {
        /* Process ADC data (outside ISR) */
        mapped_val = map_adc_to_percent(adc_val);

        /* Print values */
        print_adc_values(adc_val, mapped_val);

        delay(500);
    }
}
