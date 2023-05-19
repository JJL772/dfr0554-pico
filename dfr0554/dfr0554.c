
#include "pico/stdlib.h"
#include "pico/error.h"

#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "hardware/i2c.h"

#include "dfr0554.h"

/*********************************************************
 * Protocol definitions
 ********************************************************/

#define LCD_LCD_ADDR (0x7c>>1)
#define LCD_RGB_ADDR (0xc0>>1)

/* LCD function set */
#define LCD_FS_FONT_5X10 0b100 /* large 5x10 font*/
#define LCD_FS_FONT_5X8  0b000 /* small 5x8 font */
#define LCD_FS_LINES_1 0b0000 /* 1 line */
#define LCD_FS_LINES_2 0b1000 /* 2 lines */
#define LCD_CMD_FUNCTION_SET(font, lines) (0b00110000 | (font) | (lines))

/* Display ON/OFF */
#define LCD_ENABLE_DISP_ON    1
#define LCD_ENABLE_DISP_OFF   0
#define LCD_ENABLE_CURSOR_ON  1
#define LCD_ENABLE_CURSOR_OFF 0
#define LCD_ENABLE_BLINK_ON   1
#define LCD_ENABLE_BLINK_OFF  0
#define LCD_CMD_ENABLE(display, cursor, blink) (0b00001000 | (display<<2) | (cursor<<1) | (blink))

/* Clear display */
#define LCD_CMD_CLEAR (0b00000001)

/* Return dram addr to home */
#define LCD_CMD_RETURN_HOME (0b00000010)

/* Entry mode set */
#define LCD_CMD_ENTRYMODE(left, inc) (0b00000100 | (left<<1) | (inc))

/* Display shift */
#define LCD_CMD_DISP_SHIFT(cursor, left) (0b00001 | (cursor<<3) | (left<<2))

/**
 * RGB controller regs
 * From: https://www.nxp.com/docs/en/data-sheet/PCA9633.pdf
 */
#define LCD_RGB_REG_RED    0x4
#define LCD_RGB_REG_GREEN  0x3
#define LCD_RGB_REG_BLUE   0x2

#define LCD_RGB_REG_MODE1  0x0
#define LCD_RGB_REG_MODE2  0x1
#define LCD_RGB_REG_OUTPUT 0x8
#define LCD_RGB_REG_GRPPWM 0x6
#define LCD_RGB_REG_GRPFREQ 0x7

/*********************************************************
 * Forward decls
 ********************************************************/

static bool _dfr0554_init_device(struct dfr0554* inst);
static bool _dfr0554_simple_lcd_cmd(struct dfr0554* inst, uint8_t cmd);
static bool _dfr0554_set_reg(struct dfr0554* inst, uint8_t addr, uint8_t data);
static bool _dfr0554_write_char(struct dfr0554* inst, uint8_t value);

#define ASSERT_WASINIT 

/*********************************************************
 * Public API implementation
 ********************************************************/

int dfr0554_init(struct i2c_inst* i2c, dfr0554_t* inst) {
	memset(inst, 0, sizeof(*inst));
	inst->inst = i2c;
	inst->wasinit = 1;
	return _dfr0554_init_device(inst);
}

void dfr0554_shutdown(dfr0554_t* inst) {
	ASSERT_WASINIT;
	i2c_deinit(inst->inst);
	inst->wasinit = 0;
}

void dfr0554_enable_display(dfr0554_t* inst, int on)
{
	ASSERT_WASINIT;
	inst->en_disp = !!on;
	_dfr0554_simple_lcd_cmd(inst, LCD_CMD_ENABLE(inst->en_disp, inst->en_cur, inst->en_blink));
}

void dfr0554_clear(dfr0554_t* inst)
{
	ASSERT_WASINIT;
	_dfr0554_simple_lcd_cmd(inst, LCD_CMD_CLEAR);
}

void dfr0554_enable_blink(dfr0554_t* inst, int enable)
{
	ASSERT_WASINIT;
	inst->en_blink = !!enable;
	_dfr0554_simple_lcd_cmd(inst, LCD_CMD_ENABLE(inst->en_disp, inst->en_cur, inst->en_blink));
}

void dfr0554_enable_cursor(dfr0554_t* inst, int enable)
{
	ASSERT_WASINIT;
	inst->en_cur = !!enable;
	_dfr0554_simple_lcd_cmd(inst, LCD_CMD_ENABLE(inst->en_disp, inst->en_cur, inst->en_blink));
}

