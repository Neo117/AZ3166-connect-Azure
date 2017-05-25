/**
 ******************************************************************************
 * @file    stm32412g_discovery.c
 * @author  MCD Application Team
 * @version V2.0.0
 * @date    27-January-2017
 * @brief   This file provides a set of firmware functions to manage LEDs,
 *          push-buttons and COM ports available on STM32412G-DISCOVERY board
 *          (MB1209) from STMicroelectronics.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "audio_config.h"


static void I2Cx_MspInit( I2C_HandleTypeDef *i2c_handler );
static void I2Cx_Init( I2C_HandleTypeDef *i2c_handler );

static HAL_StatusTypeDef I2Cx_ReadMultiple( I2C_HandleTypeDef *i2c_handler, uint8_t Addr, uint16_t Reg,
                                            uint16_t MemAddSize,
                                            uint8_t *Buffer, uint16_t Length );
static HAL_StatusTypeDef I2Cx_WriteMultiple( I2C_HandleTypeDef *i2c_handler, uint8_t Addr, uint16_t Reg,
                                             uint16_t MemAddSize,
                                             uint8_t *Buffer, uint16_t Length );
static void I2Cx_Error( I2C_HandleTypeDef *i2c_handler, uint8_t Addr );

/* AUDIO IO functions */
void AUDIO_IO_Init( void );
void AUDIO_IO_DeInit( void );
void AUDIO_IO_Write( uint8_t Addr, uint16_t Reg, uint16_t Value );
uint16_t AUDIO_IO_Read( uint8_t Addr, uint16_t Reg );
void AUDIO_IO_Delay( uint32_t Delay );


static I2C_HandleTypeDef hI2cAudioHandler;

/**
 * @}
 */

/*******************************************************************************
 BUS OPERATIONS
 *******************************************************************************/

/******************************* I2C Routines *********************************/
/**
 * @brief  Initializes I2C MSP.
 * @param  i2c_handler : I2C handler
 */
