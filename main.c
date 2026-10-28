#include <stdint.h>
#include "stm32f10x.h"

void delay(volatile uint32_t count) {
    while (count--) {
        __asm volatile ("nop");
    }
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

uint8_t SPI1_Read(void) {
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = 0;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return (uint8_t)SPI1->DR;
}

void display_cmd(uint8_t cmd) {
    GPIOA->ODR &= ~GPIO_ODR_ODR4;
    GPIOA->ODR &= ~GPIO_ODR_ODR1;
    SPI1_Write(cmd);
    while (SPI1->SR & SPI_SR_BSY);
    GPIOA->ODR |= GPIO_ODR_ODR4;
}

void display_data(uint8_t data) {
    GPIOA->ODR &= ~GPIO_ODR_ODR4;
    GPIOA->ODR |= GPIO_ODR_ODR1;
    SPI1_Write(data);
    while (SPI1->SR & SPI_SR_BSY);
    GPIOA->ODR |= GPIO_ODR_ODR4;
}

void SSD1306_Init(void) {
    GPIOA->ODR |= GPIO_ODR_ODR2;
    delay(1000);
    GPIOA->ODR &= ~GPIO_ODR_ODR2;
    delay(1000);
    GPIOA->ODR |= GPIO_ODR_ODR2;
    delay(1000);

    display_cmd(0xAE);
    display_cmd(0x20);
    display_cmd(0x00);
    display_cmd(0x8D);
    display_cmd(0x14);
    display_cmd(0xAF);
}

void SSD1306_DrawChessboard(void) {
    display_cmd(0x21);
    display_cmd(0);
    display_cmd(127);

    display_cmd(0x22);
    display_cmd(0);
    display_cmd(7);

    GPIOA->ODR &= ~GPIO_ODR_ODR4;
    GPIOA->ODR |= GPIO_ODR_ODR1;

    for (uint8_t page = 0; page < 8; page++) {
        for (uint8_t col = 0; col < 128; col++) {
            if (((page + (col / 8)) % 2) == 0) {
                SPI1_Write(0xFF);
            } else {
                SPI1_Write(0x00);
            }
        }
    }

    while (SPI1->SR & SPI_SR_BSY);
    GPIOA->ODR |= GPIO_ODR_ODR4;  // CS = 1

    __asm volatile ("wfi");
}

int main(void) {
    SPI1_Init();
    SSD1306_Init();
    SSD1306_DrawChessboard();

    while (1) {
        __asm volatile ("wfi");
    }
}