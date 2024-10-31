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

#define DISPLAY_WIDTH  240 
#define DISPLAY_HEIGHT 240
#define GRID_SIZE      4
#define CELL_SIZE (DISPLAY_WIDTH / GRID_SIZE)

#define BACKGROUND_COLOR 0xC618  // Light gray for empty tiles
#define COLOR_2         0xFFFF  // White for 2
#define COLOR_4         0xFFE0  // Light yellow for 4
#define COLOR_8         0xFC00  // Orange for 8
#define COLOR_16        0xFA00  // Dark orange for 16
#define COLOR_32        0xF800  // Red for 32
#define COLOR_64        0xF81F  // Pink for 64
#define COLOR_128       0x07FF  // Cyan for 128
#define COLOR_256       0x001F  // Blue for 256
#define COLOR_512       0x07E0  // Green for 512
#define COLOR_1024      0xFFFF  // White for 1024
#define COLOR_2048      0xFFE0  // Gold for 2048

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

int calcScore(int current_score, int merge_val){
  merge_val = merge_val * 2;
  return current_score + merge_val;
}

void render_board(uint8_t board[4][4]) {
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            uint16_t x = col * CELL_SIZE;  // Calculate x position
            uint16_t y = row * CELL_SIZE;  // Calculate y position
            draw_tile(x, y, board[row][col]);  // Draw tile at x, y with value
        }
    }
}

void draw_tile(uint16_t x, uint16_t y, uint16_t value) {
    // Clear cell area first, then draw value
    lcd_fill_rect(x, y, CELL_SIZE, CELL_SIZE, BACKGROUND_COLOR);  // Fill tile background
    lcd_draw_number(x, y, value);  // Draw number or tile
}

uint16_t get_tile_color(uint16_t value) {
    switch (value) {
        case 2:    return COLOR_2;
        case 4:    return COLOR_4;
        case 8:    return COLOR_8;
        case 16:   return COLOR_16;
        case 32:   return COLOR_32;
        case 64:   return COLOR_64;
        case 128:  return COLOR_128;
        case 256:  return COLOR_256;
        case 512:  return COLOR_512;
        case 1024: return COLOR_1024;
        case 2048: return COLOR_2048;
        default:   return BACKGROUND_COLOR;
    }
}

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    // Set the LCD to the desired area
    lcd_set_address_window(x, y, x + width - 1, y + height - 1);

    // Send color data for each pixel in the rectangle
    for (uint32_t i = 0; i < width * height; i++) {
        lcd_send_data(color >> 8);  // Send the high byte
        lcd_send_data(color & 0xFF); // Send the low byte
    }
}

void lcd_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Command to set column address
    lcd_send_command(0x2A);  // Column Address Set
    lcd_send_data(x0 >> 8);  // Start column high byte
    lcd_send_data(x0 & 0xFF); // Start column low byte
    lcd_send_data(x1 >> 8);  // End column high byte
    lcd_send_data(x1 & 0xFF); // End column low byte

    // Command to set row address
    lcd_send_command(0x2B);  // Page Address Set
    lcd_send_data(y0 >> 8);  // Start row high byte
    lcd_send_data(y0 & 0xFF); // Start row low byte
    lcd_send_data(y1 >> 8);  // End row high byte
    lcd_send_data(y1 & 0xFF); // End row low byte

    // Command to start writing to memory
    lcd_send_command(0x2C);  // Memory Write
}

void lcd_send_command(uint8_t command) {
    GPIOB->ODR &= ~(1 << 11); // Set DC low for command
    GPIOB->ODR &= ~(1 << 8);  // Set CS low
    SPI1->DR = command;       // Send command
    while (!(SPI1->SR & SPI_SR_TXE)); // Wait for transmission to complete
    GPIOB->ODR |= (1 << 8);   // Set CS high
}

void lcd_send_data(uint8_t data) {
    GPIOB->ODR |= (1 << 11);  // Set DC high for data
    GPIOB->ODR &= ~(1 << 8);  // Set CS low
    SPI1->DR = data;          // Send data
    while (!(SPI1->SR & SPI_SR_TXE)); // Wait for transmission to complete
    GPIOB->ODR |= (1 << 8);   // Set CS high
}