static void I2Cx_MspInit( I2C_HandleTypeDef *i2c_handler )
{
    GPIO_InitTypeDef gpio_init_structure;

    if ( i2c_handler == (I2C_HandleTypeDef*) (&hI2cAudioHandler) )
         {
        /* AUDIO I2C MSP init */

        /*** Configure the GPIOs ***/
        /* Enable GPIO clock */
        DISCOVERY_AUDIO_I2Cx_SCL_SDA_GPIO_CLK_ENABLE()
                        ;

        /* Configure I2C Tx as alternate function */
        gpio_init_structure.Pin = DISCOVERY_AUDIO_I2Cx_SCL_PIN;
        gpio_init_structure.Mode = GPIO_MODE_AF_OD;
        gpio_init_structure.Pull = GPIO_NOPULL;
        gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_structure.Alternate = DISCOVERY_AUDIO_I2Cx_SCL_SDA_AF;
        HAL_GPIO_Init( DISCOVERY_AUDIO_I2Cx_SCL_SDA_GPIO_PORT, &gpio_init_structure );

        /* Configure I2C Rx as alternate function */
        gpio_init_structure.Pin = DISCOVERY_AUDIO_I2Cx_SDA_PIN;
        HAL_GPIO_Init( DISCOVERY_AUDIO_I2Cx_SCL_SDA_GPIO_PORT, &gpio_init_structure );

        /*** Configure the I2C peripheral ***/
        /* Enable I2C clock */
        DISCOVERY_AUDIO_I2Cx_CLK_ENABLE()
                        ;

        /* Force the I2C peripheral clock reset */
        DISCOVERY_AUDIO_I2Cx_FORCE_RESET();

        /* Release the I2C peripheral clock reset */
        DISCOVERY_AUDIO_I2Cx_RELEASE_RESET();

        /* Enable and set I2Cx Interrupt to a lower priority */
        HAL_NVIC_SetPriority( DISCOVERY_AUDIO_I2Cx_EV_IRQn, 0x0F, 0x00 );
        HAL_NVIC_EnableIRQ( DISCOVERY_AUDIO_I2Cx_EV_IRQn );

        /* Enable and set I2Cx Interrupt to a lower priority */
        HAL_NVIC_SetPriority( DISCOVERY_AUDIO_I2Cx_ER_IRQn, 0x0F, 0x00 );
        HAL_NVIC_EnableIRQ( DISCOVERY_AUDIO_I2Cx_ER_IRQn );
    }
    else
    {
        /* External and Arduino connector I2C MSP init */

        /*** Configure the GPIOs ***/
        /* Enable GPIO clock */
        DISCOVERY_EXT_I2Cx_SCL_SDA_GPIO_CLK_ENABLE()
                        ;

        /* Configure I2C Tx as alternate function */
        gpio_init_structure.Pin = DISCOVERY_EXT_I2Cx_SCL_PIN;
        gpio_init_structure.Mode = GPIO_MODE_AF_OD;
        gpio_init_structure.Pull = GPIO_NOPULL;
        gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_structure.Alternate = DISCOVERY_EXT_I2Cx_SCL_AF;
        HAL_GPIO_Init( DISCOVERY_EXT_I2Cx_SCL_SDA_GPIO_PORT, &gpio_init_structure );

        /* Configure I2C Rx as alternate function */
        gpio_init_structure.Pin = DISCOVERY_EXT_I2Cx_SDA_PIN;
        gpio_init_structure.Alternate = DISCOVERY_EXT_I2Cx_SDA_AF;
        HAL_GPIO_Init( DISCOVERY_EXT_I2Cx_SCL_SDA_GPIO_PORT, &gpio_init_structure );

        /*** Configure the I2C peripheral ***/
        /* Enable I2C clock */
        DISCOVERY_EXT_I2Cx_CLK_ENABLE()
                        ;

        /* Force the I2C peripheral clock reset */
        DISCOVERY_EXT_I2Cx_FORCE_RESET();

        /* Release the I2C peripheral clock reset */
        DISCOVERY_EXT_I2Cx_RELEASE_RESET();

        /* Enable and set I2Cx Interrupt to a lower priority */
        HAL_NVIC_SetPriority( DISCOVERY_EXT_I2Cx_EV_IRQn, 0x0F, 0x00 );
        HAL_NVIC_EnableIRQ( DISCOVERY_EXT_I2Cx_EV_IRQn );

        /* Enable and set I2Cx Interrupt to a lower priority */
        HAL_NVIC_SetPriority( DISCOVERY_EXT_I2Cx_ER_IRQn, 0x0F, 0x00 );
        HAL_NVIC_EnableIRQ( DISCOVERY_EXT_I2Cx_ER_IRQn );
    }
}

/**
 * @brief  Initializes I2C HAL.
 * @param  i2c_handler : I2C handler
 */
static void I2Cx_Init( I2C_HandleTypeDef *i2c_handler )
{

    i2c_handler->Instance = I2C1;
    i2c_handler->Init.ClockSpeed = 100000;
    i2c_handler->Init.DutyCycle = I2C_DUTYCYCLE_2;
    i2c_handler->Init.OwnAddress1 = 0;
    i2c_handler->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c_handler->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c_handler->Init.OwnAddress2 = 0;
    i2c_handler->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c_handler->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    /* Init the I2C */
    I2Cx_MspInit( i2c_handler );
    HAL_I2C_MspInit( i2c_handler );
    HAL_I2C_Init( i2c_handler );
}

/**
 * @brief  Reads multiple data.
 * @param  i2c_handler : I2C handler
 * @param  Addr: I2C address
 * @param  Reg: Reg address
 * @param  MemAddress: Memory address
 * @param  Buffer: Pointer to data buffer
 * @param  Length: Length of the data
 * @retval HAL status
 */
static HAL_StatusTypeDef I2Cx_ReadMultiple( I2C_HandleTypeDef *i2c_handler,
                                            uint8_t Addr,
                                            uint16_t Reg,
                                            uint16_t MemAddress,
                                            uint8_t *Buffer,
                                            uint16_t Length )
{
    HAL_StatusTypeDef status = HAL_OK;

    status = HAL_I2C_Mem_Read( i2c_handler, Addr, (uint16_t) Reg, MemAddress, Buffer, Length, 1000 );

    /* Check the communication status */
    if ( status != HAL_OK )
         {
        /* I2C error occurred */
        I2Cx_Error( i2c_handler, Addr );
    }
    return status;
}

