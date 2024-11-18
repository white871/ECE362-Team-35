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
#include <stdio.h>
#include "lcd.h"
#include <time.h>
#include <string.h>

#define EEPROM_ADDR 0x57
#define DISPLAY_WIDTH  240 
#define DISPLAY_HEIGHT 240
#define GRID_SIZE      4
#define CELL_SIZE (DISPLAY_WIDTH / GRID_SIZE)

#define BACKGROUND_COLOR 0xC618  // Light gray for empty tiles
#define BLACK           0x0000  //  Black for number and outline
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
void internal_clock(void);
int main(void);
void setup_adc(void);
void init_spi1(void);
void init_lcd_spi(void);
void init_i2c(void);
void i2c_start(uint32_t targadr, uint8_t size, uint8_t dir);
void i2c_stop(void);
void i2c_waitidle(void);
int i2c_recvdata(uint8_t targadr, void *data, uint8_t size);
int8_t i2c_senddata(uint8_t targadr, uint8_t data[], uint8_t size);
int i2c_checknack(void);
void i2c_clearnack(void);
void init_spi1_slow();
void spi_cmd(unsigned int data);
void spi_data(unsigned int data);
void spi1_init_oled();
void spi1_display1(const char *string);
void spi1_display2(const char *string);
void spi1_setup_dma(void);
void spi1_enable_dma(void);
void draw_tile(uint16_t x, uint16_t y, char* value, uint16_t color);
void render_board();
uint16_t get_tile_color(uint16_t value);
void shift_and_merge_left(void);
void shift_and_merge_right(void);
void shift_and_merge_up(void);
void shift_and_merge_down(void);
void make_move(char direction);
int is_game_over(void);
void add_random_tile();
void init_input(void);
void setup_adc(void);
void init_tim2(void);
void eeprom_write(uint16_t loc, const char* data, uint8_t len);
void eeprom_read(uint16_t loc, char data[], uint8_t len);
void enable_i2c_ports(void);
void compare_score(int current_score);
void calc_score();

//Variables
uint16_t board[4][4];
int is_move = 0;
uint8_t moved;
int score;

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

void display_game_over_with_graphics(void) {
    LCD_Clear(0x0000); // Clear the screen with black color

    // Draw filled rectangle as a background for text
    LCD_DrawFillRectangle(0, 0, 240, 320, RED); // Blue background

    // Display "GAME"
    LCD_DrawString(80, 130, WHITE, BLACK, "GAME", 16, 0); // White text

    // Display "OVER"
    LCD_DrawString(120, 130, WHITE, BLACK, "OVER", 16, 0); // White text
}


void make_move(char direction) {
    moved = 0;
    switch (direction) {
        case 'L': shift_and_merge_left(); break;
        case 'R': shift_and_merge_right(); break;
        case 'U': shift_and_merge_up(); break;
        case 'D': shift_and_merge_down(); break;
        default: return;
    }
    if(moved){
        add_random_tile();
    }
}

void calc_score(){
    int temp = 0;
    for(int row = 0; row < GRID_SIZE; row++){
        for(int col = 0; col < GRID_SIZE; col++){
            temp+=board[row][col];
        }
    }
    score = temp;
}


// Returns 2 with 0.9 probability or 4 with 0.1 probability
int getRandomNumber() {
    // Generate a random number between 0 and 9
    int randomValue = rand() % 10; // rand() % 10 gives a value between 0 and 9

    // Return 2 with a probability of 90% and 4 with a probability of 10%
    return (randomValue < 9) ? 2 : 4;
}

void add_random_tile() {
    int empty_count = 0;
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (board[row][col] == 0) empty_count++;
        }
    }

    if (empty_count == 0) return;  // No empty spots left

    int random_pos = rand() % empty_count;
    int count = 0;
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (board[row][col] == 0) {
                if (count == random_pos) {
                    board[row][col] = (rand() % 10 < 9) ? 2 : 4;  // 90% chance for 2, 10% for 4
                    return;
                }
                count++;
            }
        }
    }
}


void create_board() {
    // Initialize the board to all zeros
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            board[row][col] = 0;
        }
    }

    // Place two random numbers on the board
    for (int i = 0; i < 2; i++) {
        int row, col;

        // Find a unique empty spot
        do {
            row = rand() % GRID_SIZE; // Random row
            col = rand() % GRID_SIZE; // Random column
        } while (board[row][col] != 0); // Keep trying until we find an empty spot

        // Place a random number (2 or 4) in the selected spot
        board[row][col] = getRandomNumber();
    }
}