const uint8_t font_data[10][5] = {
    // 5x7 pixel representations for digits 0-9
    {0x3E, 0x51, 0x49, 0x45, 0x3E},  // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00},  // 1
    {0x42, 0x61, 0x51, 0x49, 0x46},  // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31},  // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10},  // 4
    {0x27, 0x45, 0x45, 0x45, 0x39},  // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30},  // 6
    {0x01, 0x71, 0x09, 0x05, 0x03},  // 7
    {0x36, 0x49, 0x49, 0x49, 0x36},  // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}   // 9
};

void lcd_draw_number(uint16_t x, uint16_t y, uint16_t number) {
    char str[5];  // Buffer to hold number as string
    sprintf(str, "%u", number);  // Convert number to string
    
    for (int i = 0; str[i] != '\0'; i++) {
        int digit = str[i] - '0';
        lcd_draw_digit(x + i * 6, y, digit);  // Space each digit by 6 pixels
    }
}

void lcd_draw_digit(uint16_t x, uint16_t y, uint8_t digit) {
    if (digit > 9) return;  // Invalid digit
    
    for (int col = 0; col < 5; col++) {
        uint8_t line = font_data[digit][col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                lcd_set_pixel(x + col, y + row, 0x0000);  // Black for the digit
            } else {
                lcd_set_pixel(x + col, y + row, BACKGROUND_COLOR);  // Background color
            }
        }
    }
}

void lcd_set_pixel(uint16_t x, uint16_t y, uint16_t color) {
    lcd_set_address_window(x, y, x, y);  // Set window to a single pixel
    lcd_send_data(color >> 8);           // Send high byte
    lcd_send_data(color & 0xFF);         // Send low byte
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

//===========================================================================
// SPI1 TFT Display
//===========================================================================

void init_spi1(void) {
    // Enable clocks for SPI1 and GPIOA
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;      // Enable SPI1 clock
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;     // Enable GPIOA clock

    // Configure GPIOA pins PA15 (SCK), PA5 (MISO), and PA7 (MOSI) for SPI1
    GPIOA->MODER &= ~((3UL << (15 * 2)) | (3UL << (5 * 2)) | (3UL << (7 * 2)));  // Clear mode bits
    GPIOA->MODER |= (2UL << (15 * 2)) | (2UL << (5 * 2)) | (2UL << (7 * 2));     // Set alternate function mode (10) for PA15, PA5, PA7
    GPIOA->AFR[1] &= ~(0xFUL << ((15 - 8) * 4));                                 // Clear AFRH bits for PA15
    GPIOA->AFR[0] &= ~((0xFUL << (5 * 4)) | (0xFUL << (7 * 4)));                 // Clear AFRL bits for PA5 and PA7
    GPIOA->AFR[1] |= (5UL << ((15 - 8) * 4));                                    // Set AF5 for SPI1 on PA15 (AFRH)
    GPIOA->AFR[0] |= (5UL << (5 * 4)) | (5UL << (7 * 4));                        // Set AF5 for SPI1 on PA5 and PA7 (AFRL)

    // Configure SPI1
    SPI1->CR1 &= ~SPI_CR1_SPE;

    // Set baud rate to maximum divisor (24MHz speed)
    SPI1->CR1 &= ~SPI_CR1_BR;

    SPI1->CR1 |= SPI_CR1_MSTR;              // Master selection
    SPI1->CR2 |= SPI_CR2_DS;                // 8-bit data frame format
    SPI1->CR2 &= ~SPI_CR2_DS_3;

    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI; // SSM: Software slave management, SSI: Internal slave select

    SPI1->CR2 |= SPI_CR2_FRXTH;             //8-bit threshold for FIFO reception

    SPI1->CR1 |= SPI_CR1_SPE;               // Enable SPI
}

void init_lcd_spi(void) {
    // Enable the clock for GPIOB
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    // Configure PB8, PB11, and PB14 as general-purpose outputs for CS, DC/RS, and RST
    GPIOB->MODER &= ~((3UL << (8 * 2)) | (3UL << (11 * 2)) | (3UL << (14 * 2)));  // Clear mode bits
    GPIOB->MODER |= (1UL << (8 * 2)) | (1UL << (11 * 2)) | (1UL << (14 * 2));     // Set as output mode (01)

    // Call init_spi1() to configure SPI1 with specified settings
    init_spi1();
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