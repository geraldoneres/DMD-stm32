#include "../inc/dmd.h"
#include "../../../Inc/main.h"
#include "../../../Inc/spi.h"
#include "../../../Inc/gpio.h"
#include "../../../Inc/tim.h"

static uint8_t bPixelLookupTable[8] = { 0x80,   //0, bit 7
		0x40,   //1, bit 6
		0x20,   //2. bit 5
		0x10,   //3, bit 4
		0x08,   //4, bit 3
		0x04,   //5, bit 2
		0x02,   //6, bit 1
		0x01    //7, bit 0
		};

void DMD_init(uint8_t panelsWide, uint8_t panelsHigh) {
	// uint16_t ui;
	printf("Inicializando display p10\n");
	DisplaysWide = panelsWide;
	DisplaysHigh = panelsHigh;
	DisplaysTotal = DisplaysWide * DisplaysHigh;
	row1 = DisplaysTotal << 4;
	row2 = DisplaysTotal << 5;
	row3 = ((DisplaysTotal << 2) * 3) << 2;
	bDMDScreenRAM = (unsigned char*) malloc(DisplaysTotal * DMD_RAM_SIZE_BYTES);

	clearScreen(true);

	// init the scan line/ram pointer to the required start point
	bDMDByte = 0;

	printf("Inicializando o timer\n");
	HAL_TIM_Base_Start_IT(&htim7);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {

	if (htim == &htim7) {
		scanDisplayBySPI();
	}
}

/*--------------------------------------------------------------------------------------
 Set or clear a pixel at the x and y location (0,0 is the top left corner)
 --------------------------------------------------------------------------------------*/
void writePixel(unsigned int bX, unsigned int bY, uint8_t bGraphicsMode,
		uint8_t bPixel) {
	unsigned int uiDMDRAMPointer;

	if (bX >= (DMD_PIXELS_ACROSS * DisplaysWide)
			|| bY >= (DMD_PIXELS_DOWN * DisplaysHigh)) {
		return;
	}
	uint8_t panel = (bX / DMD_PIXELS_ACROSS)
			+ (DisplaysWide * (bY / DMD_PIXELS_DOWN));
	bX = (bX % DMD_PIXELS_ACROSS) + (panel << 5);
	bY = bY % DMD_PIXELS_DOWN;
	//set pointer to DMD RAM byte to be modified
	uiDMDRAMPointer = bX / 8 + bY * (DisplaysTotal << 2);

	uint8_t lookup = bPixelLookupTable[bX & 0x07];

	switch (bGraphicsMode) {
	case GRAPHICS_NORMAL:
		if (bPixel == true)
			bDMDScreenRAM[uiDMDRAMPointer] &= ~lookup;	// zero bit is pixel on
		else
			bDMDScreenRAM[uiDMDRAMPointer] |= lookup;	// one bit is pixel off
		break;
	case GRAPHICS_INVERSE:
		if (bPixel == false)
			bDMDScreenRAM[uiDMDRAMPointer] &= ~lookup;	// zero bit is pixel on
		else
			bDMDScreenRAM[uiDMDRAMPointer] |= lookup;	// one bit is pixel off
		break;
	case GRAPHICS_TOGGLE:
		if (bPixel == true) {
			if ((bDMDScreenRAM[uiDMDRAMPointer] & lookup) == 0)
				bDMDScreenRAM[uiDMDRAMPointer] |= lookup;// one bit is pixel off
			else
				bDMDScreenRAM[uiDMDRAMPointer] &= ~lookup;// one bit is pixel off
		}
		break;
	case GRAPHICS_OR:
		//only set pixels on
		if (bPixel == true)
			bDMDScreenRAM[uiDMDRAMPointer] &= ~lookup;	// zero bit is pixel on
		break;
	case GRAPHICS_NOR:
		//only clear on pixels
		if ((bPixel == true)
				&& ((bDMDScreenRAM[uiDMDRAMPointer] & lookup) == 0))
			bDMDScreenRAM[uiDMDRAMPointer] |= lookup;	// one bit is pixel off
		break;
	}

}

void drawString(int bX, int bY, const char *bChars, uint8_t length,
		uint8_t bGraphicsMode) {
	if (bX >= (DMD_PIXELS_ACROSS * DisplaysWide)
			|| bY >= DMD_PIXELS_DOWN * DisplaysHigh)
		return;
	uint8_t height = pgm_read_byte(Font + FONT_HEIGHT);
	if ((bY + height) < 0)
		return;

	int strWidth = 0;
	drawLine(bX - 1, bY, bX - 1, bY + height, GRAPHICS_INVERSE);

	for (int i = 0; i < length; i++) {
		int charWide = drawChar(bX + strWidth, bY, bChars[i], bGraphicsMode);
		if (charWide > 0) {
			strWidth += charWide;
			drawLine(bX + strWidth, bY, bX + strWidth, bY + height,
			GRAPHICS_INVERSE);
			strWidth++;
		} else if (charWide < 0) {
			return;
		}
		if ((bX + strWidth) >= DMD_PIXELS_ACROSS * DisplaysWide
				|| bY >= DMD_PIXELS_DOWN * DisplaysHigh)
			return;
	}
}

void drawMarquee(const char *bChars, uint8_t length, int left, int top) {
	marqueeWidth = 0;
	for (int i = 0; i < length; i++) {
		marqueeText[i] = bChars[i];
		marqueeWidth += charWidth(bChars[i]) + 1;
	}
	marqueeHeight = pgm_read_byte(Font + FONT_HEIGHT);
	marqueeText[length] = '\0';
	marqueeOffsetY = top;
	marqueeOffsetX = left;
	marqueeLength = length;
	drawString(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength,
	GRAPHICS_NORMAL);
}

bool stepMarquee(int amountX, int amountY) {
	bool ret = false;
	marqueeOffsetX += amountX;
	marqueeOffsetY += amountY;
	if (marqueeOffsetX < -marqueeWidth) {
		marqueeOffsetX = DMD_PIXELS_ACROSS * DisplaysWide;
		clearScreen(true);
		ret = true;
	} else if (marqueeOffsetX > DMD_PIXELS_ACROSS * DisplaysWide) {
		marqueeOffsetX = -marqueeWidth;
		clearScreen(true);
		ret = true;
	}

	if (marqueeOffsetY < -marqueeHeight) {
		marqueeOffsetY = DMD_PIXELS_DOWN * DisplaysHigh;
		clearScreen(true);
		ret = true;
	} else if (marqueeOffsetY > DMD_PIXELS_DOWN * DisplaysHigh) {
		marqueeOffsetY = -marqueeHeight;
		clearScreen(true);
		ret = true;
	}

	// Special case horizontal scrolling to improve speed
	if (amountY == 0 && amountX == -1) {
		// Shift entire screen one bit
		for (int i = 0; i < DMD_RAM_SIZE_BYTES * DisplaysTotal; i++) {
			if ((i % (DisplaysWide * 4)) == (DisplaysWide * 4) - 1) {
				bDMDScreenRAM[i] = (bDMDScreenRAM[i] << 1) + 1;
			} else {
				bDMDScreenRAM[i] = (bDMDScreenRAM[i] << 1)
						+ ((bDMDScreenRAM[i + 1] & 0x80) >> 7);
			}
		}

		// Redraw last char on screen
		int strWidth = marqueeOffsetX;
		for (int i = 0; i < marqueeLength; i++) {
			int wide = charWidth(marqueeText[i]);
			if (strWidth + wide >= DisplaysWide * DMD_PIXELS_ACROSS) {
				drawChar(strWidth, marqueeOffsetY, marqueeText[i],
				GRAPHICS_NORMAL);
				return ret;
			}
			strWidth += wide + 1;
		}
	} else if (amountY == 0 && amountX == 1) {
		// Shift entire screen one bit
		for (int i = (DMD_RAM_SIZE_BYTES * DisplaysTotal) - 1; i >= 0; i--) {
			if ((i % (DisplaysWide * 4)) == 0) {
				bDMDScreenRAM[i] = (bDMDScreenRAM[i] >> 1) + 128;
			} else {
				bDMDScreenRAM[i] = (bDMDScreenRAM[i] >> 1)
						+ ((bDMDScreenRAM[i - 1] & 1) << 7);
			}
		}

		// Redraw last char on screen
		int strWidth = marqueeOffsetX;
		for (int i = 0; i < marqueeLength; i++) {
			int wide = charWidth(marqueeText[i]);
			if (strWidth + wide >= 0) {
				drawChar(strWidth, marqueeOffsetY, marqueeText[i],
				GRAPHICS_NORMAL);
				return ret;
			}
			strWidth += wide + 1;
		}
	} else {
		drawString(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength,
		GRAPHICS_NORMAL);
	}

	return ret;
}

/*--------------------------------------------------------------------------------------
 Clear the screen in DMD RAM
 --------------------------------------------------------------------------------------*/
void clearScreen(uint8_t bNormal) {
	if (bNormal) // clear all pixels
		memset(bDMDScreenRAM, 0xff, (DMD_RAM_SIZE_BYTES * DisplaysTotal));
	else
		// set all pixels
		memset(bDMDScreenRAM, 0x00, (DMD_RAM_SIZE_BYTES * DisplaysTotal));
}

/*--------------------------------------------------------------------------------------
 Draw or clear a line from x1,y1 to x2,y2
 --------------------------------------------------------------------------------------*/
void drawLine(int x1, int y1, int x2, int y2, uint8_t bGraphicsMode) {
	int dy = y2 - y1;
	int dx = x2 - x1;
	int stepx, stepy;

	if (dy < 0) {
		dy = -dy;
		stepy = -1;
	} else {
		stepy = 1;
	}
	if (dx < 0) {
		dx = -dx;
		stepx = -1;
	} else {
		stepx = 1;
	}
	dy <<= 1;			// dy is now 2*dy
	dx <<= 1;			// dx is now 2*dx

	writePixel(x1, y1, bGraphicsMode, true);
	if (dx > dy) {
		int fraction = dy - (dx >> 1);	// same as 2*dy - dx
		while (x1 != x2) {
			if (fraction >= 0) {
				y1 += stepy;
				fraction -= dx;	// same as fraction -= 2*dx
			}
			x1 += stepx;
			fraction += dy;	// same as fraction -= 2*dy
			writePixel(x1, y1, bGraphicsMode, true);
		}
	} else {
		int fraction = dx - (dy >> 1);
		while (y1 != y2) {
			if (fraction >= 0) {
				x1 += stepx;
				fraction -= dy;
			}
			y1 += stepy;
			fraction += dx;
			writePixel(x1, y1, bGraphicsMode, true);
		}
	}
}

/*--------------------------------------------------------------------------------------
 Draw or clear a circle of radius r at x,y centre
 --------------------------------------------------------------------------------------*/
void drawCircle(int xCenter, int yCenter, int radius, uint8_t bGraphicsMode) {
	int x = 0;
	int y = radius;
	int p = (5 - radius * 4) / 4;

	drawCircleSub(xCenter, yCenter, x, y, bGraphicsMode);
	while (x < y) {
		x++;
		if (p < 0) {
			p += 2 * x + 1;
		} else {
			y--;
			p += 2 * (x - y) + 1;
		}
		drawCircleSub(xCenter, yCenter, x, y, bGraphicsMode);
	}
}

void drawCircleSub(int cx, int cy, int x, int y, uint8_t bGraphicsMode) {

	if (x == 0) {
		writePixel(cx, cy + y, bGraphicsMode, true);
		writePixel(cx, cy - y, bGraphicsMode, true);
		writePixel(cx + y, cy, bGraphicsMode, true);
		writePixel(cx - y, cy, bGraphicsMode, true);
	} else if (x == y) {
		writePixel(cx + x, cy + y, bGraphicsMode, true);
		writePixel(cx - x, cy + y, bGraphicsMode, true);
		writePixel(cx + x, cy - y, bGraphicsMode, true);
		writePixel(cx - x, cy - y, bGraphicsMode, true);
	} else if (x < y) {
		writePixel(cx + x, cy + y, bGraphicsMode, true);
		writePixel(cx - x, cy + y, bGraphicsMode, true);
		writePixel(cx + x, cy - y, bGraphicsMode, true);
		writePixel(cx - x, cy - y, bGraphicsMode, true);
		writePixel(cx + y, cy + x, bGraphicsMode, true);
		writePixel(cx - y, cy + x, bGraphicsMode, true);
		writePixel(cx + y, cy - x, bGraphicsMode, true);
		writePixel(cx - y, cy - x, bGraphicsMode, true);
	}
}

/*--------------------------------------------------------------------------------------
 Draw or clear a box(rectangle) with a single pixel border
 --------------------------------------------------------------------------------------*/
void drawBox(int x1, int y1, int x2, int y2, uint8_t bGraphicsMode) {
	drawLine(x1, y1, x2, y1, bGraphicsMode);
	drawLine(x2, y1, x2, y2, bGraphicsMode);
	drawLine(x2, y2, x1, y2, bGraphicsMode);
	drawLine(x1, y2, x1, y1, bGraphicsMode);
}

/*--------------------------------------------------------------------------------------
 Draw or clear a filled box(rectangle) with a single pixel border
 --------------------------------------------------------------------------------------*/
void drawFilledBox(int x1, int y1, int x2, int y2, uint8_t bGraphicsMode) {
	for (int b = x1; b <= x2; b++) {
		drawLine(b, y1, b, y2, bGraphicsMode);
	}
}

/*--------------------------------------------------------------------------------------
 Draw the selected test pattern
 --------------------------------------------------------------------------------------*/
void drawTestPattern(uint8_t bPattern) {
	int ui;

	int numPixels = DisplaysTotal * DMD_PIXELS_ACROSS * DMD_PIXELS_DOWN;
	int pixelsWide = DMD_PIXELS_ACROSS * DisplaysWide;
	for (ui = 0; ui < numPixels; ui++) {
		switch (bPattern) {
		case PATTERN_ALT_0:	// every alternate pixel, first pixel on
			if ((ui & pixelsWide) == 0)
				//even row
				writePixel((ui & (pixelsWide - 1)),
						((ui & ~(pixelsWide - 1)) / pixelsWide),
						GRAPHICS_NORMAL, ui & 1);
			else
				//odd row
				writePixel((ui & (pixelsWide - 1)),
						((ui & ~(pixelsWide - 1)) / pixelsWide),
						GRAPHICS_NORMAL, !(ui & 1));
			break;
		case PATTERN_ALT_1:	// every alternate pixel, first pixel off
			if ((ui & pixelsWide) == 0)
				//even row
				writePixel((ui & (pixelsWide - 1)),
						((ui & ~(pixelsWide - 1)) / pixelsWide),
						GRAPHICS_NORMAL, !(ui & 1));
			else
				//odd row
				writePixel((ui & (pixelsWide - 1)),
						((ui & ~(pixelsWide - 1)) / pixelsWide),
						GRAPHICS_NORMAL, ui & 1);
			break;
		case PATTERN_STRIPE_0:	// vertical stripes, first stripe on
			writePixel((ui & (pixelsWide - 1)),
					((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL,
					ui & 1);
			break;
		case PATTERN_STRIPE_1:	// vertical stripes, first stripe off
			writePixel((ui & (pixelsWide - 1)),
					((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL,
					!(ui & 1));
			break;
		}
	}
}

/*--------------------------------------------------------------------------------------
 Scan the dot matrix LED panel display, from the RAM mirror out to the display hardware.
 Call 4 times to scan the whole display which is made up of 4 interleaved rows within the 16 total rows.
 Insert the calls to this function into the main loop for the highest call rate, or from a timer interrupt
 --------------------------------------------------------------------------------------*/
void scanDisplayBySPI() {
	//if PIN_OTHER_SPI_nCS is in use during a DMD scan request then scanDisplayBySPI() will exit without conflict! (and skip that scan)
	if (0)	// digitalRead( PIN_OTHER_SPI_nCS ) == HIGH
	{
		//SPI transfer pixels to the display hardware shift registers
		int rowsize = DisplaysTotal << 2;
		int offset = rowsize * bDMDByte;
		for (int i = 0; i < rowsize; i++) {

			HAL_SPI_Transmit(&hspi1,
					(uint8_t*) &bDMDScreenRAM[offset + i + row3], 2, 5);
			HAL_SPI_Transmit(&hspi1,
					(uint8_t*) &bDMDScreenRAM[offset + i + row2], 2, 5);
			HAL_SPI_Transmit(&hspi1,
					(uint8_t*) &bDMDScreenRAM[offset + i + row1], 2, 5);
			HAL_SPI_Transmit(&hspi1, (uint8_t*) &bDMDScreenRAM[offset + i], 2,
					5);
		}

		OE_DMD_ROWS_OFF();
		LATCH_DMD_SHIFT_REG_TO_OUTPUT();
		switch (bDMDByte) {
		case 0:			// row 1, 5, 9, 13 were clocked out
			LIGHT_DMD_ROW_01_05_09_13()
			;
			bDMDByte = 1;
			break;
		case 1:			// row 2, 6, 10, 14 were clocked out
			LIGHT_DMD_ROW_02_06_10_14()
			;
			bDMDByte = 2;
			break;
		case 2:			// row 3, 7, 11, 15 were clocked out
			LIGHT_DMD_ROW_03_07_11_15()
			;
			bDMDByte = 3;
			break;
		case 3:			// row 4, 8, 12, 16 were clocked out
			LIGHT_DMD_ROW_04_08_12_16()
			;
			bDMDByte = 0;
			break;
		}
		OE_DMD_ROWS_ON();
	}
}

void selectFont(const uint8_t *font) {
	Font = font;
}

int drawChar(const int bX, const int bY, const unsigned char letter,
		uint8_t bGraphicsMode) {
	if (bX > (DMD_PIXELS_ACROSS * DisplaysWide)
			|| bY > (DMD_PIXELS_DOWN * DisplaysHigh))
		return -1;
	unsigned char c = letter;
	uint8_t height = pgm_read_byte(Font + FONT_HEIGHT);
	if (c == ' ') {
		int charWide = charWidth(' ');
		drawFilledBox(bX, bY, bX + charWide, bY + height, GRAPHICS_INVERSE);
		return charWide;
	}
	uint8_t width = 0;
	uint8_t bytes = (height + 7) / 8;

	uint8_t firstChar = pgm_read_byte(Font + FONT_FIRST_CHAR);
	uint8_t charCount = pgm_read_byte(Font + FONT_CHAR_COUNT);

	uint16_t index = 0;

	if (c < firstChar || c >= (firstChar + charCount))
		return 0;
	c -= firstChar;

	if (pgm_read_byte(Font + FONT_LENGTH) == 0
			&& pgm_read_byte(Font + FONT_LENGTH + 1) == 0) {
		// zero length is flag indicating fixed width font (array does not contain width data entries)
		width = pgm_read_byte(Font + FONT_FIXED_WIDTH);
		index = c * bytes * width + FONT_WIDTH_TABLE;
	} else {
		// variable width font, read width data, to get the index
		for (int i = 0; i < c; i++) {
			index += pgm_read_byte(Font + FONT_WIDTH_TABLE + i);
		}
		index = index * bytes + charCount + FONT_WIDTH_TABLE;
		width = pgm_read_byte(Font + FONT_WIDTH_TABLE + c);
	}
	if (bX < -width || bY < -height)
		return width;

	// last but not least, draw the character
	for (int j = 0; j < width; j++) { // Width
		for (int i = bytes - 1; i < 254; i--) { // Vertical Bytes
			uint8_t data = pgm_read_byte(Font + index + j + (i * width));
			int offset = (i * 8);
			if ((i == bytes - 1) && bytes > 1) {
				offset = height - 8;
			}
			for (int k = 0; k < 8; k++) { // Vertical bits
				if ((offset + k >= i * 8) && (offset + k <= height)) {
					if (data & (1 << k)) {
						writePixel(bX + j, bY + offset + k, bGraphicsMode,
						true);
					} else {
						writePixel(bX + j, bY + offset + k, bGraphicsMode,
						false);
					}
				}
			}
		}
	}
	return width;
}

int charWidth(const unsigned char letter) {
	unsigned char c = letter;
	// Space is often not included in font so use width of 'n'
	if (c == ' ')
		c = 'n';
	uint8_t width = 0;

	uint8_t firstChar = pgm_read_byte(Font + FONT_FIRST_CHAR);
	uint8_t charCount = pgm_read_byte(Font + FONT_CHAR_COUNT);

	//   uint16_t index = 0;

	if (c < firstChar || c >= (firstChar + charCount)) {
		return 0;
	}
	c -= firstChar;

	if (pgm_read_byte(Font + FONT_LENGTH) == 0
			&& pgm_read_byte(Font + FONT_LENGTH + 1) == 0) {
		// zero length is flag indicating fixed width font (array does not contain width data entries)
		width = pgm_read_byte(Font + FONT_FIXED_WIDTH);
	} else {
		// variable width font, read width data
		width = pgm_read_byte(Font + FONT_WIDTH_TABLE + c);
	}
	return width;
}

