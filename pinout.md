# Fixed bench pinout

The same external devices are used in all framework and baseline builds:

- SHT31-D I2C address: `0x44`
- SSD1306 I2C address: `0x3c`
- I2C speed: `100 kHz`
- UART: `115200 8N1`
- Button: external pull-up `10 kohm` to `3.3 V`, button to GND, active low
- LED: external series resistor `1 kohm`, active high

| Board | I2C | UART | Button | LED |
| --- | --- | --- | --- | --- |
| ESP32 DevKit V1 | I2C0 SDA `GPIO21`, SCL `GPIO22` | UART0 TX `GPIO1`, RX `GPIO3` | `GPIO18` | `GPIO19` |
| Black Pill STM32F411CEU6 | I2C1 SDA `PB7`, SCL `PB6` | USART2 TX `PA2`, RX `PA3` | `PB0` | `PB1` |
| DevEBox STM32F407VGT6 | I2C1 SDA `PB9`, SCL `PB8` | USART2 TX `PA2`, RX `PA3` | `PA0` | `PA1` |
| Pro Micro nRF52840 | I2C0 SDA `P0.08`, SCL `P0.11` | UART0 board/USB default | `P0.06` | `P0.17` |

The STM32Cube generated projects must configure the pins above before calling the framework BSP.
The nRF Connect project must provide a Zephyr board or overlay that maps `i2c0` and `uart0` to
the pins selected here.

The button and LED wiring schematic is available at `firmware/docs/esquema_botao_led.svg`.
