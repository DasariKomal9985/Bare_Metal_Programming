#include <stdint.h>

/* ================================================================
   RCC REGISTERS
   ================================================================*/
#define RCC_APB2ENR     (*(volatile uint32_t*)0x40021018)
#define RCC_APB1ENR     (*(volatile uint32_t*)0x4002101C)
#define RCC_CFGR        (*(volatile uint32_t*)0x40021004)

/* ================================================================
   GPIOA REGISTERS
   ================================================================*/
#define GPIOA_CRL       (*(volatile uint32_t*)0x40010800)

/* ================================================================
   USART2 REGISTERS
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
   ================================================================*/
#define NVIC_ISER0      (*(volatile uint32_t*)0xE000E100)

/* ================================================================
   GLOBAL VARIABLES
   ================================================================*/
volatile uint16_t adc_val = 0;   // MUST be volatile
char msg[20];
volatile uint16_t mapped_val = 0;

/* ================================================================
   SIMPLE DELAY
   ================================================================*/
void delay(int t)
{
    for (volatile int i = 0; i < t * 1000; i++);
}
uint16_t map_adc_to_percent(uint16_t adc)
{
    return (adc * 100) / 4095;
}

/* ================================================================
   UART2 INITIALIZATION
   ================================================================*/
void UART2_Init(void)
{
    RCC_APB2ENR |= (1 << 2);     // GPIOA
    RCC_APB1ENR |= (1 << 17);    // USART2

    /* PA2 -> TX (AF Push-Pull, 50MHz) */
    GPIOA_CRL &= ~(0xF << 8);
    GPIOA_CRL |=  (0xB << 8);

    /* PA3 -> RX (Floating input) */
    GPIOA_CRL &= ~(0xF << 12);
    GPIOA_CRL |=  (0x4 << 12);

    /* 9600 baud @ 36MHz */
    USART2_BRR = 0xEA6;

    /* Enable USART, TX, RX */
    USART2_CR1 |= (1 << 13) | (1 << 3) | (1 << 2);
}

/* ================================================================
   UART SEND FUNCTIONS
   ================================================================*/
void UART2_SendChar(char c)
{
    while (!(USART2_SR & (1 << 7)));
    USART2_DR = c;
}

void UART2_SendString(char *s)
{
    while (*s)
        UART2_SendChar(*s++);
}

/* ================================================================
   INTEGER TO STRING
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
   ADC INITIALIZATION — INTERRUPT MODE (FIXED)
   ================================================================*/
void ADC_Init(void)
{
    RCC_APB2ENR |= (1 << 2);   // GPIOA
    RCC_APB2ENR |= (1 << 9);   // ADC1

    /* ADC clock = PCLK2 / 6 = 12 MHz */
    RCC_CFGR &= ~(3 << 14);
    RCC_CFGR |=  (2 << 14);

    /* PA4 analog input */
    GPIOA_CRL &= ~(0xF << 16);

    /* Sample time: 239.5 cycles */
    ADC1_SMPR2 |= (7 << 12);

    /* Channel 4 */
    ADC1_SQR3 = 4;

    /* Enable EOC interrupt */
    ADC1_CR1 |= (1 << 5);

    /* Enable ADC interrupt in NVIC (IRQ18) */
    NVIC_ISER0 |= (1 << 18);

    /* Continuous conversion mode */
    ADC1_CR2 |= (1 << 1);

    /* -------- STM32F1 REQUIRED SEQUENCE -------- */

    /* Wake up ADC */
    ADC1_CR2 |= (1 << 0);
    delay(10);

    /* Reset calibration */
    ADC1_CR2 |= (1 << 3);
    while (ADC1_CR2 & (1 << 3));

    /* Start calibration */
    ADC1_CR2 |= (1 << 2);
    while (ADC1_CR2 & (1 << 2));

    /* ?? START CONVERSION (ADON AGAIN) */
    ADC1_CR2 |= (1 << 0);
}

/* ================================================================
   ADC INTERRUPT HANDLER
   ================================================================*/
void ADC1_2_IRQHandler(void)
{
    if (ADC1_SR & (1 << 1))   // EOC
    {
        adc_val = ADC1_DR;   // Reading DR clears EOC
        ADC1_SR &= ~(1 << 1);
    }
}

/* ================================================================
   MAIN FUNCTION
   ================================================================*/
int main(void)
{
    UART2_Init();
    ADC_Init();

    UART2_SendString("ADC Pot Value (Interrupt Mode):\r\n");

    while (1)
    {
        mapped_val = map_adc_to_percent(adc_val);
        print_adc_values(adc_val, mapped_val);
        delay(500);
    }
}

