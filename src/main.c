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
#include <stdlib.h>

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

uint16_t display[34] = {
        0x002, // Command to set the cursor at the first position line 1
        0x200+'E', 0x200+'C', 0x200+'E', 0x200+'3', 0x200+'6', + 0x200+'2', 0x200+' ', 0x200+'i',
        0x200+'s', 0x200+' ', 0x200+'t', 0x200+'h', + 0x200+'e', 0x200+' ', 0x200+' ', 0x200+' ',
        0x0c0, // Command to set the cursor at the first position line 2
        0x200+'c', 0x200+'l', 0x200+'a', 0x200+'s', 0x200+'s', + 0x200+' ', 0x200+'f', 0x200+'o',
        0x200+'r', 0x200+' ', 0x200+'y', 0x200+'o', + 0x200+'u', 0x200+'!', 0x200+' ', 0x200+' ',
}; //display for OLED font

//DECLARATIONS
void init_input(void);
void setup_adc(void);
void init_i2c(void);
void i2c_start(uint32_t targadr, uint8_t size, uint8_t dir);
void i2c_stop(void);
void i2c_waitidle(void);
int i2c_recvdata(uint8_t targadr, void *data, uint8_t size);
int8_t i2c_senddata(uint8_t targadr, uint8_t data[], uint8_t size);
int i2c_checknack(void);
void i2c_clearnack(void);
void init_spi1();
void spi_cmd(unsigned int data);
void spi_data(unsigned int data);
void spi1_init_oled();
void spi1_display1(const char *string);
void spi1_display2(const char *string);
void spi1_setup_dma(void);
void spi1_enable_dma(void);

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
    GPIOA->MODER &= ~0xC000CC00;  // Clear mode bits
    GPIOA->MODER |= 0x80008800;     // Set alternate function mode (10) for PA15, PA5, PA7
    GPIOA->AFR[1] &= ~0xF0000000;    // Clear AFRH bits for PA15 (AF0)
    GPIOA->AFR[0] &= ~0xF0F00000;    // Clear AFRL bits for PA5 and PA7 (AF0)

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
    GPIOB->MODER &= ~0x30C30000;  // Clear PB8, PB11, PB14
    GPIOB->MODER |= 0x20420000;     // Set PB8, PB11, PB14 for output (01)

    // Call init_spi1() to configure SPI1 with specified settings
    init_spi1();
}

void init_i2c(void) {
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    I2C1->CR1 &= ~I2C_CR1_PE; //disable I2C first
    I2C1->CR1 |= I2C_CR1_ERRIE; //error interrupt enabled
    I2C1->CR1 |= I2C_CR1_ANFOFF; //disable analog filter
    I2C1->CR1 |= I2C_CR1_NOSTRETCH; //disable clock stretching
    I2C1->TIMINGR |= 0x50074883; //6 * 8Mhz = 48Mhz clock, 400 kHZ fast mode (supported by EEPROM)
    I2C1->CR2 &= ~I2C_CR2_ADD10; //7-bit addressing
    I2C1->CR2 |= I2C_CR2_AUTOEND; //send stop after last NBytes is transferred
    I2C1->CR1 |= I2C_CR1_PE; //enable I2C
}

void i2c_start(uint32_t targadr, uint8_t size, uint8_t dir) {
    uint32_t tempCR2 = I2C1->CR2;
    tempCR2 &= ~0xFF67FF; //clearing NBytes, STOP, START, RD_WRN, SADD
    if (dir){
        tempCR2 |= I2C_CR2_RD_WRN; //operation is a read
    }
    tempCR2 |= ((targadr<<1) & I2C_CR2_SADD) | ((size << 16) & I2C_CR2_NBYTES); //set target address and data size
    tempCR2 |= I2C_CR2_START; //prepare start of read/write
    I2C1->CR2 = tempCR2;
}

void i2c_stop(void) {
      //Check if stop bit is set
    if (I2C1->ISR & I2C_ISR_STOPF) {
        return;
    }
    I2C1->CR2 |= I2C_CR2_STOP;
    while (!(I2C1->ISR & I2C_ISR_STOPF)); //wait for STOP 
    I2C1->ICR |= I2C_ICR_STOPCF; //clear stop flag
}

void i2c_waitidle(void) {
    while ((I2C1->ISR & I2C_ISR_BUSY));
}

