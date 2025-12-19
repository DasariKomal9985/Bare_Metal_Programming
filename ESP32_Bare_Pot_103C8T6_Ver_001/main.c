#include <stdint.h>

/* ================= USER CONFIG ================= */
#define WIFI_SSID   "beaglebone"
#define WIFI_PASS   "12345678910"
#define SERVER_IP   "192.168.1.103"

/* ================= RCC ================= */
#define RCC_APB2ENR (*(volatile uint32_t*)0x40021018)
#define RCC_APB1ENR (*(volatile uint32_t*)0x4002101C)
#define RCC_CFGR    (*(volatile uint32_t*)0x40021004)

/* ================= GPIO ================= */
#define GPIOA_CRL   (*(volatile uint32_t*)0x40010800)
#define GPIOB_CRL   (*(volatile uint32_t*)0x40010C00)

/* ================= USART2 (Docklight) ================= */
#define USART2_SR   (*(volatile uint32_t*)0x40004400)
#define USART2_DR   (*(volatile uint32_t*)0x40004404)
#define USART2_BRR  (*(volatile uint32_t*)0x40004408)
#define USART2_CR1  (*(volatile uint32_t*)0x4000440C)

/* ================= USART3 (ESP) ================= */
#define USART3_SR   (*(volatile uint32_t*)0x40004800)
#define USART3_DR   (*(volatile uint32_t*)0x40004804)
#define USART3_BRR  (*(volatile uint32_t*)0x40004808)
#define USART3_CR1  (*(volatile uint32_t*)0x4000480C)

/* ================= ADC ================= */
#define ADC1_SR     (*(volatile uint32_t*)0x40012400)
#define ADC1_CR1    (*(volatile uint32_t*)0x40012404)
#define ADC1_CR2    (*(volatile uint32_t*)0x40012408)
#define ADC1_SMPR2  (*(volatile uint32_t*)0x40012410)
#define ADC1_SQR3   (*(volatile uint32_t*)0x40012434)
#define ADC1_DR     (*(volatile uint32_t*)0x4001244C)

/* ================= NVIC ================= */
#define NVIC_ISER0  (*(volatile uint32_t*)0xE000E100)

/* ================= GLOBALS ================= */
volatile uint16_t adc_val;
volatile uint8_t  adc_flag = 0;
volatile uint8_t  inet_flag = 0;
char msg[32];

/* ================= DELAY ================= */
void delay(int t)
{
    for (volatile int i = 0; i < t * 1000; i++);
}

/* ================= UART2 (Docklight) ================= */
void UART2_Init(void)
{
    RCC_APB2ENR |= (1<<2);
    RCC_APB1ENR |= (1<<17);

    GPIOA_CRL &= ~(0xF<<8);
    GPIOA_CRL |=  (0xB<<8);

    GPIOA_CRL &= ~(0xF<<12);
    GPIOA_CRL |=  (0x4<<12);

    USART2_BRR = 0xEA6;
    USART2_CR1 |= (1<<13)|(1<<3)|(1<<2);
}

void UART2_SendChar(char c)
{
    while (!(USART2_SR & (1<<7)));
    USART2_DR = c;
}

void UART2_SendString(char *s)
{
    while (*s) UART2_SendChar(*s++);
}

/* ================= UART3 (ESP) ================= */
void UART3_Init(void)
{
    RCC_APB2ENR |= (1<<3);
    RCC_APB1ENR |= (1<<18);

    GPIOB_CRL &= ~(0xF<<8);
    GPIOB_CRL |=  (0xB<<8);

    GPIOB_CRL &= ~(0xF<<12);
    GPIOB_CRL |=  (0x4<<12);

    USART3_BRR = 0x138; // 115200
    USART3_CR1 |= (1<<13)|(1<<3)|(1<<2);
}

void UART3_SendChar(char c)
{
    while (!(USART3_SR & (1<<7)));
    USART3_DR = c;
}

void UART3_SendString(char *s)
{
    while (*s) UART3_SendChar(*s++);
}

/* ================= INT TO STR ================= */
void int_to_str(uint16_t v, char *b)
{
    int i=0, j=0; char t[6];
    if(v==0) b[i++]='0';
    else {
        while(v){ t[j++]=(v%10)+'0'; v/=10; }
        while(j) b[i++]=t[--j];
    }
    b[i]=0;
}

/* ================= ADC ================= */
void ADC_Init(void)
{
    RCC_APB2ENR |= (1<<2)|(1<<9);
    RCC_CFGR |= (2<<14);

    GPIOA_CRL &= ~(0xF<<16);
    ADC1_SMPR2 |= (7<<12);
    ADC1_SQR3 = 4;

    ADC1_CR1 |= (1<<5);
    NVIC_ISER0 |= (1<<18);

    ADC1_CR2 |= (1<<1)|(1<<0);
    delay(10);

    ADC1_CR2 |= (1<<3);
    while(ADC1_CR2&(1<<3));

    ADC1_CR2 |= (1<<2);
    while(ADC1_CR2&(1<<2));

    ADC1_CR2 |= (1<<0);
}

void ADC1_2_IRQHandler(void)
{
    if(ADC1_SR&(1<<1))
    {
        adc_val = ADC1_DR;
        adc_flag = 1;   // equivalent to lm35_flag
    }
}

/* ================= ESP INIT ================= */
void ESP_Init(void)
{
    UART2_SendString("ESP Init...\r\n");

    UART3_SendString("AT\r\n");
    delay(1000);

    UART3_SendString("AT+CWMODE=1\r\n");
    delay(1000);

    UART3_SendString("AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASS "\"\r\n");
    delay(6000);

    UART3_SendString("AT+CIPMUX=0\r\n");
    delay(1000);

    inet_flag = 1;  // Wi-Fi ready
    UART2_SendString("ESP Ready\r\n");
}

/* ================= CLOUD SERVER (BAREMETAL) ================= */
void Cloud_Server_BareMetal(void)
{
    static uint32_t rate_delay = 0;
    char buf[10];

    if(rate_delay > 0)
    {
        rate_delay--;
        return;
    }

    if((inet_flag == 1) && (adc_flag == 1))
    {
        adc_flag = 0;
        rate_delay = 2000;   // ~2 sec gap

        uint16_t percent = (adc_val * 100) / 4095;

        UART3_SendString("AT+CIPSTART=\"TCP\",\"" SERVER_IP "\",8080\r\n");
        delay(1500);

        UART3_SendString("AT+CIPSEND=100\r\n");
        delay(500);

        UART3_SendString("GET /page?pot=");
        int_to_str(percent, buf);
        UART3_SendString(buf);
        UART3_SendString(" HTTP/1.1\r\n");
        UART3_SendString("Host: " SERVER_IP "\r\n");
        UART3_SendString("Connection: close\r\n\r\n");

        delay(1000);
        UART3_SendString("AT+CIPCLOSE\r\n");

        UART2_SendString("Cloud Sent: ");
        UART2_SendString(buf);
        UART2_SendString("\r\n");
    }
}


/* ================= MAIN ================= */
int main(void)
{
    UART2_Init();
    UART3_Init();
    ADC_Init();
    ESP_Init();

    UART2_SendString("System Started\r\n");

    while(1)
    {
        Cloud_Server_BareMetal();
        delay(2000);
    }
}
