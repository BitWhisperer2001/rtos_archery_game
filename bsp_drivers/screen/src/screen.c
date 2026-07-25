#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_i2c.h"

#include "system_init.h"
#include "system_fault.h"

#include "ssd1306.h"
#include "screen.h"

#include "cmsis_gcc.h"

#define SCREEN_IO_SCL       (GPIO_Pin_6)
#define SCREEN_IO_SDA       (GPIO_Pin_7)
#define SCREEN_IO_PORT      (GPIOB)
#define SCREEN_IO_BUS_CLK   (RCC_AHB1Periph_GPIOB)
#define SCREEN_BUS_CLK      (RCC_APB1Periph_I2C1)
#define SCREEN_PERPH        (I2C1)

static void             screen_start(void);
static void             screen_io_i2c_config(void);
static void             screen_gpio_delay(void);
static bool             screen_i2c_bus_recovery(void);
static void             screen_i2c_peripheral_config(void);
static screen_status_t  screen_i2c_check_error(void);
static screen_status_t  screen_i2c_wait_flag(uint32_t flag, FlagStatus expected_state);
static screen_status_t  screen_data_write_once(uint8_t address, const uint8_t *data, uint16_t length);
       screen_status_t  screen_data_write(uint8_t address, const uint8_t *data, uint16_t length);

static void screen_io_i2c_config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /*
     * Initialize the complete SPL structure before overriding board-specific
     * fields; otherwise future library fields can receive indeterminate RAM.
     */
    GPIO_StructInit(&GPIO_InitStructure);
    RCC_AHB1PeriphClockCmd(SCREEN_IO_BUS_CLK, ENABLE);
    GPIO_PinAFConfig(SCREEN_IO_PORT, GPIO_PinSource6,  GPIO_AF_I2C1);
    GPIO_PinAFConfig(SCREEN_IO_PORT, GPIO_PinSource7, GPIO_AF_I2C1);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Pin = SCREEN_IO_SCL | SCREEN_IO_SDA;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
    GPIO_Init(SCREEN_IO_PORT, &GPIO_InitStructure);
}

/*
 * OLED transfer intentionally remains timeout-bounded polling I2C. The
 * screen task is the sole framebuffer/I2C owner, so this simple model cannot
 * race rendering and is easier to recover than a half-finished DMA bus.
 */

static void screen_gpio_delay(void)
{
    for (volatile uint32_t i = 0; i < 100U; i++) {
        __NOP();
    }
}

static bool screen_i2c_bus_recovery(void)
{
    GPIO_InitTypeDef gpio;

    I2C_Cmd(SCREEN_PERPH, DISABLE);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = SCREEN_IO_SCL | SCREEN_IO_SDA;
    gpio.GPIO_Mode = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_OD;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Medium_Speed;
    GPIO_Init(SCREEN_IO_PORT, &gpio);

    GPIO_SetBits(SCREEN_IO_PORT, SCREEN_IO_SCL | SCREEN_IO_SDA);

    screen_gpio_delay();

    for (uint8_t i = 0; i < 9U; i++) {
        if (GPIO_ReadInputDataBit(SCREEN_IO_PORT, SCREEN_IO_SDA) != Bit_RESET) {
            break;
        }
        GPIO_ResetBits(SCREEN_IO_PORT, SCREEN_IO_SCL);
        screen_gpio_delay();
        GPIO_SetBits(SCREEN_IO_PORT, SCREEN_IO_SCL);
        screen_gpio_delay();
    }
    GPIO_ResetBits(SCREEN_IO_PORT, SCREEN_IO_SDA);
    screen_gpio_delay();

    GPIO_SetBits(SCREEN_IO_PORT, SCREEN_IO_SCL);
    screen_gpio_delay();

    GPIO_SetBits(SCREEN_IO_PORT, SCREEN_IO_SDA);
    screen_gpio_delay();

    bool released = GPIO_ReadInputDataBit(SCREEN_IO_PORT, SCREEN_IO_SCL) != Bit_RESET && GPIO_ReadInputDataBit(SCREEN_IO_PORT, SCREEN_IO_SDA) != Bit_RESET;

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);

    screen_io_i2c_config();
    screen_i2c_peripheral_config();

    return released;
}

static void screen_start(void)
{
    /* Initialization failure is fatal because the game has no alternate UI. */
    if (SSD1306_Init() == 0U) {
        system_panic(SYSTEM_PANIC_RESOURCE_CREATION);
    }
}

