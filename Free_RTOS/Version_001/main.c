#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* =========================================================
   RCC
   ========================================================= */
#define RCC_APB2ENR (*(volatile uint32_t*)0x40021018)
#define RCC_APB1ENR (*(volatile uint32_t*)0x4002101C)
#define RCC_CFGR    (*(volatile uint32_t*)0x40021004)

/* =========================================================
   GPIO
   ========================================================= */
#define GPIOA_CRL   (*(volatile uint32_t*)0x40010800)
#define GPIOB_CRL   (*(volatile uint32_t*)0x40010C00)
#define GPIOB_ODR   (*(volatile uint32_t*)0x40010C0C)

/* =========================================================
   USART2
   ========================================================= */
#define USART2_SR   (*(volatile uint32_t*)0x40004400)
#define USART2_DR   (*(volatile uint32_t*)0x40004404)
#define USART2_BRR  (*(volatile uint32_t*)0x40004408)
#define USART2_CR1  (*(volatile uint32_t*)0x4000440C)

/* =========================================================
   ADC1
   ========================================================= */
#define ADC1_SR     (*(volatile uint32_t*)0x40012400)
#define ADC1_CR1    (*(volatile uint32_t*)0x40012404)
#define ADC1_CR2    (*(volatile uint32_t*)0x40012408)
#define ADC1_SMPR2  (*(volatile uint32_t*)0x40012410)
#define ADC1_SQR3   (*(volatile uint32_t*)0x40012434)
#define ADC1_DR     (*(volatile uint32_t*)0x4001244C)

/* =========================================================
   SPI1
   ========================================================= */
#define SPI1_CR1    (*(volatile uint32_t*)0x40013000)
#define SPI1_SR     (*(volatile uint32_t*)0x40013008)
#define SPI1_DR     (*(volatile uint32_t*)0x4001300C)

/* =========================================================
   I2C1 (basic)
   ========================================================= */
#define I2C1_CR1    (*(volatile uint32_t*)0x40005400)
#define I2C1_CR2    (*(volatile uint32_t*)0x40005404)
#define I2C1_SR1    (*(volatile uint32_t*)0x40005414)
#define I2C1_DR     (*(volatile uint32_t*)0x40005410)

/* =========================================================
   GLOBAL RTOS OBJECTS
   ========================================================= */
SemaphoreHandle_t uartSem;
char uartBuf[128];

volatile uint16_t adc_val;

/* =========================================================
   UART (bare-metal)
   ========================================================= */
void UART_SendString(const char *s)
{
    while (*s)
    {
        while (!(USART2_SR & (1 << 7)));
        USART2_DR = *s++;
    }
}

void UART_Init(void)
{
    RCC_APB2ENR |= (1 << 2);
    RCC_APB1ENR |= (1 << 17);

    GPIOA_CRL &= ~((0xF << 8) | (0xF << 12));
    GPIOA_CRL |=  (0xB << 8);
    GPIOA_CRL |=  (0x4 << 12);

    USART2_BRR = 0x0EA6; // 9600 baud
    USART2_CR1 = (1 << 13) | (1 << 3) | (1 << 2);
}

/* =========================================================
   LED (PB4)
   ========================================================= */
void LED_Init(void)
{
    RCC_APB2ENR |= (1 << 3);
    GPIOB_CRL &= ~(0xF << 16);
    GPIOB_CRL |=  (0x1 << 16);
}

/* =========================================================
   ADC (PA4)
   ========================================================= */
void ADC_Init(void)
{
    RCC_APB2ENR |= (1 << 2) | (1 << 9);

    GPIOA_CRL &= ~(0xF << 16);   // PA4 analog

    RCC_CFGR |= (2 << 14);       // ADC clk /6
    ADC1_SMPR2 |= (7 << 12);     // long sample
    ADC1_SQR3 = 4;

    ADC1_CR2 |= (1 << 1);        // continuous
    ADC1_CR2 |= (1 << 0);
}

/* =========================================================
   SPI (basic loopback / nRF read ready)
   ========================================================= */
uint8_t SPI_Transfer(uint8_t d)
{
    while (!(SPI1_SR & (1 << 1)));
    SPI1_DR = d;
    while (!(SPI1_SR & (1 << 0)));
    return SPI1_DR;
}

void SPI_Init(void)
{
    RCC_APB2ENR |= (1 << 2) | (1 << 12);

    GPIOA_CRL |= (0xB << 20) | (0x4 << 24) | (0xB << 28);

    SPI1_CR1 = (1 << 2) | (1 << 9) | (1 << 8) | (3 << 3);
    SPI1_CR1 |= (1 << 6);
}

/* =========================================================
   I2C (simple flag)
   ========================================================= */
void I2C_Init(void)
{
    RCC_APB1ENR |= (1 << 21);
    I2C1_CR1 |= (1 << 0);
}

/* =========================================================
   RTOS TASKS
   ========================================================= */
void LED_Task(void *p)
{
    while (1)
    {
        GPIOB_ODR ^= (1 << 4);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void ADC_Task(void *p)
{
    while (1)
    {
        ADC1_CR2 |= (1 << 0);
        while (!(ADC1_SR & (1 << 1)));
        adc_val = ADC1_DR;

        int percent = (adc_val * 100) / 4095;
        sprintf(uartBuf, "[ADC ] Raw=%d  %%=%d\r\n", adc_val, percent);
        xSemaphoreGive(uartSem);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void SPI_Task(void *p)
{
    while (1)
    {
        uint8_t r = SPI_Transfer(0x55);
        sprintf(uartBuf, "[SPI ] RX=0x%02X\r\n", r);
        xSemaphoreGive(uartSem);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void I2C_Task(void *p)
{
    while (1)
    {
        sprintf(uartBuf, "[I2C ] Bus Active\r\n");
        xSemaphoreGive(uartSem);

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void UART_Task(void *p)
{
    while (1)
    {
        xSemaphoreTake(uartSem, portMAX_DELAY);
        UART_SendString(uartBuf);
    }
}

/* =========================================================
   MAIN
   ========================================================= */
int main(void)
{
    LED_Init();
    UART_Init();
    ADC_Init();
    SPI_Init();
    I2C_Init();

    uartSem = xSemaphoreCreateBinary();

    xTaskCreate(LED_Task,  "LED",  128, NULL, 1, NULL);
    xTaskCreate(ADC_Task,  "ADC",  256, NULL, 2, NULL);
    xTaskCreate(SPI_Task,  "SPI",  256, NULL, 2, NULL);
    xTaskCreate(I2C_Task,  "I2C",  256, NULL, 2, NULL);
    xTaskCreate(UART_Task, "UART", 256, NULL, 3, NULL);

    UART_SendString("FreeRTOS Multi-Peripheral Demo\r\n");

    vTaskStartScheduler();

    while (1);
}
