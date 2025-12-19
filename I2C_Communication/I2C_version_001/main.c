#include <stdint.h>

/* ================= RCC ================= */
#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)
#define RCC_APB1ENR   (*(volatile uint32_t*)0x4002101C)

/* ================= AFIO ================= */
#define AFIO_MAPR     (*(volatile uint32_t*)0x40010004)

/* ================= GPIO ================= */
#define GPIOA_CRL     (*(volatile uint32_t*)0x40010800)
#define GPIOB_CRH     (*(volatile uint32_t*)0x40010C04)

/* ================= USART2 ================= */
#define USART2_SR     (*(volatile uint32_t*)0x40004400)
#define USART2_DR     (*(volatile uint32_t*)0x40004404)
#define USART2_BRR    (*(volatile uint32_t*)0x40004408)
#define USART2_CR1    (*(volatile uint32_t*)0x4000440C)

/* ================= I2C1 ================= */
#define I2C1_CR1      (*(volatile uint32_t*)0x40005400)
#define I2C1_CR2      (*(volatile uint32_t*)0x40005404)
#define I2C1_SR1      (*(volatile uint32_t*)0x40005414)
#define I2C1_SR2      (*(volatile uint32_t*)0x40005418)
#define I2C1_DR       (*(volatile uint32_t*)0x40005410)
#define I2C1_CCR      (*(volatile uint32_t*)0x4000541C)
#define I2C1_TRISE    (*(volatile uint32_t*)0x40005420)

/* ================= UART FUNCTIONS ================= */
void uart2_send_char(char c)
{
    while (!(USART2_SR & (1 << 7))); // TXE
    USART2_DR = c;
}

void uart2_send_string(const char *s)
{
    while (*s)
        uart2_send_char(*s++);
}

void uart2_send_hex(uint8_t v)
{
    const char map[] = "0123456789ABCDEF";
    uart2_send_char(map[(v >> 4) & 0x0F]);
    uart2_send_char(map[v & 0x0F]);
}

/* ================= I2C INIT ================= */
void i2c1_init(void)
{
    /* Enable clocks */
    RCC_APB2ENR |= (1 << 0);   // AFIO
    RCC_APB2ENR |= (1 << 3);   // GPIOB
    RCC_APB1ENR |= (1 << 21);  // I2C1

    /* ?? Enable I2C1 REMAP ? PB8 / PB9 */
    AFIO_MAPR |= (1 << 1);     // I2C1_REMAP

    /* PB8, PB9 ? AF Open-Drain, 50 MHz */
    GPIOB_CRH &= ~((0xF << 0) | (0xF << 4));
    GPIOB_CRH |=  (0xB << 0);  // PB8 AF OD
    GPIOB_CRH |=  (0xB << 4);  // PB9 AF OD

    /* Reset I2C */
    I2C1_CR1 |=  (1 << 15);
    I2C1_CR1 &= ~(1 << 15);

    /* PCLK1 = 36 MHz */
    I2C1_CR2 = 36;

    /* Standard mode 100 kHz */
    I2C1_CCR = 180;
    I2C1_TRISE = 37;

    /* Enable I2C */
    I2C1_CR1 |= (1 << 0);
}

/* ================= I2C START ================= */
void i2c_start(void)
{
    while (I2C1_SR2 & (1 << 1));     // Wait BUSY = 0
    I2C1_CR1 |= (1 << 8);            // START
    while (!(I2C1_SR1 & (1 << 0)));  // SB
    (void)I2C1_SR1;
}

/* ================= I2C STOP ================= */
void i2c_stop(void)
{
    I2C1_CR1 |= (1 << 9);
}

/* ================= I2C ADDRESS CHECK ================= */
int i2c_check_address(uint8_t addr)
{
    I2C1_DR = (addr << 1);   // Write mode

    while (!(I2C1_SR1 & ((1 << 1) | (1 << 10))));

    if (I2C1_SR1 & (1 << 10))   // AF = NACK
    {
        I2C1_SR1 &= ~(1 << 10);
        i2c_stop();
        return 0;
    }

    (void)I2C1_SR2;  // Clear ADDR
    i2c_stop();
    return 1;
}

/* ================= MAIN ================= */
int main(void)
{
    /* UART clocks */
    RCC_APB2ENR |= (1 << 2);   // GPIOA
    RCC_APB1ENR |= (1 << 17);  // USART2

    /* PA2 TX, PA3 RX */
    GPIOA_CRL &= ~((0xF << 8) | (0xF << 12));
    GPIOA_CRL |=  (0xB << 8);  // TX
    GPIOA_CRL |=  (0x4 << 12); // RX

    /* UART2 baud = 9600 */
    USART2_BRR = 0x0EA6;
    USART2_CR1 = (1 << 13) | (1 << 3) | (1 << 2);

    /* Init I2C */
    i2c1_init();

    /* Print header */
    uart2_send_string("\r\nWelcome to I2C Communication:\r\n");
    uart2_send_string("Theme of Project is Interfacing I2C\r\n\r\n");
    uart2_send_string("Heading: I2C Scanner\r\n");
    uart2_send_string("Master : STM32F103C8T6\r\n");
    uart2_send_string("Slave  : OLED Display (I2C)\r\n");
    uart2_send_string("Pins   : PB8 (SCL), PB9 (SDA)\r\n\r\n");
    uart2_send_string("I2C Scanner Started...\r\n\r\n");

    /* Scan addresses */
    for (uint8_t addr = 0x08; addr < 0x78; addr++)
    {
        i2c_start();
        if (i2c_check_address(addr))
        {
            uart2_send_string("Address Found: 0x");
            uart2_send_hex(addr);
            uart2_send_string("\r\n");
        }
    }

    uart2_send_string("\r\nScan Complete\r\n");

    while (1);
}