void dfr0554_set_rgb(dfr0554_t* inst, uint8_t red, uint8_t green, uint8_t blue)
{
	ASSERT_WASINIT;
	_dfr0554_set_reg(inst, LCD_RGB_REG_RED, red);
	_dfr0554_set_reg(inst, LCD_RGB_REG_GREEN, green);
	_dfr0554_set_reg(inst, LCD_RGB_REG_BLUE, blue);
}

void dfr0554_set_cursor_pos(dfr0554_t* inst, uint8_t col, uint8_t row)
{
	ASSERT_WASINIT;
	// @TODO: support the larger font mode!
	uint8_t addr = (row * 64) + col;
	_dfr0554_simple_lcd_cmd(inst, 0x80 | addr);
}

void dfr0554_scroll(dfr0554_t* inst, int direction)
{
	ASSERT_WASINIT;
	uint8_t dir = (direction == DFR0554_RIGHT) ? 0b100 : 0;
	_dfr0554_simple_lcd_cmd(inst, 0b00011000 | dir);
}

void dfr0554_print(dfr0554_t* inst, const char *str)
{
	ASSERT_WASINIT;
	for(; *str; ++str)
	{
		if (!_dfr0554_write_char(inst, *str))
			break;
	}
}

void dfr0554_printf(dfr0554_t* inst, const char* fmt, ...)
{
	va_list vl;
	va_start(vl, fmt);
	char buf[256];
	vsnprintf(buf, sizeof(buf), fmt, vl);
	va_end(vl);
	dfr0554_print(inst, buf);
}

/*********************************************************
 * Private API implementation
 ********************************************************/

/** Write LCD command */
static bool _dfr0554_lcd_cmd(struct dfr0554* inst, uint8_t* commandBytes, size_t numBytes, int rs)
{
	uint8_t bytes[numBytes+1];
	memset(bytes, 0, sizeof(bytes));
	
	bytes[0] = rs ? 0b01000000 : 0x0;
	for (int i = 1; i < numBytes+1; ++i)
		bytes[i] = commandBytes[i-1];

	return i2c_write_blocking(inst->inst, LCD_LCD_ADDR, bytes, sizeof(bytes), false) > 0;
}

static bool _dfr0554_simple_lcd_cmd(struct dfr0554* inst, uint8_t cmd)
{
	return _dfr0554_lcd_cmd(inst, &cmd, 1, false);
}

/** Set single register in RGB controller */
static bool _dfr0554_set_reg(struct dfr0554* inst, uint8_t addr, uint8_t data)
{
	uint8_t arr[2] = {addr, data};
	return i2c_write_blocking(inst->inst, LCD_RGB_ADDR, arr, sizeof(arr), false) != 0;
}

/** Write single value to LCD mem at current position */
static bool _dfr0554_write_char(struct dfr0554* inst, uint8_t value)
{
	return _dfr0554_lcd_cmd(inst, &value, 1, true);
}

static bool _dfr0554_init_device(struct dfr0554* inst)
{
	/* Datasheet calls for ~50ms where voltage will stabilize */
	sleep_ms(50);

	/* Docs say we need to call this in a funky loop */
	for (int i = 0; i < 3; ++i)
	{
		_dfr0554_simple_lcd_cmd(inst, LCD_CMD_FUNCTION_SET(LCD_FS_FONT_5X8, LCD_FS_LINES_2));
		sleep_ms(5);
	}

	dfr0554_set_rgb(inst, 255, 255, 255);

	_dfr0554_set_reg(inst, LCD_RGB_REG_MODE1, 0x0);
	_dfr0554_set_reg(inst, LCD_RGB_REG_MODE2, 0x20);  /* Blinking mode */
	_dfr0554_set_reg(inst, LCD_RGB_REG_OUTPUT, 0xFF); /* Enable all PWMs */

	/* Configure PWM duty cycle to full, highest possible freq */
	_dfr0554_set_reg(inst, LCD_RGB_REG_GRPFREQ, 0x0);
	_dfr0554_set_reg(inst, LCD_RGB_REG_GRPPWM, 0xFF);

	_dfr0554_simple_lcd_cmd(inst, LCD_CMD_ENTRYMODE(1, 0));
	_dfr0554_simple_lcd_cmd(inst, LCD_CMD_ENABLE(1, 0, 0));
	dfr0554_clear(inst);
	
	sleep_ms(500);
	
	return true;
}

