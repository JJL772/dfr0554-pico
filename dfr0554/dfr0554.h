/**
 * i2c driver for DFR0554 16x2 LCD display, for rapi pico w
 */
#pragma once

#include <stdint.h>

typedef struct dfr0554 {
	struct i2c_inst* inst;
	uint8_t en_disp;
	uint8_t en_cur;
	uint8_t en_blink;
	uint8_t wasinit;
} dfr0554_t;

int dfr0554_init(struct i2c_inst* i2c, dfr0554_t* inst);

void dfr0554_shutdown(dfr0554_t* inst);

/**
 * @brief Set display on or off
 * @param inst Instance
 * @param en Enable if 1
 */
void dfr0554_enable_display(dfr0554_t* inst, int en);

/**
 * @brief Enable/disable cursor blinking
 * @param inst Instance
 * @param en Enable blink if 1
 */
void dfr0554_enable_blink(dfr0554_t* inst, int en);

/**
 * @brief Enable cursor for LCD
 * @param inst Instance
 * @param en Enable cursor if 1
 */
void dfr0554_enable_cursor(dfr0554_t* inst, int en);

/**
 * @brief Clear the display
 * @param inst Instance
 */
void dfr0554_clear(dfr0554_t* inst);

/**
 * @brief Set backlight color of the display
 */
void dfr0554_set_rgb(dfr0554_t* inst, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Set the cursor pos
 * @param inst Instance
 * @param x x (or column)
 * @param y y (or row)
 */
void dfr0554_set_cursor_pos(dfr0554_t* inst, uint8_t x, uint8_t y);

/**
 * @brief For use with dfr0554_scroll
 */
enum {
	DFR0554_LEFT = 0,
	DFR0554_RIGHT = 1
};

/**
 * @brief Scroll the LCD text in a certain direction
 * @param inst Instance
 * @param direction RIGHT == 1, LEFT == 0
 */
void dfr0554_scroll(dfr0554_t* inst, int direction);

/**
 * @brief Print the string to the display
 * @param inst Instance
 * @param str String to print
 */
void dfr0554_print(dfr0554_t* inst, const char* str);

/**
 * @brief Formatted print to display
 * @param inst Instance
 * @param fmt Format
 * @param ... format args
 */
void dfr0554_printf(dfr0554_t* inst, const char* fmt, ...);
