#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "hal_stm32f4_private.h"
#include "system/hal_system.h"
#include "gpio/hal_gpio.h"
#include "uart/hal_uart.h"
#include "i2c/hal_i2c.h"
#include "timer/hal_timer.h"
#if defined(STM32F411xE)
#include "bsp.h"
#endif

uint32_t test_interrupt_mask;
static uint32_t core_clock = HSI_VALUE;
static uint32_t pclk1 = HSI_VALUE;
static uint32_t pclk2 = HSI_VALUE;
static uint32_t notifications;
static bool interrupts_enabled[128];
static bool fail_clock;
static bool fail_i2c_init;
static bool fail_timer_start;
static bool partial_uart;

void SystemCoreClockUpdate(void) {}
HAL_StatusTypeDef HAL_Init(void) { return HAL_OK; }
void HAL_IncTick(void) {}
uint32_t HAL_GetTick(void) { static uint32_t tick; return tick++; }
uint32_t HAL_RCC_GetPCLK1Freq(void) { return pclk1; }
uint32_t HAL_RCC_GetPCLK2Freq(void) { return pclk2; }
uint32_t HAL_RCC_GetHCLKFreq(void) { return core_clock; }

HAL_StatusTypeDef HAL_RCC_OscConfig(const RCC_OscInitTypeDef *config) {
    assert(config->OscillatorType == RCC_OSCILLATORTYPE_HSI);
    assert(config->HSIState == RCC_HSI_ON);
    if (config->PLL.PLLState == RCC_PLL_ON) {
        assert(config->PLL.PLLSource == RCC_PLLSOURCE_HSI);
        assert(config->PLL.PLLM == 16 && config->PLL.PLLP == RCC_PLLP_DIV2);
#if defined(STM32F411xE)
        assert(config->PLL.PLLN == 200 && config->PLL.PLLQ == 5);
#else
        assert(config->PLL.PLLN == 336 && config->PLL.PLLQ == 7);
#endif
        PWR->CSR |= PWR_CSR_VOSRDY;
    }
    return fail_clock ? HAL_ERROR : HAL_OK;
}

HAL_StatusTypeDef HAL_RCC_ClockConfig(const RCC_ClkInitTypeDef *config, uint32_t latency) {
    if (config->SYSCLKSource == RCC_SYSCLKSOURCE_PLLCLK) {
        assert((RCC->APB1ENR & RCC_APB1ENR_PWREN) != 0);
        assert((PWR->CR & PWR_CR_VOS) == PWR_REGULATOR_VOLTAGE_SCALE1);
        assert(config->AHBCLKDivider == RCC_SYSCLK_DIV1);
#if defined(STM32F411xE)
        assert(latency == FLASH_LATENCY_3);
        assert(config->APB1CLKDivider == RCC_HCLK_DIV2 && config->APB2CLKDivider == RCC_HCLK_DIV1);
        core_clock = 100000000;
        pclk1 = 50000000;
        pclk2 = 100000000;
        RCC->CFGR = RCC_CFGR_PPRE1_DIV2;
#else
        assert(latency == FLASH_LATENCY_5);
        assert(config->APB1CLKDivider == RCC_HCLK_DIV4 && config->APB2CLKDivider == RCC_HCLK_DIV2);
        core_clock = 168000000;
        pclk1 = 42000000;
        pclk2 = 84000000;
        RCC->CFGR = RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;
#endif
    }
    return HAL_OK;
}

void HAL_NVIC_SetPriority(IRQn_Type irq, uint32_t priority, uint32_t subpriority) {
    (void)irq;
    assert(priority == 5 && subpriority == 0);
}
void HAL_NVIC_EnableIRQ(IRQn_Type irq) { interrupts_enabled[irq] = true; }
void HAL_NVIC_ClearPendingIRQ(IRQn_Type irq) { (void)irq; }

