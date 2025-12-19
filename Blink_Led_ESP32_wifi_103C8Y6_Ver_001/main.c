#include <stdint.h>

/* ================= RCC ================= */
#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)
#define RCC_APB1ENR   (*(volatile uint32_t*)0x4002101C)

/* ================= GPIO ================= */
#define GPIOA_CRL     (*(volatile uint32_t*)0x40010800)
#define GPIOB_CRL     (*(volatile uint32_t*)0x40010C00)
#define GPIOB_CRH     (*(volatile uint32_t*)0x40010C04)
#define GPIOB_ODR     (*(volatile uint32_t*)0x40010C0C)

/* ================= AFIO ================= */
#define AFIO_MAPR     (*(volatile uint32_t*)0x40010004)

/* ================= USART2 (DEBUG) ================= */
#define USART2_SR     (*(volatile uint32_t*)0x40004400)
#define USART2_DR     (*(volatile uint32_t*)0x40004404)
#define USART2_BRR    (*(volatile uint32_t*)0x40004408)
#define USART2_CR1    (*(volatile uint32_t*)0x4000440C)

/* ================= USART3 (ESP32) ================= */
#define USART3_SR     (*(volatile uint32_t*)0x40004800)
#define USART3_DR     (*(volatile uint32_t*)0x40004804)
#define USART3_BRR    (*(volatile uint32_t*)0x40004808)
#define USART3_CR1    (*(volatile uint32_t*)0x4000480C)

/* ================= UART2 DEBUG ================= */
void UART2_Init(void)
{
    RCC_APB2ENR |= (1 << 2);      // GPIOA
    RCC_APB1ENR |= (1 << 17);     // USART2

    GPIOA_CRL &= ~(0xF << 8);
    GPIOA_CRL |=  (0xB << 8);     // PA2 TX

    USART2_BRR = 0xEA6;           // 9600 @ 36MHz
    USART2_CR1 |= (1<<13)|(1<<3)|(1<<2);
}

void UART2_SendChar(char c)
{
    while (!(USART2_SR & (1<<7)));
    USART2_DR = c;
}

void UART2_SendString(const char *s)
{
    while (*s) UART2_SendChar(*s++);
}

/* ================= UART3 ESP32 ================= */
void UART3_Init(void)
{
    RCC_APB2ENR |= (1 << 3);      // GPIOB
    RCC_APB1ENR |= (1 << 18);     // USART3

    GPIOB_CRH &= ~(0xF << 12);
    GPIOB_CRH |=  (0x4 << 12);    // PB11 RX

    USART3_BRR = 0xEA6;           // 9600 @ 36MHz
    USART3_CR1 |= (1<<13)|(1<<2);
}

char UART3_Read(void)
{
    while (!(USART3_SR & (1<<5)));
    return USART3_DR;
}

/* ================= GPIO ================= */
void GPIO_Init(void)
{
    RCC_APB2ENR |= (1<<0) | (1<<3);   // AFIO + GPIOB

    AFIO_MAPR &= ~(7<<24);
    AFIO_MAPR |=  (2<<24);            // Disable JTAG

    GPIOB_CRL &= ~(0xF << 16);
    GPIOB_CRL |=  (0x1 << 16);        // PB4 OUTPUT

    GPIOB_ODR &= ~(1<<4);             // LED OFF
}

/* ================= MAIN ================= */
int main(void)
{
    GPIO_Init();
    UART2_Init();
    UART3_Init();

    UART2_SendString("STM32 READY\r\n");

    while (1)
    {
        char c = UART3_Read();

        UART2_SendString("RX: ");
        UART2_SendChar(c);
        UART2_SendString("\r\n");

        if (c == '1')
        {
            GPIOB_ODR |= (1<<4);
            UART2_SendString("LED ON\r\n");
        }
        else if (c == '0')
        {
            GPIOB_ODR &= ~(1<<4);
            UART2_SendString("LED OFF\r\n");
        }
    }
}
