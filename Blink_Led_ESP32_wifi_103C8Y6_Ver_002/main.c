#include <stdint.h>
#include <string.h>
#include <stdio.h>

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

/* ================= DELAY ================= */
void delay(volatile uint32_t d)
{
    while (d--) __asm("nop");
}

/* ================= UART2 DEBUG ================= */
void uart2_init(void)
{
    RCC_APB2ENR |= (1 << 2);      // GPIOA
    RCC_APB1ENR |= (1 << 17);     // USART2

    GPIOA_CRL &= ~((0xF << 8) | (0xF << 12));
    GPIOA_CRL |=  (0xB << 8);     // PA2 TX
    GPIOA_CRL |=  (0x4 << 12);    // PA3 RX

    USART2_BRR = 0xEA6;           // 115200 @36MHz
    USART2_CR1 |= (1<<13)|(1<<3)|(1<<2);
}

void uart2_tx(char c)
{
    while (!(USART2_SR & (1<<7)));
    USART2_DR = c;
}

void uart2_print(const char *s)
{
    while (*s) uart2_tx(*s++);
}

/* ================= UART3 ESP32 ================= */
void uart3_init(void)
{
    RCC_APB2ENR |= (1 << 3) | (1 << 0);   // GPIOB + AFIO
    RCC_APB1ENR |= (1 << 18);             // USART3

    AFIO_MAPR &= ~(7 << 24);
    AFIO_MAPR |=  (2 << 24);              // Disable JTAG

    GPIOB_CRH &= ~((0xF << 8) | (0xF << 12));
    GPIOB_CRH |=  (0xB << 8);              // PB10 TX
    GPIOB_CRH |=  (0x4 << 12);             // PB11 RX

    USART3_BRR = 0xEA6;                    // 115200
    USART3_CR1 |= (1<<13)|(1<<3)|(1<<2);
}

void uart3_tx(char c)
{
    while (!(USART3_SR & (1<<7)));
    USART3_DR = c;
}

void uart3_print(const char *s)
{
    while (*s) uart3_tx(*s++);
}

/* ================= GPIO ================= */
void gpio_init(void)
{
    RCC_APB2ENR |= (1 << 3);      // GPIOB

    GPIOB_CRL &= ~(0xF << 16);
    GPIOB_CRL |=  (0x1 << 16);    // PB4 OUTPUT

    GPIOB_ODR &= ~(1 << 4);       // LED OFF
}

/* ================= ESP RX HANDLER ================= */
void esp_read_response(uint32_t timeout)
{
    while (timeout--)
    {
        if (USART3_SR & (1 << 5))   // RXNE
        {
            char c = USART3_DR;
            uart2_tx(c);           // SHOW ESP OUTPUT ON PC
        }
    }
}

/* ================= ESP AT COMMAND ================= */
void esp_cmd(const char *cmd)
{
    uart2_print("\r\n>> ");
    uart2_print(cmd);        // show command

    uart3_print(cmd);        // send to ESP32

    esp_read_response(2000000);   // <<< READ RESPONSE
}

/* ================= WEB PAGE ================= */
const char webpage[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n\r\n"
"<html>"
"<h2>STM32 LED Control</h2>"
"<a href=\"/on\">LED ON</a><br>"
"<a href=\"/off\">LED OFF</a>"
"</html>";

void send_page(void)
{
    char buf[40];
    int len = strlen(webpage);

    sprintf(buf, "AT+CIPSEND=0,%d\r\n", len);
    esp_cmd(buf);
    esp_cmd(webpage);
}

/* ================= MAIN ================= */
int main(void)
{
    char rx[300];
    int idx = 0;

    gpio_init();
    uart2_init();
    uart3_init();

    uart2_print("\r\nSTM32 BOOT\r\n");

    /* ESP32 INIT */
    esp_cmd("AT\r\n");
    esp_cmd("AT+CWMODE=1\r\n");
    esp_cmd("AT+CWJAP=\"beaglebone\",\"12345678910\"\r\n");
    esp_cmd("AT+CIFSR\r\n");          // <<< IP APPEARS HERE
    esp_cmd("AT+CIPMUX=1\r\n");
    esp_cmd("AT+CIPSERVER=1,80\r\n");

    uart2_print("\r\nWEB SERVER READY\r\n");

    while (1)
    {
        if (USART3_SR & (1 << 5))
        {
            char c = USART3_DR;
            uart2_tx(c);             // debug
            rx[idx++] = c;

            if (idx >= sizeof(rx)) idx = 0;

            if (strstr(rx, "GET /on"))
            {
                GPIOB_ODR |= (1 << 4);
                send_page();
                idx = 0;
            }
            else if (strstr(rx, "GET /off"))
            {
                GPIOB_ODR &= ~(1 << 4);
                send_page();
                idx = 0;
            }
            else if (strstr(rx, "GET / "))
            {
                send_page();
                idx = 0;
            }
        }
    }
}