static void screen_i2c_peripheral_config(void)
{
    I2C_InitTypeDef I2C1_InitStructure;

    /* Start from documented SPL defaults before applying the 400 kHz profile. */
    I2C_StructInit(&I2C1_InitStructure);
    RCC_APB1PeriphClockCmd(SCREEN_BUS_CLK, ENABLE);
    I2C_DeInit(SCREEN_PERPH);
    I2C1_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C1_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C1_InitStructure.I2C_ClockSpeed = 400000;
    I2C1_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C1_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C1_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_Init(SCREEN_PERPH, &I2C1_InitStructure);
    I2C_ITConfig(SCREEN_PERPH, I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR, DISABLE);
    I2C_Cmd(SCREEN_PERPH, ENABLE);
}

void screen_init(void)
{
    screen_io_i2c_config();
    screen_i2c_peripheral_config();
    screen_start();
}

static screen_status_t screen_i2c_check_error(void)
{
    uint16_t sr1 = SCREEN_PERPH->SR1;

    if ((sr1 & I2C_SR1_AF) != 0U) {
        I2C_ClearFlag(SCREEN_PERPH, I2C_FLAG_AF);
        return SCREEN_STATUS_NACK;
    }

    if ((sr1 & I2C_SR1_BERR) != 0U) {
        I2C_ClearFlag(SCREEN_PERPH, I2C_FLAG_BERR);
        return SCREEN_STATUS_BUS_ERROR;
    }

    if ((sr1 & I2C_SR1_ARLO) != 0U) {
        I2C_ClearFlag(SCREEN_PERPH, I2C_FLAG_ARLO);
        return SCREEN_STATUS_ARBITRATION_LOST;
    }

    if ((sr1 & I2C_SR1_OVR) != 0U) {
        I2C_ClearFlag(SCREEN_PERPH, I2C_FLAG_OVR);
        return SCREEN_STATUS_OVERRUN;
    }

    if ((sr1 & I2C_SR1_TIMEOUT) != 0U) {
        I2C_ClearFlag(SCREEN_PERPH, I2C_FLAG_TIMEOUT);
        return SCREEN_STATUS_TIMEOUT;
    }

    return SCREEN_STATUS_OK;
}

static screen_status_t screen_i2c_wait_flag(uint32_t flag, FlagStatus expected_state)
{
    uint32_t start_tick = sys_get_tick();
    while (I2C_GetFlagStatus(SCREEN_PERPH, flag) != expected_state) {
        screen_status_t status = screen_i2c_check_error();
        if (status != SCREEN_STATUS_OK) {
            return status;
        }
        if ((uint32_t)(sys_get_tick() - start_tick) >=
            SCREEN_I2C_TIMEOUT_MS) {
            return SCREEN_STATUS_TIMEOUT;
        }
    }
    return SCREEN_STATUS_OK;
}

static screen_status_t screen_data_write_once(uint8_t address, const uint8_t *data, uint16_t length)
{
    screen_status_t status;
    status = screen_i2c_wait_flag(I2C_FLAG_BUSY, RESET);
    if (status != SCREEN_STATUS_OK) {
        return status;
    }
    I2C_GenerateSTART(SCREEN_PERPH, ENABLE);
    status = screen_i2c_wait_flag(I2C_FLAG_SB, SET);
    if (status != SCREEN_STATUS_OK) {
        return status;
    }
    I2C_Send7bitAddress(SCREEN_PERPH, address, I2C_Direction_Transmitter);

    status = screen_i2c_wait_flag(I2C_FLAG_ADDR, SET);
    if (status != SCREEN_STATUS_OK) {
        return status;
    }
    (void)SCREEN_PERPH->SR1;
    (void)SCREEN_PERPH->SR2;

    for (uint16_t i = 0; i < length; i++) {
        status = screen_i2c_wait_flag(I2C_FLAG_TXE, SET);
        if (status != SCREEN_STATUS_OK) {
            return status;
        }
        I2C_SendData(SCREEN_PERPH, data[i]);
    }
    status = screen_i2c_wait_flag(I2C_FLAG_BTF, SET);
    if (status != SCREEN_STATUS_OK) {
        return status;
    }
    I2C_GenerateSTOP(SCREEN_PERPH, ENABLE);
    return SCREEN_STATUS_OK;
}

screen_status_t screen_data_write(uint8_t address, const uint8_t *data, uint16_t length)
{
    screen_status_t status;

    if ((data == NULL) || (length == 0U)) {
        return SCREEN_STATUS_INVALID_ARGUMENT;
    }
    status = screen_data_write_once(address, data, length);

    if (status == SCREEN_STATUS_OK) {
        return SCREEN_STATUS_OK;
    }

    I2C_GenerateSTOP(SCREEN_PERPH, ENABLE);

    if (!screen_i2c_bus_recovery()) {
        return SCREEN_STATUS_RECOVERY_FAILED;
    }
    return screen_data_write_once(address, data, length);
}