/**
 * @brief  Writes a value in a register of the device through BUS in using DMA mode.
 * @param  i2c_handler : I2C handler
 * @param  Addr: Device address on BUS Bus.
 * @param  Reg: The target register address to write
 * @param  MemAddress: Memory address
 * @param  Buffer: The target register value to be written
 * @param  Length: buffer size to be written
 * @retval HAL status
 */
static HAL_StatusTypeDef I2Cx_WriteMultiple( I2C_HandleTypeDef *i2c_handler,
                                             uint8_t Addr,
                                             uint16_t Reg,
                                             uint16_t MemAddress,
                                             uint8_t *Buffer,
                                             uint16_t Length )
{
    HAL_StatusTypeDef status = HAL_OK;

    status = HAL_I2C_Mem_Write( i2c_handler, Addr, (uint16_t) Reg, MemAddress, Buffer, Length, 1000 );

    /* Check the communication status */
    if ( status != HAL_OK )
         {
        /* Re-Initialize the I2C Bus */
        I2Cx_Error( i2c_handler, Addr );
    }
    return status;
}

/**
 * @brief  Manages error callback by re-initializing I2C.
 * @param  i2c_handler : I2C handler
 * @param  Addr: I2C Address
 */
static void I2Cx_Error( I2C_HandleTypeDef *i2c_handler, uint8_t Addr )
{
    /* De-initialize the I2C communication bus */
    HAL_I2C_DeInit( i2c_handler );

    /* Re-Initialize the I2C communication bus */
    I2Cx_Init( i2c_handler );
}

/**
 * @brief  Deinitializes I2C interface
 * @param  i2c_handler : I2C handler
 */
static void I2Cx_DeInit( I2C_HandleTypeDef *i2c_handler )
{
    if ( i2c_handler == (I2C_HandleTypeDef*) (&hI2cAudioHandler) )
         {
        /* Audio and LCD I2C configuration */
        i2c_handler->Instance = DISCOVERY_AUDIO_I2Cx;
    }
    else
    {
        /* External, EEPROM and Arduino connector I2C configuration */
        i2c_handler->Instance = DISCOVERY_EXT_I2Cx;
    }

    /* Disable I2C block */
    __HAL_I2C_DISABLE( i2c_handler );

    /* DeInit the I2S */
    HAL_I2C_DeInit( i2c_handler );
}

/********************************* LINK AUDIO *********************************/

/**
 * @brief  Initializes Audio low level.
 */
void AUDIO_IO_Init( void )
{
    I2Cx_Init( &hI2cAudioHandler );
}

/**
 * @brief  Deinitializes Audio low level.
 */
void AUDIO_IO_DeInit( void )
{
    I2Cx_DeInit( &hI2cAudioHandler );
}

/**
 * @brief  Writes a single data.
 * @param  Addr: I2C address
 * @param  Reg: Reg address
 * @param  Value: Data to be written
 */
void AUDIO_IO_Write( uint8_t Addr, uint16_t Reg, uint16_t Value )
{
    uint16_t tmp = Value;

    uint16_t reg_write = ((uint16_t)( Reg << 1 ) | ((Value >> 8)));

    Value = (uint8_t)( tmp );

    I2Cx_WriteMultiple( &hI2cAudioHandler, Addr, reg_write, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &Value, 1 );
}

/**
 * @brief  Reads a single data.
 * @param  Addr: I2C address
 * @param  Reg: Reg address
 * @retval Data to be read
 */
uint16_t AUDIO_IO_Read( uint8_t Addr, uint16_t Reg )
{
    uint16_t read_value = 0, tmp = 0;

    I2Cx_ReadMultiple( &hI2cAudioHandler, Addr, Reg, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &read_value, 2 );

    tmp = ((uint16_t)( read_value >> 8 ) & 0x00FF);

    tmp |= ((uint16_t)( read_value << 8 ) & 0xFF00);

    read_value = tmp;

    return read_value;
}

/**
 * @brief  AUDIO Codec delay
 * @param  Delay: Delay in ms
 */
void AUDIO_IO_Delay( uint32_t Delay )
{
    HAL_Delay( Delay );
}
