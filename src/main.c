/**
  ******************************************************************************
  * @file    main.c
  * @author  Weili An, Niraj Menon
  * @date    Feb 7, 2024
  * @brief   ECE 362 Lab 7 student template
  ******************************************************************************
*/

/*******************************************************************************/

// Fill out your username!  Even though we're not using an autotest, 
// it should be a habit to fill out your username in this field now.
const char* username = "mcdonou5";

/*******************************************************************************/ 

#include "stm32f0xx.h"
#include <stdint.h>

//DECLARATIONS
void init_input(void);
void setup_adc(void)

// Returns 2 with 0.9 probability or 4 with 0.1 probability
int getRandomNumber() {
    return (random(0, 10) < 9) ? 2 : 4;
}

void internal_clock();

int main(void){
    internal_clock();

}

//PROJECT CODE


//from lab 4
void setup_adc(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER &= ~0xC;
    GPIOA->MODER |= 0xC;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->CR2 |= RCC_CR2_HSI14ON;
    while (!(RCC->CR2 & RCC_CR2_HSI14RDY));
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));
    ADC1->CHSELR = 0;
    ADC1->CHSELR = (1 << 1); 
    while (!(ADC1->ISR & ADC_ISR_ADRDY));
}

void init_input(void){
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC->MODER &= ~0xFF00; //clearing PC4-7
    GPIOC->MODER |= 0x5500; //PC4-7 configured as input
}