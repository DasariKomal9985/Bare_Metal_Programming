#include <stdint.h>

/* ================= RCC ================= */
#define RCC_APB2ENR (*(volatile uint32_t*)0x40021018)
#define RCC_APB1ENR (*(volatile uint32_t*)0x4002101C)

/* ================= GPIO ================= */
#define GPIOA_CRL   (*(volatile uint32_t*)0x40010800)
#define GPIOB_CRL   (*(volatile uint32_t*)0x40010C00)
#define GPIOB_ODR   (*(volatile uint32_t*)0x40010C0C)

/* ================= USART2 ================= */
#define USART2_SR   (*(volatile uint32_t*)0x40004400)
#define USART2_DR   (*(volatile uint32_t*)0x40004404)
#define USART2_BRR  (*(volatile uint32_t*)0x40004408)
#define USART2_CR1  (*(volatile uint32_t*)0x4000440C)

/* ================= SPI1 ================= */
#define SPI1_CR1    (*(volatile uint32_t*)0x40013000)
#define SPI1_SR     (*(volatile uint32_t*)0x40013008)
#define SPI1_DR     (*(volatile uint32_t*)0x4001300C)

/* ================= nRF24L01 ================= */
#define NRF_CMD_R_REGISTER   0x00
#define NRF_REG_STATUS       0x07

/* ================= UART FUNCTIONS ================= */
void uart2_send_char(char c)
{
    while (!(USART2_SR & (1 << 7)));   // TXE
    USART2_DR = c;
}

void uart2_send_string(const char *s)
{
    while (*s) uart2_send_char(*s++);
}

void uart2_send_hex(uint8_t v)
{
    const char map[] = "0123456789ABCDEF";
    uart2_send_char(map[(v >> 4) & 0x0F]);
    uart2_send_char(map[v & 0x0F]);
}

/* ================= SPI TRANSFER ================= */
uint8_t spi1_transfer(uint8_t data)
{
    while (!(SPI1_SR & (1 << 1)));   // TXE
    SPI1_DR = data;
    while (!(SPI1_SR & (1 << 0)));   // RXNE
    return SPI1_DR;
}

/* ================= MAIN ================= */
int main(void)
{
    /* Enable clocks */
    RCC_APB2ENR |= (1 << 2) | (1 << 3) | (1 << 12); // GPIOA, GPIOB, SPI1
    RCC_APB1ENR |= (1 << 17);                       // USART2

    /* ================= UART2 SETUP ================= */
    GPIOA_CRL &= ~((0xF << 8) | (0xF << 12));
    GPIOA_CRL |=  (0xB << 8);   // PA2 TX
    GPIOA_CRL |=  (0x4 << 12);  // PA3 RX

    USART2_BRR = 0x0EA6;        // 9600 baud
    USART2_CR1 = (1 << 13) | (1 << 3) | (1 << 2);

    /* ================= SPI1 PINS ================= */
    GPIOA_CRL &= ~((0xF << 20) | (0xF << 24) | (0xF << 28));
    GPIOA_CRL |=  (0xB << 20);  // PA5  SCK
    GPIOA_CRL |=  (0x4 << 24);  // PA6  MISO
    GPIOA_CRL |=  (0xB << 28);  // PA7  MOSI

    /* ================= CSN & CE ================= */
    GPIOB_CRL &= ~((0xF << 16) | (0xF << 20));
    GPIOB_CRL |=  (0x3 << 16);  // PB12 CSN
    GPIOB_CRL |=  (0x3 << 20);  // PB13 CE

    GPIOB_ODR |=  (1 << 12);    // CSN HIGH
    GPIOB_ODR &= ~(1 << 13);    // CE LOW

    /* ================= SPI1 CONFIG ================= */
    SPI1_CR1 =
        (1 << 2) |    // Master
        (1 << 9) |    // SSM
        (1 << 8) |    // SSI
        (3 << 3);     // Baud rate = fPCLK/16

    SPI1_CR1 |= (1 << 6);       // SPI Enable

    /* ================= WELCOME MESSAGE ================= */
    uart2_send_string("\r\nWelcome to SPI Communication\r\n");
    uart2_send_string("Theme of Project is Interfacing SPI Protocol\r\n\r\n");

    uart2_send_string("Heading : SPI Register Read Test\r\n");
    uart2_send_string("Master  : STM32F103C8T6\r\n");
    uart2_send_string("Slave   : nRF24L01 (SPI Device)\r\n\r\n");

    uart2_send_string("SPI Pin Configuration:\r\n");
    uart2_send_string("SCK  -> PA5\r\n");
    uart2_send_string("MOSI -> PA7\r\n");
    uart2_send_string("MISO -> PA6\r\n");
    uart2_send_string("CSN  -> PB12\r\n");
    uart2_send_string("CE   -> PB13 (Held LOW)\r\n\r\n");

    uart2_send_string("SPI Mode : Master\r\n");
    uart2_send_string("CPOL/CPHA: Mode 0 (0,0)\r\n");
    uart2_send_string("Data Size: 8-bit\r\n");
    uart2_send_string("Clock    : fPCLK / 16\r\n\r\n");

    uart2_send_string("Operation:\r\n");
    uart2_send_string("Reading STATUS register (0x07) from nRF24L01\r\n");
    uart2_send_string("Using SPI full-duplex communication\r\n\r\n");

    /* ================= READ STATUS REGISTER ================= */
    GPIOB_ODR &= ~(1 << 12);    // CSN LOW
    for (volatile int i = 0; i < 1000; i++); // CSN setup delay

    spi1_transfer(NRF_CMD_R_REGISTER | NRF_REG_STATUS);
    uint8_t status = spi1_transfer(0xFF);

    GPIOB_ODR |= (1 << 12);     // CSN HIGH

    uart2_send_string("STATUS Register: 0x");
    uart2_send_hex(status);
    uart2_send_string("\r\nSPI Communication OK\r\n");

    while (1);
}
