
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"

#include "../dfr0554/dfr0554.h"

int main() {
	i2c_init(i2c0, 100000);
	gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
	gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
	gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);

    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
	static dfr0554_t d;
	dfr0554_init(i2c0, &d);
	
	dfr0554_clear(&d);
	dfr0554_print(&d, "123456789");
	dfr0554_set_rgb(&d, 0, 255, 0);
	
	while(1);
}
