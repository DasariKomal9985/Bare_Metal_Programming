#include <stdint.h>

/* ================= RCC ================= */
#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)
#define RCC_APB1ENR   (*(volatile uint32_t*)0x4002101C)

/* ================= GPIO ================= */
#define GPIOA_CRL     (*(volatile uint32_t*)0x40010800)

/* ================= USART2 ================= */
#define USART2_SR     (*(volatile uint32_t*)0x40004400)
#define USART2_DR     (*(volatile uint32_t*)0x40004404)
#define USART2_BRR    (*(volatile uint32_t*)0x40004408)
#define USART2_CR1    (*(volatile uint32_t*)0x4000440C)

/* ================= NVIC ================= */
#define NVIC_ISER1    (*(volatile uint32_t*)0xE000E104)

/* ================= DEFINES ================= */
#define RX_BUF_SIZE  64

/* ================= GLOBALS ================= */
volatile char rx_buffer[RX_BUF_SIZE];
volatile uint8_t rx_index = 0;
volatile uint8_t line_ready = 0;

/* ================= UART SEND ================= */
void uart2_send_char(char c)
{
    while (!(USART2_SR & (1 << 7)));   // TXE
    USART2_DR = c;
}

void uart2_send_string(const char *s)
{
    while (*s)
        uart2_send_char(*s++);
}

/* ================= USART2 INTERRUPT ================= */
void USART2_IRQHandler(void)
{
    if (USART2_SR & (1 << 5))   // RXNE
    {
        char ch = USART2_DR;

        if (ch == '\r')   // ENTER pressed
        {
            rx_buffer[rx_index] = '\0';   // Null terminate
            line_ready = 1;
            rx_index = 0;
        }
        else
        {
            if (rx_index < RX_BUF_SIZE - 1)
            {
                rx_buffer[rx_index++] = ch;
            }
        }
    }
}

/* ================= MAIN ================= */
int main(void)
{
    /* Enable clocks */
    RCC_APB2ENR |= (1 << 2);     // GPIOA
    RCC_APB1ENR |= (1 << 17);    // USART2

    /* PA2 -> TX, PA3 -> RX */
    GPIOA_CRL &= ~((0xF << 8) | (0xF << 12));
    GPIOA_CRL |=  (0xB << 8);    // PA2 AF PP
    GPIOA_CRL |=  (0x4 << 12);   // PA3 input floating

    /* Baud rate 9600 (PCLK1 = 36MHz) */
    USART2_BRR = 0x0EA6;

    /* USART2 config
       UE, TE, RE
       RX interrupt
       9-bit (parity)
       EVEN parity
    */
    USART2_CR1 =
        (1 << 13) |   // UE
        (1 << 3)  |   // TE
        (1 << 2)  |   // RE
        (1 << 5)  |   // RXNEIE
        (1 << 12) |   // 9-bit
        (1 << 10);    // EVEN parity

    /* Enable USART2 IRQ */
    NVIC_ISER1 |= (1 << 6);

    /* Welcome message */
    uart2_send_string("\r\nWelcome to UART Communication:\r\n");
    uart2_send_string("Theme of Project is Interfacing UART2\r\n\r\n");
    uart2_send_string("Heading: TTL Communication\r\n");
    uart2_send_string("Baud Rate : 9600\r\n");
    uart2_send_string("Parity    : EVEN\r\n");
    uart2_send_string("Mode      : Interrupt Based\r\n\r\n");

    while (1)
    {
        if (line_ready)
        {
            uart2_send_string("From Keyboard : ");
            uart2_send_string((char *)rx_buffer);
            uart2_send_string("\r\n");

            uart2_send_string("To   Teraterm : ");
            uart2_send_string((char *)rx_buffer);
            uart2_send_string("\r\n\r\n");

            line_ready = 0;
        }
    }
}
