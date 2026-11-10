#include <stdint.h>
#include "stm32f10x.h"

#define FONT_WIDTH 5
const uint8_t digits_font[10][FONT_WIDTH] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}  // '9'
};

volatile uint8_t current_digit = 0;

void delay(volatile uint32_t count) {
    while (count--) {
        __asm volatile ("nop");
    }
}

uint8_t bitrev(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

void SPI1_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_SPI1EN;

    GPIOA->CRL &= ~(
        (GPIO_CRL_CNF1 | GPIO_CRL_MODE1) |
        (GPIO_CRL_CNF2 | GPIO_CRL_MODE2) |
        (GPIO_CRL_CNF4 | GPIO_CRL_MODE4) |
        (GPIO_CRL_CNF5 | GPIO_CRL_MODE5) |
        (GPIO_CRL_CNF7 | GPIO_CRL_MODE7)
    );

    GPIOA->CRL |= GPIO_CRL_MODE1 | GPIO_CRL_MODE2 | GPIO_CRL_MODE4;

    GPIOA->CRL |= (GPIO_CRL_CNF5_1 | GPIO_CRL_MODE5) |
                  (GPIO_CRL_CNF7_1 | GPIO_CRL_MODE7);

    GPIOA->ODR |= GPIO_ODR_ODR4 | GPIO_ODR_ODR2;

    SPI1->CR1 = SPI_CR1_CPOL | SPI_CR1_CPHA |
                SPI_CR1_MSTR |
                SPI_CR1_SSM  | SPI_CR1_SSI |
                SPI_CR1_SPE;
}

void SPI1_Write(uint8_t data) {
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;
}

void cmd(uint8_t c) {
    GPIOA->ODR &= ~GPIO_ODR_ODR4;
    GPIOA->ODR &= ~GPIO_ODR_ODR1;
    SPI1_Write(c);
    while (SPI1->SR & SPI_SR_BSY);
    GPIOA->ODR |= GPIO_ODR_ODR4;
}

void dat(uint8_t d) {
    GPIOA->ODR &= ~GPIO_ODR_ODR4;
    GPIOA->ODR |= GPIO_ODR_ODR1;
    SPI1_Write(d);
    while (SPI1->SR & SPI_SR_BSY);
    GPIOA->ODR |= GPIO_ODR_ODR4;
}

void SSD1306_SetPos(uint8_t page, uint8_t col) {
    cmd(0xB0 | (page & 0x07));
    cmd(0x10 | ((col >> 4) & 0x0F));
    cmd(0x00 | (col & 0x0F));
}

void SSD1306_Clear(void) {
    for (uint8_t p = 0; p < 8; p++) {
        SSD1306_SetPos(p, 0);
        GPIOA->ODR &= ~GPIO_ODR_ODR4;
        GPIOA->ODR |= GPIO_ODR_ODR1;
        for (uint8_t i = 0; i < 128; i++) {
            SPI1_Write(0x00);
        }
        while (SPI1->SR & SPI_SR_BSY);
        GPIOA->ODR |= GPIO_ODR_ODR4;
    }
}

void SSD1306_Init(void) {
    GPIOA->ODR |= GPIO_ODR_ODR2;
    delay(1000);
    GPIOA->ODR &= ~GPIO_ODR_ODR2;
    delay(1000);
    GPIOA->ODR |= GPIO_ODR_ODR2;
    delay(1000);

    cmd(0xAE);
    cmd(0x20);
    cmd(0x02);
    cmd(0x8D);
    cmd(0x14);
    cmd(0xAF);

    SSD1306_Clear();
}

void PutChar(char x) {
    uint8_t digit = (x >= '0' && x <= '9') ? (x - '0') : (x % 10);
    const uint8_t *pData = digits_font[digit];

    SSD1306_SetPos(0, 0);
    for (int i = 0; i < FONT_WIDTH; i++) {
        dat(pData[i]);
    }
    dat(0x00);

    for (uint8_t page_offset = 0; page_offset < 4; page_offset++) {
        SSD1306_SetPos(2 + page_offset, 54);
        for (uint8_t col = 0; col < FONT_WIDTH; col++) {
            uint8_t two_bits = (pData[col] >> (page_offset * 2)) & 0x03;
            uint8_t scaled_byte = 0;
            if (two_bits & 0x01) scaled_byte |= 0x0F;
            if (two_bits & 0x02) scaled_byte |= 0xF0;
            for (uint8_t w = 0; w < 4; w++) {
                dat(scaled_byte);
            }
        }
        dat(0x00);
    }
}

void TIM1_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    RCC->APB2RSTR |= RCC_APB2RSTR_TIM1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_TIM1RST;

    TIM1->PSC = 7200 - 1;
    TIM1->ARR = 20000 - 1;

    TIM1->DIER |= TIM_DIER_UIE;

    NVIC_ClearPendingIRQ(TIM1_UP_IRQn);
    NVIC_EnableIRQ(TIM1_UP_IRQn);

    TIM1->CR1 |= TIM_CR1_CEN;
}

void TIM1_UP_IRQHandler(void) {
    if (TIM1->SR & TIM_SR_UIF) {
        TIM1->SR &= ~TIM_SR_UIF;
        current_digit = (current_digit + 1) % 10;
        PutChar('0' + current_digit);
    }
}

int main(void) {
    SPI1_Init();
    SSD1306_Init();

    current_digit = 0;
    PutChar('0');

    TIM1_Init();

    while (1) {
        __asm volatile ("nop");
    }
}