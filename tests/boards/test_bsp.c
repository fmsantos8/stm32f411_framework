#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "bsp.h"

struct hal_gpio { unsigned int identifier; };
struct hal_i2c { unsigned int identifier; };
struct hal_uart { unsigned int identifier; };
struct hal_timer { unsigned int identifier; };

static hal_gpio_t led;
static hal_gpio_t button;
static hal_i2c_t bus;
static hal_uart_t uart;
static hal_timer_t timer;
static unsigned int calls;
static unsigned int failed_step;
static hal_gpio_config_t led_config;
static hal_gpio_config_t button_config;
static hal_i2c_config_t bus_config;
static hal_uart_config_t uart_config;
static hal_timer_config_t timer_config;

static void on_timer(void) {}

static hal_status_t next_result(void) {
    ++calls;
    return calls == failed_step ? HAL_STATUS_ERROR : HAL_STATUS_OK;
}

hal_status_t hal_gpio_config(hal_gpio_t **gpio, const hal_gpio_config_t *config) {
    assert(config != NULL && gpio != NULL);
    assert(calls < 2);
    if (calls == 0) {
        assert(config->direction == HAL_GPIO_OUTPUT);
        led_config = *config;
        *gpio = &led;
    } else {
        assert(config->direction == HAL_GPIO_INPUT);
        button_config = *config;
        *gpio = &button;
    }
    return next_result();
}

hal_status_t hal_i2c_config(hal_i2c_t **i2c, const hal_i2c_config_t *config) {
    assert(calls == 2 && config != NULL && i2c != NULL);
    bus_config = *config;
    *i2c = &bus;
    return next_result();
}

hal_status_t hal_uart_config(hal_uart_t **serial, const hal_uart_config_t *config) {
    assert(calls == 3 && config != NULL && serial != NULL);
    uart_config = *config;
    *serial = &uart;
    return next_result();
}

hal_status_t hal_timer_config(hal_timer_t **resource, const hal_timer_config_t *config) {
    assert(calls == 4 && config != NULL && resource != NULL);
    timer_config = *config;
    *resource = &timer;
    return next_result();
}

int main(int argc, char **argv) {
    assert(argc == 2);
    failed_step = (unsigned int)strtoul(argv[1], NULL, 10);
    assert(failed_step <= 5);

    bsp_resources_t resources;
    unsigned char original[sizeof(resources)];
    memset(&resources, 0xA5, sizeof(resources));
    memcpy(original, &resources, sizeof(original));

    assert(bsp_init(NULL, &resources) == HAL_STATUS_INVALID_ARGUMENT);
    assert(bsp_init(on_timer, NULL) == HAL_STATUS_INVALID_ARGUMENT);
    assert(calls == 0 && memcmp(original, &resources, sizeof(original)) == 0);

    hal_status_t status = bsp_init(on_timer, &resources);
    if (failed_step != 0) {
        assert(status == HAL_STATUS_ERROR && calls == failed_step);
        assert(memcmp(original, &resources, sizeof(original)) == 0);
    } else {
        assert(status == HAL_STATUS_OK && calls == 5);
        assert(resources.led == &led && resources.button == &button);
        assert(resources.devices_i2c == &bus && resources.telemetry_uart == &uart);
        assert(resources.timebase_timer == &timer);
        assert(led_config.pin.port == 1 && led_config.pin.number == 1);
        assert(button_config.pin.port == 1 && button_config.pin.number == 0);
        assert(button_config.pull == HAL_GPIO_PULL_NONE);
        assert(!resources.button_active_level && resources.led_active_level);
        assert(led_config.initial_level == !resources.led_active_level);
        assert(resources.sht31_address == 0x44 && resources.ssd1306_address == 0x3C);
        assert(bus_config.frequency == 100000 && bus_config.timeout == 100);
        assert(bus_config.instance == 0);
        assert(bus_config.clock_pin.port == 1 && bus_config.clock_pin.number == 6);
        assert(bus_config.data_pin.port == 1 && bus_config.data_pin.number == 7);
        assert(uart_config.baudrate == 115200 && uart_config.timeout == 1000);
        assert(uart_config.instance == 1);
        assert(uart_config.transmit_pin.port == 0 && uart_config.transmit_pin.number == 2);
        assert(uart_config.receive_pin.port == 0 && uart_config.receive_pin.number == 3);
        assert(uart_config.data_width == HAL_UART_DATA_WIDTH_8);
        assert(uart_config.parity == HAL_UART_PARITY_NONE && uart_config.stop_count == HAL_UART_STOP_COUNT_1);
        assert(timer_config.instance == 0 && timer_config.period == 1 && timer_config.callback == on_timer);
    }

    unsigned int previous_calls = calls;
    unsigned char previous_resources[sizeof(resources)];
    memcpy(previous_resources, &resources, sizeof(previous_resources));
    assert(bsp_init(on_timer, &resources) == HAL_STATUS_INVALID_STATE);
    assert(calls == previous_calls);
    assert(memcmp(previous_resources, &resources, sizeof(previous_resources)) == 0);
    return 0;
}