int8_t i2c_senddata(uint8_t targadr, uint8_t data[], uint8_t size) {
    i2c_waitidle(); //wait for I2C to be idle
    i2c_start(targadr, size, 0); //send START with write bit (dir = 0)
    uint8_t i;
    for (i = 0; i < size; i++)
    {
        int count = 0;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
        count += 1;
        if (count > 1000000)
            return -1;
        if (i2c_checknack()) {
            i2c_clearnack();
            i2c_stop();
            return -1;
    }   
}
I2C1->TXDR = data[i] & I2C_TXDR_TXDATA; //mask data[i] with TXDR_TXDATA
}
while (((I2C1->ISR & I2C_ISR_TC) && (I2C1->ISR & I2C_ISR_NACKF))); //wait for these to be not set

if (I2C1->ISR & I2C_ISR_NACKF){
        i2c_stop();
        return -1; //write failed
    }
    i2c_stop(); //send STOP
    return 0; //0 for success
}

int i2c_recvdata(uint8_t targadr, void *data, uint8_t size) {
    i2c_waitidle(); //wait for I2C to be idle
    i2c_start(targadr, size, 1); //send START with read bit (dir = 1)

    uint8_t i;
    for (i = 0; i < size; i++)
    {
    int count = 0;
    while ((I2C1->ISR & I2C_ISR_RXNE) == 0) {
        count += 1;
        if (count > 100000000)
            return -1;
        if (i2c_checknack()) {
            i2c_clearnack();
            i2c_stop();
            return -1;
    }
}  
  ((uint8_t*)data)[i] = (I2C1->RXDR & I2C_RXDR_RXDATA); //store masked data
}
i2c_stop();
return 0;
}

void i2c_clearnack(void) {
    I2C1->ICR |= I2C_ICR_NACKCF; // Clear the NACK flag
}
int i2c_checknack(void) {
    return ((I2C1->ISR & I2C_ISR_NACKF) ? 1 : 0);
}



//SPI LAB FUNCTIONS FOR OLED
void init_spi1() {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    GPIOA->MODER &= ~0xC000CC00;
    GPIOA->MODER |= 0x80008800;
    GPIOA->AFR[1] &= ~0xF0000000;
    GPIOA->AFR[0] &= ~0xF0F00000;
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 |= SPI_CR1_BR | SPI_CR1_MSTR; //BR to 111 and Master Mode
    SPI1->CR2 |= SPI_CR2_TXDMAEN | SPI_CR2_NSSP | SPI_CR2_SSOE; //NSSP and SSOE bit and enable TXDMAEN
    SPI1->CR2 |= 0xF00; //1111 in DS
    SPI1->CR2 &= ~0x600; //1001 in DS
    SPI1->CR1 |= SPI_CR1_SPE;
}
void spi_cmd(unsigned int data) {
    while((SPI1->SR & SPI_SR_TXE) == 0);
    SPI1->DR = data;
}
void spi_data(unsigned int data) {
    spi_cmd(data | 0x200);
}
void spi1_init_oled() {
    nano_wait(1000000);
    spi_cmd(0x38);
    spi_cmd(0x08);
    spi_cmd(0x01);
    nano_wait(2000000);
    spi_cmd(0x06);
    spi_cmd(0x02);
    spi_cmd(0x0c);
}
void spi1_display1(const char *string) {
    spi_cmd(0x00);
    while (*string != '\0')
    {
        spi_data(*string);
        string++;
    }
}
void spi1_display2(const char *string) {
    spi_cmd(0xc0);
    while (*string != '\0')
    {
        spi_data(*string);
        string++;
    }
}

void spi1_setup_dma(void) {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    DMA1_Channel3->CMAR = (uint32_t)display;
    DMA1_Channel3->CPAR = (uint32_t)&SPI1->DR;
    DMA1_Channel3->CNDTR = 34;
    DMA1_Channel3->CCR |= DMA_CCR_DIR;
    DMA1_Channel3->CCR |= DMA_CCR_MINC;
    DMA1_Channel3->CCR |= 0x500; //MSIZE and PSIZE-> 16 bits
    DMA1_Channel3->CCR |= DMA_CCR_CIRC;
    SPI1->CR2 |= SPI_CR2_TXDMAEN;
}

void spi1_enable_dma(void) {
    DMA1_Channel3->CCR |= DMA_CCR_EN;
}