int calcScore(int current_score, int merge_val){
  merge_val = merge_val * 2;
  return current_score + merge_val;
}

void render_board() {
    char str[6];
    for (int col = 0; col < 4 ; col++) {
        for (int row = 0; row < 4; row++) {
            uint16_t x = row * CELL_SIZE;  // Calculate x position
            uint16_t y = col * CELL_SIZE;  // Calculate y position
            int num = board[row][col];
            snprintf(str, sizeof(str), "%d", num); // int to string
            draw_tile(x, y, str, get_tile_color(num));
        }
    }
    if(is_game_over()){
        display_game_over_with_graphics();
        while(1){}
    }else{
        calc_score();
        snprintf(str, sizeof(str), "%d", score);
        LCD_DrawString(80, 280, COLOR_8, COLOR_2, str, 16, 0);
    }
}    


void draw_tile(uint16_t x, uint16_t y, char* value, uint16_t color) {
    LCD_DrawFillRectangle(x, y, x + CELL_SIZE, y + CELL_SIZE, color);  // Fill tile background
    LCD_DrawRectangle(x, y, x + CELL_SIZE, y + CELL_SIZE, BLACK);  // Fill tile background
    //LCD_DrawChar(x, y, BLACK, get_tile_color(value), 48 + value, 16, 0);  // Draw number or tile
    LCD_DrawString(x + 15, y + 20, BLACK, color, value, 16, 0);
}

void internal_clock();
int xvalue = 0;
int yvalue = 0;
int main(void){
    internal_clock();
    init_spi1();
    spi1_setup_dma();
    spi1_enable_dma();
    spi1_init_oled();
    RCC -> AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC -> MODER |= GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0;

    spi1_display1("High Score: N/A");
    LCD_Setup();
    LCD_Clear(0000);
    create_board();
    setup_adc();
    init_tim2();
    //render_board();
    
    /*while(!is_game_over()){
        make_move('D');
        render_board();
    }*/
   while(1){
    
   }
}

//PROJECT CODE