HAL_StatusTypeDef __real_HAL_I2C_Init(I2C_HandleTypeDef *device);
HAL_StatusTypeDef __wrap_HAL_I2C_Init(I2C_HandleTypeDef *device) {
    assert((RCC->APB1ENR & (RCC_APB1ENR_I2C1EN | RCC_APB1ENR_I2C2EN | RCC_APB1ENR_I2C3EN)) != 0);
    return fail_i2c_init ? HAL_ERROR : __real_HAL_I2C_Init(device);
}
HAL_StatusTypeDef __real_HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *device);
HAL_StatusTypeDef __wrap_HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *device) {
    assert(test_interrupt_mask == 1);
    return fail_timer_start ? HAL_ERROR : __real_HAL_TIM_Base_Start_IT(device);
}
HAL_StatusTypeDef __real_HAL_UART_Transmit(UART_HandleTypeDef *, const uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef __wrap_HAL_UART_Transmit(UART_HandleTypeDef *device, const uint8_t *data, uint16_t size, uint32_t timeout) {
    if (partial_uart) {
        assert(size == 3 && timeout == 100);
        device->TxXferCount = 1;
        return HAL_TIMEOUT;
    }
    return __real_HAL_UART_Transmit(device, data, size, timeout);
}

void TIM2_IRQHandler(void);
void TIM3_IRQHandler(void);
void TIM4_IRQHandler(void);
void TIM5_IRQHandler(void);
static void on_timer(void) { ++notifications; }
static uint32_t used(void) {
    hal_memory_info_t info;
    assert(hal_memory_get_info(&info) == HAL_MEMORY_OK);
    return info.used;
}
static uint32_t alternate(GPIO_TypeDef *port, uint32_t pin) {
    return (port->AFR[pin / 8] >> (4 * (pin % 8))) & 15;
}

int main(int argc, char **argv) {
    assert(argc == 2);
    assert(mmap((void *)PERIPH_BASE, 0x40000, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == (void *)PERIPH_BASE);
    _Alignas(max_align_t) static unsigned char arena[8192];
    hal_system_config_t system = {.memory_buffer = arena, .memory_capacity = sizeof(arena)};
    hal_gpio_t *gpio = NULL;
    hal_gpio_config_t pin = {.pin = {2, 13}, .direction = HAL_GPIO_OUTPUT, .initial_level = true};
    hal_i2c_config_t bus = {.instance = 0, .clock_pin = {1, 6}, .data_pin = {1, 7}, .frequency = 100000, .timeout = 100};
    hal_i2c_t *i2c;
    hal_uart_config_t serial = {.instance = 0, .transmit_pin = {0, 9}, .receive_pin = {0, 10},
        .baudrate = 115200, .data_width = HAL_UART_DATA_WIDTH_8, .parity = HAL_UART_PARITY_NONE,
        .stop_count = HAL_UART_STOP_COUNT_1, .timeout = 100};
    hal_uart_t *uart;
    hal_timer_config_t periodic = {.instance = 0, .period = 1, .callback = on_timer};
    hal_timer_t *timer;
    assert(hal_gpio_config(&gpio, &pin) == HAL_STATUS_NOT_INITIALIZED && gpio == NULL);
    assert(hal_i2c_config(&i2c, &bus) == HAL_STATUS_NOT_INITIALIZED && i2c == NULL);
    assert(hal_uart_config(&uart, &serial) == HAL_STATUS_NOT_INITIALIZED && uart == NULL);
    assert(hal_timer_config(&timer, &periodic) == HAL_STATUS_NOT_INITIALIZED && timer == NULL);
    assert(hal_system_seal() == HAL_STATUS_NOT_INITIALIZED);
    assert(hal_system_init(NULL) == HAL_STATUS_INVALID_ARGUMENT);
    hal_system_config_t bad = {.memory_buffer = arena + 1, .memory_capacity = 100};
    assert(hal_system_init(&bad) == HAL_STATUS_INVALID_ARGUMENT);
    if (strcmp(argv[1], "clock_failure") == 0) {
        fail_clock = true;
        assert(hal_system_init(&system) == HAL_STATUS_ERROR);
        assert(hal_memory_get_info(&(hal_memory_info_t){0}) == HAL_MEMORY_NOT_INITIALIZED);
        assert(hal_gpio_config(&gpio, &pin) == HAL_STATUS_NOT_INITIALIZED);
        assert(hal_i2c_config(&i2c, &bus) == HAL_STATUS_NOT_INITIALIZED && i2c == NULL);
        assert(hal_uart_config(&uart, &serial) == HAL_STATUS_NOT_INITIALIZED && uart == NULL);
        assert(hal_timer_config(&timer, &periodic) == HAL_STATUS_NOT_INITIALIZED && timer == NULL);
        fail_clock = false;
    }
    if (strcmp(argv[1], "no_memory") == 0) {
        system.memory_capacity = 1;
    }
    if (strcmp(argv[1], "bsp") == 0) {
        system.memory_capacity = 2048;
    }
    assert(hal_system_init(&system) == HAL_STATUS_OK);
    assert(hal_system_init(&system) == HAL_STATUS_INVALID_STATE);
    assert((RCC->APB2ENR & RCC_APB2ENR_SYSCFGEN) != 0);
#if defined(STM32F411xE)
    if (strcmp(argv[1], "bsp") == 0) {
        bsp_resources_t resources;
        assert(bsp_init(on_timer, &resources) == HAL_STATUS_OK);
        assert(resources.button != NULL && resources.led != NULL && resources.devices_i2c != NULL);
        assert(resources.telemetry_uart != NULL && resources.timebase_timer != NULL);
        assert(!resources.button_active_level && resources.led_active_level);
        assert(resources.sht31_address == 0x44 && resources.ssd1306_address == 0x3C);
        assert((RCC->AHB1ENR & (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN)) ==
               (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN));
        assert((GPIOB->MODER & 0xF) == 4 && (GPIOB->PUPDR & 0xF) == 0);
        assert(alternate(GPIOB, 6) == 4 && alternate(GPIOB, 7) == 4);
        assert((GPIOB->OTYPER & 0xC0) == 0xC0);
        assert(alternate(GPIOA, 2) == 7 && alternate(GPIOA, 3) == 7);
        assert((RCC->APB1ENR & (RCC_APB1ENR_I2C1EN | RCC_APB1ENR_USART2EN | RCC_APB1ENR_TIM2EN)) ==
               (RCC_APB1ENR_I2C1EN | RCC_APB1ENR_USART2EN | RCC_APB1ENR_TIM2EN));
        assert((I2C1->CR1 & I2C_CR1_PE) != 0 && (USART2->CR1 & USART_CR1_UE) != 0);
        assert((USART2->CR1 & (USART_CR1_M | USART_CR1_PCE)) == 0);
        assert((TIM2->CR1 & TIM_CR1_CEN) == 0 && (TIM2->DIER & TIM_DIER_UIE) == 0);
        assert(interrupts_enabled[TIM2_IRQn] && notifications == 0);
        assert(used() > 0 && used() <= system.memory_capacity);
        assert(hal_system_seal() == HAL_STATUS_OK);

        bool level;
        GPIOB->IDR = 0;
        assert(hal_gpio_read(resources.button, &level) == HAL_STATUS_OK && level == resources.button_active_level);
        assert(hal_gpio_write(resources.led, resources.led_active_level) == HAL_STATUS_OK && GPIOB->BSRR == 2);
        const uint8_t message[] = {1, 2, 3};
        uint32_t transmitted;
        USART2->SR = USART_SR_TXE | USART_SR_TC;
        assert(hal_uart_write(resources.telemetry_uart, message, sizeof(message), &transmitted) == HAL_STATUS_OK);
        assert(transmitted == sizeof(message));
        I2C1->SR1 = I2C_SR1_SB | I2C_SR1_ADDR | I2C_SR1_TXE | I2C_SR1_BTF;
        assert(hal_i2c_write(resources.devices_i2c, resources.sht31_address, message, sizeof(message)) == HAL_STATUS_OK);
        assert(hal_timer_start(resources.timebase_timer) == HAL_STATUS_OK);
        TIM2->SR = TIM_SR_UIF;
        TIM2_IRQHandler();
        assert(notifications == 1);
        puts("bsp integration OK");
        return 0;
    }
#endif
    if (strcmp(argv[1], "no_memory") == 0) {
        assert(hal_gpio_config(&gpio, &pin) == HAL_STATUS_NO_MEMORY && gpio == NULL);
        assert(hal_gpio_config(&gpio, &pin) == HAL_STATUS_NO_MEMORY);
        assert(hal_i2c_config(&i2c, &bus) == HAL_STATUS_NO_MEMORY && i2c == NULL);
        assert(hal_i2c_config(&i2c, &bus) == HAL_STATUS_NO_MEMORY);
        assert(hal_uart_config(&uart, &serial) == HAL_STATUS_NO_MEMORY && uart == NULL);
        assert(hal_uart_config(&uart, &serial) == HAL_STATUS_NO_MEMORY);
        assert(hal_timer_config(&timer, &periodic) == HAL_STATUS_NO_MEMORY && timer == NULL);
        assert(used() == 0 && RCC->AHB1ENR == 0);
        puts("no_memory OK");
        return 0;
    }

    assert(hal_gpio_config(&gpio, &pin) == HAL_STATUS_OK);
    assert((RCC->AHB1ENR & RCC_AHB1ENR_GPIOCEN) != 0);
    assert(GPIOC->BSRR == 1U << 13);
    assert(((GPIOC->MODER >> 26) & 3) == 1 && (GPIOC->OTYPER & (1U << 13)) == 0);
    assert(hal_gpio_write(gpio, false) == HAL_STATUS_OK && GPIOC->BSRR == 1U << 29);
    bool level = true;
    assert(hal_gpio_read(gpio, &level) == HAL_STATUS_NOT_SUPPORTED && level);
    uint32_t before = used();
    hal_gpio_t *duplicate = gpio;
    assert(hal_gpio_config(&duplicate, &pin) == HAL_STATUS_RESOURCE_BUSY && duplicate == NULL);
    assert(used() == before);
    pin.pin = (hal_pin_t){0, 0};
    pin.direction = HAL_GPIO_INPUT;
    pin.pull = HAL_GPIO_PULL_UP;
    hal_gpio_t *input;
    assert(hal_gpio_config(&input, &pin) == HAL_STATUS_OK);
    GPIOA->IDR = 1;
    assert(hal_gpio_read(input, &level) == HAL_STATUS_OK && level);
    assert((GPIOA->PUPDR & 3) == 1);

    before = used();
    bus.timeout = UINT32_MAX;
    assert(hal_i2c_config(&i2c, &bus) == HAL_STATUS_NOT_SUPPORTED && used() == before);
    bus.timeout = 100;
    if (strcmp(argv[1], "i2c_failure") == 0) {
        fail_i2c_init = true;
        assert(hal_i2c_config(&i2c, &bus) == HAL_STATUS_ERROR && i2c == NULL);
        assert(used() > before);
        fail_i2c_init = false;
        assert(hal_i2c_config(&i2c, &bus) == HAL_STATUS_RESOURCE_BUSY);
        puts("i2c_failure OK");
        return 0;
    }
    assert(hal_i2c_config(&i2c, &bus) == HAL_STATUS_OK);
    assert((RCC->APB1ENR & RCC_APB1ENR_I2C1EN) != 0 && (RCC->AHB1ENR & RCC_AHB1ENR_GPIOBEN) != 0);
    assert(alternate(GPIOB, 6) == 4 && alternate(GPIOB, 7) == 4);
    assert((GPIOB->OTYPER & 0xC0) == 0xC0 && (GPIOB->PUPDR & 0xF000) == 0);
    assert((I2C1->CCR & I2C_CCR_CCR) == pclk1 / 200000);
    uint8_t data[] = {1, 2, 3};
    assert(hal_i2c_write(i2c, 0x07, data, 1) == HAL_STATUS_INVALID_ARGUMENT);
    assert(hal_i2c_read(i2c, 0x44, data, 65536) == HAL_STATUS_NOT_SUPPORTED);
    I2C1->SR1 = I2C_SR1_SB | I2C_SR1_ADDR | I2C_SR1_TXE | I2C_SR1_BTF;
    assert(hal_i2c_write(i2c, 0x44, data, sizeof(data)) == HAL_STATUS_OK);
    assert((I2C1->CR1 & I2C_CR1_STOP) != 0);
    I2C1->SR1 = I2C_SR1_SB | I2C_SR1_AF;
    assert(hal_i2c_write(i2c, 0x44, data, 1) == HAL_STATUS_ERROR);
    I2C1->SR1 = 0;
    assert(hal_i2c_read(i2c, 0x44, data, 1) == HAL_STATUS_TIMEOUT);

    // Same I2C3 signal uses AF9 only on F411, not on F407.
    bus.instance = 2;
    bus.clock_pin = (hal_pin_t){0, 8};
    bus.data_pin = (hal_pin_t){1, 4};
#if defined(STM32F411xE)
    hal_i2c_t *third_bus;
    assert(hal_i2c_config(&third_bus, &bus) == HAL_STATUS_OK);
    assert(alternate(GPIOA, 8) == 4 && alternate(GPIOB, 4) == 9);
#else
    before = used();
    assert(hal_i2c_config(&i2c, &bus) == HAL_STATUS_NOT_SUPPORTED && used() == before);
#endif

    assert(hal_uart_config(&uart, &serial) == HAL_STATUS_OK);
    assert((RCC->APB2ENR & RCC_APB2ENR_USART1EN) != 0);
    assert(alternate(GPIOA, 9) == 7 && alternate(GPIOA, 10) == 7);
    assert((USART1->CR1 & (USART_CR1_M | USART_CR1_PCE)) == 0);
    USART1->SR = USART_SR_TXE | USART_SR_TC;
    uint32_t transmitted = 99;
    assert(hal_uart_write(uart, data, sizeof(data), &transmitted) == HAL_STATUS_OK && transmitted == sizeof(data));
    partial_uart = true;
    assert(hal_uart_write(uart, data, sizeof(data), &transmitted) == HAL_STATUS_TIMEOUT && transmitted == 2);
    partial_uart = false;
    USART1->SR = 0;
    assert(hal_uart_write(uart, data, sizeof(data), &transmitted) == HAL_STATUS_TIMEOUT && transmitted == 0);
    assert(hal_uart_write(uart, NULL, 0, &transmitted) == HAL_STATUS_OK && transmitted == 0);

    serial.instance = 1;
    serial.transmit_pin = (hal_pin_t){0, 2};
    serial.receive_pin = (hal_pin_t){0, 3};
    serial.data_width = HAL_UART_DATA_WIDTH_7;
    before = used();
    assert(hal_uart_config(&uart, &serial) == HAL_STATUS_NOT_SUPPORTED && used() == before);
    serial.parity = HAL_UART_PARITY_EVEN;
    assert(hal_uart_config(&uart, &serial) == HAL_STATUS_OK);
    assert((USART2->CR1 & USART_CR1_PCE) != 0 && (USART2->CR1 & USART_CR1_M) == 0);
    USART2->SR = USART_SR_TXE | USART_SR_TC;
    data[2] = 0x80;
    USART2->DR = 0x55;
    assert(hal_uart_write(uart, data, sizeof(data), &transmitted) == HAL_STATUS_INVALID_ARGUMENT && transmitted == 0);
    assert(USART2->DR == 0x55);
    serial.instance = 5;
    serial.transmit_pin = (hal_pin_t){2, 6};
    serial.receive_pin = (hal_pin_t){2, 7};
    serial.data_width = HAL_UART_DATA_WIDTH_8;
    assert(hal_uart_config(&uart, &serial) == HAL_STATUS_OK);
    assert((USART6->CR1 & (USART_CR1_M | USART_CR1_PCE)) == (USART_CR1_M | USART_CR1_PCE));

    // Controller conflict must not claim the alternative route's pins.
    serial.instance = 0;
    serial.transmit_pin = (hal_pin_t){1, 6};
    serial.receive_pin = (hal_pin_t){1, 7};
    before = used();
    assert(hal_uart_config(&uart, &serial) == HAL_STATUS_RESOURCE_BUSY && used() == before);

    if (strcmp(argv[1], "timpre") == 0) {
#if defined(RCC_DCKCFGR_TIMPRE)
        RCC->DCKCFGR |= RCC_DCKCFGR_TIMPRE;
        RCC->CFGR = RCC_CFGR_PPRE1_DIV8;
        pclk1 = core_clock / 8;
#endif
    }
    assert(hal_timer_config(&timer, &periodic) == HAL_STATUS_OK);
    uint32_t frequency = pclk1 * 2;
#if defined(RCC_DCKCFGR_TIMPRE)
    if (strcmp(argv[1], "timpre") == 0) { frequency = pclk1 * 4; }
#endif
    assert(((uint64_t)TIM2->PSC + 1) * ((uint64_t)TIM2->ARR + 1) == frequency / 1000);
    assert((RCC->APB1ENR & RCC_APB1ENR_TIM2EN) != 0 && interrupts_enabled[TIM2_IRQn]);
    assert((TIM2->CR1 & TIM_CR1_CEN) == 0 && (TIM2->DIER & TIM_DIER_UIE) == 0);
    TIM2->SR = TIM_SR_UIF;
    TIM2_IRQHandler();
    assert(notifications == 0);
    periodic.instance = 1;
    periodic.period = UINT32_MAX;
    before = used();
    hal_timer_t *other;
    assert(hal_timer_config(&other, &periodic) == HAL_STATUS_NOT_SUPPORTED && used() == before);
    periodic.period = 1000;
    assert(hal_timer_config(&other, &periodic) == HAL_STATUS_OK);
    assert(((uint64_t)TIM3->PSC + 1) * ((uint64_t)TIM3->ARR + 1) == frequency);
    assert(TIM3->PSC <= UINT16_MAX && TIM3->ARR <= UINT16_MAX);

    assert(hal_system_seal() == HAL_STATUS_OK && hal_system_seal() == HAL_STATUS_OK);
    pin.pin = (hal_pin_t){0, 1};
    before = used();
    assert(hal_gpio_config(&duplicate, &pin) == HAL_STATUS_INVALID_STATE && used() == before);
    bus.instance = 0;
    bus.clock_pin = (hal_pin_t){1, 6};
    bus.data_pin = (hal_pin_t){1, 7};
    assert(hal_i2c_config(&i2c, &bus) == HAL_STATUS_INVALID_STATE && i2c == NULL);
    assert(hal_uart_config(&uart, &serial) == HAL_STATUS_INVALID_STATE && uart == NULL);
    assert(hal_timer_config(&other, &periodic) == HAL_STATUS_INVALID_STATE && other == NULL);
    assert(used() == before);
    if (strcmp(argv[1], "start_failure") == 0) {
        fail_timer_start = true;
        assert(hal_timer_start(timer) == HAL_STATUS_ERROR);
        TIM2->SR = TIM_SR_UIF;
        TIM2_IRQHandler();
        assert(notifications == 0 && (TIM2->DIER & TIM_DIER_UIE) == 0 && (TIM2->CR1 & TIM_CR1_CEN) == 0);
        fail_timer_start = false;
    }
    assert(hal_timer_start(timer) == HAL_STATUS_OK && test_interrupt_mask == 0);
    assert(TIM2->CNT == 0 && (TIM2->CR1 & TIM_CR1_CEN) != 0);
    TIM2->SR = TIM_SR_UIF;
    TIM2_IRQHandler();
    assert(notifications == 1 && (TIM2->SR & TIM_SR_UIF) == 0);
    TIM2_IRQHandler();
    assert(notifications == 1);
    assert(hal_timer_start(timer) == HAL_STATUS_INVALID_STATE);
    puts(argv[1]);
    return 0;
}
