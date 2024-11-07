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
void setup_adc(void);
void init_tim2(void);

// Returns 2 with 0.9 probability or 4 with 0.1 probability
int getRandomNumber() {
    return (random(0, 10) < 9) ? 2 : 4;
}

void internal_clock();
int xvalue = 0;
int yvalue = 0;
int main(void){
    internal_clock();
    RCC -> AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC -> MODER |= GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0;
    setup_adc();
    init_tim2();
    
}

//PROJECT CODE


//from lab 4
void setup_adc(void) {
    //ADC_IN6, ADC_IN7 -> PA6, PA7
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA -> MODER |= GPIO_MODER_MODER6_1 | GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER7_1;
    RCC -> APB2ENR |= RCC_APB2ENR_ADCEN;
    RCC->CR2 |= RCC_CR2_HSI14ON;
    while (!(RCC->CR2 & RCC_CR2_HSI14RDY));
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));
    ADC1 -> CHSELR = 0;
    ADC1 -> CHSELR |= ADC_CHSELR_CHSEL6 | ADC_CHSELR_CHSEL7;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));

}
// Test function for reading left, right, up, and down
void setLights(int xvalue, int yvalue)
{
    if (xvalue > 3000)
    {
        GPIOC -> BSRR |= GPIO_BSRR_BS_6;
        GPIOC -> BRR |= GPIO_BRR_BR_7;
        GPIOC -> BRR |= GPIO_BRR_BR_8;
        GPIOC -> BRR |= GPIO_BRR_BR_9;
    } else if (xvalue < 1000)
    {
        GPIOC -> BRR |= GPIO_BRR_BR_6;
        GPIOC -> BSRR |= GPIO_BSRR_BS_7;
        GPIOC -> BRR |= GPIO_BRR_BR_8;
        GPIOC -> BRR |= GPIO_BRR_BR_9;
    }
    else if (yvalue > 3000)
    {
        GPIOC -> BRR |= GPIO_BRR_BR_6;
        GPIOC -> BRR |= GPIO_BRR_BR_7;
        GPIOC -> BSRR |= GPIO_BSRR_BS_8;
        GPIOC -> BRR |= GPIO_BRR_BR_9;
    } else if (yvalue < 1000)
    {
        GPIOC -> BRR |= GPIO_BRR_BR_6;
        GPIOC -> BRR |= GPIO_BRR_BR_7;
        GPIOC -> BRR |= GPIO_BRR_BR_8;
        GPIOC -> BSRR |= GPIO_BSRR_BS_9;
    } else 
    {
        GPIOC -> BRR |= GPIO_BRR_BR_6;
        GPIOC -> BRR |= GPIO_BRR_BR_7;
        GPIOC -> BRR |= GPIO_BRR_BR_8;
        GPIOC -> BRR |= GPIO_BRR_BR_9;
    }
}
void TIM2_IRQHandler()
{
    TIM2 -> SR &= ~TIM_SR_UIF;
    ADC1 -> CR |= ADC_CR_ADSTART;
    while ((ADC1 -> ISR & ADC_ISR_EOC) == 0);
    yvalue = ADC1 -> DR;
    while ((ADC1 -> ISR & ADC_ISR_EOC) == 0);
    xvalue = ADC1 -> DR;
    setLights(xvalue, yvalue);
}

void init_tim2(void)
{
    RCC -> APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2 -> PSC = 4800 - 1;
    TIM2 -> ARR = 1000 - 1;
    TIM2 -> DIER |= TIM_DIER_UIE;

    TIM2 -> CR1 |= TIM_CR1_CEN;
    NVIC_EnableIRQ(TIM2_IRQn);
}