//from lab 4
void setup_adc(void) {
    //ADC_IN6, ADC_IN7 -> PA6, PA7
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA -> MODER |= GPIO_MODER_MODER6_1 | GPIO_MODER_MODER6_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER4_1;
    RCC -> APB2ENR |= RCC_APB2ENR_ADCEN;
    RCC->CR2 |= RCC_CR2_HSI14ON;
    while (!(RCC->CR2 & RCC_CR2_HSI14RDY));
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));
    ADC1 -> CHSELR = 0;
    ADC1 -> CHSELR |= ADC_CHSELR_CHSEL6 | ADC_CHSELR_CHSEL4;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));

}
// Test function for reading left, right, up, and down
void setLights(int xvalue, int yvalue)
{
    if ((xvalue > 3000) & !is_move)
    {
        is_move = 1;
        make_move('R');
        /*GPIOC -> BSRR |= GPIO_BSRR_BS_6;
        GPIOC -> BRR |= GPIO_BRR_BR_7;
        GPIOC -> BRR |= GPIO_BRR_BR_8;
        GPIOC -> BRR |= GPIO_BRR_BR_9;*/
    } else if ((xvalue < 1000) & !is_move)
    {
        is_move = 1;
        make_move('L');
        // GPIOC -> BRR |= GPIO_BRR_BR_6;
        // GPIOC -> BSRR |= GPIO_BSRR_BS_7;
        // GPIOC -> BRR |= GPIO_BRR_BR_8;
        // GPIOC -> BRR |= GPIO_BRR_BR_9;
    }
    else if ((yvalue > 3000) & !is_move)
    {
        is_move = 1;
        make_move('D'); // U
        // GPIOC -> BRR |= GPIO_BRR_BR_6;
        // GPIOC -> BRR |= GPIO_BRR_BR_7;
        // GPIOC -> BSRR |= GPIO_BSRR_BS_8;
        // GPIOC -> BRR |= GPIO_BRR_BR_9;
    } else if ((yvalue < 1000) & !is_move)
    {
        is_move = 1;
        make_move('U'); // D
        // GPIOC -> BRR |= GPIO_BRR_BR_6;
        // GPIOC -> BRR |= GPIO_BRR_BR_7;
        // GPIOC -> BRR |= GPIO_BRR_BR_8;
        // GPIOC -> BSRR |= GPIO_BSRR_BS_9;
    } else 
    {
        is_move = 0;
        return;
        // GPIOC -> BRR |= GPIO_BRR_BR_6;
        // GPIOC -> BRR |= GPIO_BRR_BR_7;
        // GPIOC -> BRR |= GPIO_BRR_BR_8;
        // GPIOC -> BRR |= GPIO_BRR_BR_9;
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
    render_board();
}

//===========================================================================
// SPI1 TFT Display
//===========================================================================
void init_spi1_slow(){
    // Enable clocks for SPI1 and GPIOA
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN; // Enable SPI1 clock
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN; // Enable GPIOB clock

    // Clear mode bits for PB3, PB4, and PB5
    GPIOB->MODER &= ~((0x3 << 6) | (0x3 << 8) | (0x3 << 10));

    // Set alternate function mode (10) for PB3, PB4, and PB5
    GPIOB->MODER |= (0x2 << 6) | (0x2 << 8) | (0x2 << 10);

    // Clear AFRL bits for PB3 PB4 and PB5 (alternate function register low)
    GPIOB->AFR[0] &= ~((0xF << 12) | (0xF << 16) | (0xF << 20));

    // Configure SPI1
    SPI1->CR1 &= ~SPI_CR1_SPE;

    // Set baud rate to maximum divisor (24MHz speed)
    SPI1->CR1 |= (SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2);

    // Master selection
    SPI1->CR1 |= SPI_CR1_MSTR;
    
    // 8-bit data frame format
    SPI1->CR2 |= SPI_CR2_DS_0;
    SPI1->CR2 |= SPI_CR2_DS_1;
    SPI1->CR2 |= SPI_CR2_DS_2;
    SPI1->CR2 &= ~SPI_CR2_DS_3;

    // SSM: Software slave management, SSI: Internal slave select
    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;

    //8-bit threshold for FIFO reception
    SPI1->CR2 |= SPI_CR2_FRXTH;

    //SPI1->CR1 &= ~SPI_CR1_CPOL;
    //SPI1->CR1 &= ~SPI_CR1_CPHA;

    // Enable SPI
    SPI1->CR1 |= SPI_CR1_SPE;
}

void enable_sdcard(){
    GPIOB->BSRR |= GPIO_BSRR_BR_2;
}

void disable_sdcard(){
    GPIOB->BSRR |= GPIO_BSRR_BS_2;
}

void init_sdcard_io(){
    init_spi1_slow();
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~0x30;
    GPIOB->MODER |= 0x10;
    //GPIOB->PUPDR |= GPIO_PUPDR_PUPDR2_0;
    //GPIOB->PUPDR |= GPIO_PUPDR_PUPDR2_1;
    disable_sdcard();
}

void sdcard_io_high_speed(){
    SPI1->CR1 &= ~SPI_CR1_SPE;

    SPI1->CR1 &= ~SPI_CR1_BR_2;
    SPI1->CR1 &= ~SPI_CR1_BR_1;
    SPI1->CR1 |= SPI_CR1_BR_0;

    SPI1->CR1 |= SPI_CR1_SPE;
}

void init_lcd_spi(){
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~0x20830000;
    GPIOB->MODER |= 0x10410000;
    init_spi1_slow();
    sdcard_io_high_speed();
}

///////////

///////////

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

void eeprom_write(uint16_t loc, const char* data, uint8_t len) {
    uint8_t bytes[34];
    bytes[0] = loc>>8;
    bytes[1] = loc&0xFF;
    for(int i = 0; i<len; i++){
        bytes[i+2] = data[i];
    }
    i2c_senddata(EEPROM_ADDR, bytes, len+2);
}

void eeprom_read(uint16_t loc, char data[], uint8_t len) {
    uint8_t bytes[2];
    bytes[0] = loc>>8;
    bytes[1] = loc&0xFF;
    i2c_senddata(EEPROM_ADDR, bytes, 2);
    i2c_recvdata(EEPROM_ADDR, data, len);
}

void enable_i2c_ports(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER &= ~0x3C0000;
    GPIOA->MODER |= 0x280000; //PA9 and PA10 in alternate function
    GPIOA->AFR[1] |= 0x440; //PA9 and PA10 to AF4
    GPIOA->PUPDR |= 0x140000;
}

void compare_score(int current_score)
{   
    char old_score[5];
    eeprom_read(0x0, old_score, 5);
    char current_string[6];
    snprintf(current_string, 6, "%d", current_score);
    int current_high = atoi(old_score);
    char hiState[15] = "High Score:";
    char currState[15] = "Score:";
    char test[5] = "0786888";
    char copy[6]; 
    if (current_score > 0) //change to > current_high in actual application
    { 
      //  nano_wait(10000000000);
        eeprom_write(0x0, current_string, 6);
        strcpy(copy, current_string);
        strcat(hiState, current_string);
        spi1_display1(hiState);
        strcat(currState, copy);
        spi1_display2(currState);

    }
    else
    {   
     //   nano_wait(10000000000);
        eeprom_write(0x0, old_score, 6);
        strcat(hiState, old_score);
        spi1_display1(hiState);
    }
}

//Game Logic
int is_game_over() {
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (board[row][col] == 0) return 0;
            if ((col < GRID_SIZE - 1) && board[row][col] == board[row][col + 1]) return 0;
            if (row < GRID_SIZE - 1 && board[row][col] == board[row + 1][col]) return 0;
        }
    }
    return 1;  // No moves left
}

