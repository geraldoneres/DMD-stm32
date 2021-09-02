#ifndef DMD_H_
#define DMD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

//DMD I/O pin macros
#define LIGHT_DMD_ROW_01_05_09_13()       { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2,  GPIO_PIN_RESET); HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,  GPIO_PIN_RESET);}
#define LIGHT_DMD_ROW_02_06_10_14()       { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2,  GPIO_PIN_RESET); HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,  GPIO_PIN_SET);}
#define LIGHT_DMD_ROW_03_07_11_15()       { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2,  GPIO_PIN_SET); HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,  GPIO_PIN_RESET); }
#define LIGHT_DMD_ROW_04_08_12_16()       { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2,  GPIO_PIN_SET); HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0,  GPIO_PIN_SET); }
#define LATCH_DMD_SHIFT_REG_TO_OUTPUT()   { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3,  GPIO_PIN_SET); HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3,  GPIO_PIN_RESET); }
#define OE_DMD_ROWS_OFF()                 { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_RESET);}
#define OE_DMD_ROWS_ON()                  { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3,  GPIO_PIN_SET); }

//Pixel/graphics writing modes (bGraphicsMode)
#define GRAPHICS_NORMAL    0
#define GRAPHICS_INVERSE   1
#define GRAPHICS_TOGGLE    2
#define GRAPHICS_OR        3
#define GRAPHICS_NOR       4

//drawTestPattern Patterns
#define PATTERN_ALT_0     0
#define PATTERN_ALT_1     1
#define PATTERN_STRIPE_0  2
#define PATTERN_STRIPE_1  3

#define pgm_read_byte(addr) (*(const uint8_t *)(addr))

//display screen (and subscreen) sizing
#define DMD_PIXELS_ACROSS         32      //pixels across x axis (base 2 size expected)
#define DMD_PIXELS_DOWN           16      //pixels down y axis
#define DMD_BITSPERPIXEL           1      //1 bit per pixel, use more bits to allow for pwm screen brightness control
#define DMD_RAM_SIZE_BYTES        ((DMD_PIXELS_ACROSS * (DMD_BITSPERPIXEL / 8)) * DMD_PIXELS_DOWN) //(32x * 1 / 8) = 4 bytes, * 16y = 64 bytes per screen here.

// Font Indices
#define FONT_LENGTH             0
#define FONT_FIXED_WIDTH        2
#define FONT_HEIGHT             3
#define FONT_FIRST_CHAR         4
#define FONT_CHAR_COUNT         5
#define FONT_WIDTH_TABLE        6

typedef uint8_t (*FontCallback)(const uint8_t*);

uint8_t *bDMDScreenRAM;
char marqueeText[256];
uint8_t marqueeLength;
int marqueeWidth;
int marqueeHeight;
int marqueeOffsetX;
int marqueeOffsetY;
//Pointer to current font
const uint8_t *Font;
//Display information
uint8_t DisplaysWide;
uint8_t DisplaysHigh;
uint8_t DisplaysTotal;
int row1, row2, row3;
//scanning pointer into bDMDScreenRAM, setup init @ 48 for the first valid scan
uint8_t bDMDByte;

void DMD_init(uint8_t panelsWide, uint8_t panelsHigh);
void writePixel(unsigned int bX, unsigned int bY, uint8_t bGraphicsMode,
		uint8_t bPixel); // Set or clear a pixel at the x and y location (0,0 is the top left corner)
void drawString(int bX, int bY, const char *bChars, uint8_t length,
		uint8_t bGraphicsMode);
void selectFont(const uint8_t *font);
int drawChar(const int bX, const int bY, const unsigned char letter,
		uint8_t bGraphicsMode);
int charWidth(const unsigned char letter);
void drawMarquee(const char *bChars, uint8_t length, int left, int top);
bool stepMarquee(int amountX, int amountY);
void clearScreen(uint8_t bNormal);
void drawLine(int x1, int y1, int x2, int y2, uint8_t bGraphicsMode);
void drawCircle(int xCenter, int yCenter, int radius, uint8_t bGraphicsMode);
void drawBox(int x1, int y1, int x2, int y2, uint8_t bGraphicsMode);
void drawFilledBox(int x1, int y1, int x2, int y2, uint8_t bGraphicsMode);
void drawTestPattern(uint8_t bPattern);
void scanDisplayBySPI();
void drawCircleSub(int cx, int cy, int x, int y, uint8_t bGraphicsMode);

#ifdef __cplusplus
}
#endif

#endif /* DMD_H_ */
