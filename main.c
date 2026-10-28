#include <stdint.h>
#include "stm32f10x.h"

// Ассемблерная задержка со слайда 10
void delay_us(uint32_t us) {
    __asm volatile (
        "push {r0}\r\n"
        "mov r0, %0\r\n"
        "1:\r\n"
        "cmp r0, #0\r\n"
        "beq 2f\r\n"
        "sub r0, r0, #1\r\n"
        "nop\r\n"
        "b 1b\r\n"
        "2:\r\n"
        "pop {r0}\r\n"
        :: "r" (9 * us)
    );
}

int main(void) {
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPAEN;

    GPIOC->CRH = (GPIOC->CRH & ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13)) | GPIO_CRH_MODE13_0;

    GPIOA->CRL = (GPIOA->CRL & ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0)) | GPIO_CRL_CNF0_1;
    GPIOA->CRL = (GPIOA->CRL & ~(GPIO_CRL_CNF1 | GPIO_CRL_MODE1)) | GPIO_CRL_CNF1_1;

    GPIOA->ODR |= GPIO_ODR_ODR0 | GPIO_ODR_ODR1;

    uint32_t target_steps = 64; 
    uint32_t current_step = 0;

    uint8_t btn_A_was_pressed = 0;
    uint8_t btn_C_was_pressed = 0;

    while (1) {
        if (!(GPIOA->IDR & GPIO_IDR_IDR0)) {
            if (!btn_A_was_pressed) {
                btn_A_was_pressed = 1;
                if (target_steps > 1) {
                    target_steps /= 2;
                }
            }
        } else {
            btn_A_was_pressed = 0;
        }

        if (!(GPIOA->IDR & GPIO_IDR_IDR1)) {
            if (!btn_C_was_pressed) {
                btn_C_was_pressed = 1;
                if (target_steps < 4096) {
                    target_steps *= 2;
                }
            }
        } else {
            btn_C_was_pressed = 0;
        }

        delay_us(7812);
        current_step++;

        if (current_step >= target_steps) {
            current_step = 0;
            GPIOC->ODR ^= GPIO_ODR_ODR13;
        }
    }
}