void shift_and_merge_up() {
    for (int row = 0; row < GRID_SIZE; row++) {
        int target = 0;
        for (int col = 1; col < GRID_SIZE; col++) {
            if (board[row][col] != 0) {
                int current = col;
                while (current > target && board[row][current - 1] == 0) {
                    board[row][current - 1] = board[row][current];
                    board[row][current] = 0;
                    current--;
                    moved = 1;
                }
                
                if (current > 0 && board[row][current - 1] == board[row][current]) {
                    board[row][current - 1] *= 2;
                    board[row][current] = 0;
                    target = current;
                    moved = 1;
                }
            }
        }
    }
}

void shift_and_merge_down() {
    for (int row = 0; row < GRID_SIZE; row++) {
        int target = GRID_SIZE - 1;  // Start from the rightmost position

        for (int col = GRID_SIZE - 1; col >= 0; col--) {
            if (board[row][col] != 0) {
                // Move tile to the target position
                if (target != col) {
                    board[row][target] = board[row][col];
                    board[row][col] = 0;
                    moved = 1;
                }

                // Merge with the previous tile if they are the same
                if ((target < GRID_SIZE - 1) && (board[row][target] == board[row][target + 1])) {
                    board[row][target + 1] *= 2;
                    board[row][target] = 0;
                    target++;  // Move forward to handle the merged tile correctly
                    moved = 1;
                }

                target--;
            }
        }
    }
}

void shift_and_merge_left() {
    for (int col = 0; col < GRID_SIZE; col++) {
        int target = 0;  // The position to place the next non-zero element in the column

        for (int row = 0; row < GRID_SIZE; row++) {
            if (board[row][col] != 0) {
                // Move tile to the target position
                if (target != row) {
                    board[target][col] = board[row][col];
                    board[row][col] = 0;
                    moved = 1;
                }

                // Merge with the tile above if they are the same
                if (target > 0 && board[target][col] == board[target - 1][col]) {
                    board[target - 1][col] *= 2;
                    board[target][col] = 0;
                    target--;  // Move back to handle merging correctly
                    moved = 1;
                }

                target++;
            }
        }
    }
}


void shift_and_merge_right() {
    for (int col = 0; col < GRID_SIZE; col++) {
        int target = GRID_SIZE - 1;  // Start from the bottom position in each column

        for (int row = GRID_SIZE - 1; row >= 0; row--) {
            if (board[row][col] != 0) {
                // Move tile to the target position
                if (target != row) {
                    board[target][col] = board[row][col];
                    board[row][col] = 0;
                    moved = 1;
                }

                // Merge with the tile below if they are the same
                if (target < GRID_SIZE - 1 && board[target][col] == board[target + 1][col]) {
                    board[target + 1][col] *= 2;
                    board[target][col] = 0;
                    target++;  // Move forward to handle merging correctly
                    moved = 1;
                }

                target--;
            }
        }
    }
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

