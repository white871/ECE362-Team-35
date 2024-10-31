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
void init_i2c(void);
void start_i2c(uint32_t targadr, uint8_t size, uint8_t dir);
void stop_i2c(void);
void i2c_waitidle(void);

// Returns 2 with 0.9 probability or 4 with 0.1 probability
int getRandomNumber() {
    return (random(0, 10) < 9) ? 2 : 4;
}

void internal_clock();

int main(void){
    internal_clock();
    enable_ports();

}

//PROJECT CODE


//from lab 4
void setup_adc(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER &= ~0xC; //clear PA1 
    GPIOA->MODER |= 0xC; //PA1 to analog
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->CR2 |= RCC_CR2_HSI14ON; //14 MHz clock
    while (!(RCC->CR2 & RCC_CR2_HSI14RDY));
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));
    ADC1->CHSELR = 0;
    ADC1->CHSELR = (1 << 1); 
    while (!(ADC1->ISR & ADC_ISR_ADRDY));
}

void init_i2c(void){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER &= ~0x3C0000;
    GPIOA->MODER |= 0x280000; //PA9 and PA10 in alternate function
    GPIOA->AFR[1] |= 0x440; //PA9 and PA10 to AF4
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    I2C1->CR1 &= ~I2C_CR1_PE; //disable I2C first
    I2C1->CR1 |= I2C_CR1_ERRIE; //error interrupts enabled
    I2C1->CR1 |= I2C_CR1_ANFOFF; //disable analog filter
    I2C1->TIMINGR |= 0x501F0204; //6 * 8Mhz = 48Mhz clock, 400 kHZ fast mode (supported by EEPROM)
    I2C1->CR2 &= ~I2C_CR2_ADD10; //7-bit addressing
    I2C1->CR2 |= I2C_CR2_AUTOEND; //send stop after last NBytes is transferred
    I2C1->CR1 |= I2C_CR1_PE; //enable I2C
}

void start_i2c(uint32_t targadr, uint8_t size, uint8_t dir){
    uint32_t tempCR2 = I2C1->CR2;
    tempCR2 &= ~0xFF67FF; //clearing NBytes, STOP, START, RD_WRN, SADD
    if (dir){
        tempCR2 |= I2C_CR2_RD_WRN; //operation is a read
    }
    tempCR2 |= ((targadr<<1) & I2C_CR2_SADD) | ((size << 16) & I2C_CR2_NBYTES); //set target address and data size
    tempCR2 |= I2C_CR2_START; //prepare start of read/write
    I2C1->CR2 = tempCR2;
}

void stop_i2c(void){
    //Check if stop bit is set
    if (I2C1->ISR & I2C_ISR_STOPF) {
        return;
    }
    I2C1->CR2 |= I2C_CR2_STOP;
    while (!(I2C1->ISR & I2C_ISR_STOPF)); //wait for STOP 
    I2C1->ICR |= I2C_ICR_STOPCF;
}

void i2c_waitidle(void){
    while ((I2C1->ISR & I2C_ISR_BUSY));
}