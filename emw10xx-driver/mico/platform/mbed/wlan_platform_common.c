/* MiCO Team
 * Copyright (c) 2017 MXCHIP Information Tech. Co.,Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>

#include "mico.h"

#include "mico_board.h"
#include "mico_board_conf.h"
#include "wlan_platform_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/
#ifndef MIN
#define MIN(a,b) (((a) < (b))?(a):(b))
#endif

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
OSStatus host_platform_deinit_wlan_powersave_clock( void );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

void host_platform_reset_wifi( bool reset_asserted )
{
#if defined (MICO_USE_WIFI_RESET_PIN )
    ( reset_asserted == true ) ? platform_gpio_output_low( &wifi_control_pin_drivers[WIFI_PIN_RESET] ) : platform_gpio_output_high( &wifi_control_pin_drivers[WIFI_PIN_RESET] );
#else
    UNUSED_PARAMETER( reset_asserted );
#endif
}

void host_platform_power_wifi( bool power_enabled )
{
#if   defined ( MICO_USE_WIFI_POWER_PIN ) && defined ( MICO_USE_WIFI_POWER_PIN_ACTIVE_HIGH )
    ( power_enabled == true ) ? platform_gpio_output_high( &wifi_control_pin_drivers[WIFI_PIN_POWER] ) : platform_gpio_output_low ( &wifi_control_pin_drivers[WIFI_PIN_POWER] );
#elif defined ( MICO_USE_WIFI_POWER_PIN )
    ( power_enabled == true ) ? platform_gpio_output_low ( &wifi_control_pin_drivers[WIFI_PIN_POWER] ) : platform_gpio_output_high( &wifi_control_pin_drivers[WIFI_PIN_POWER] );
#else
    UNUSED_PARAMETER( power_enabled );
#endif
}

OSStatus host_platform_init( void )
{
    host_platform_deinit_wlan_powersave_clock( );

#if defined ( MICO_USE_WIFI_RESET_PIN )
    platform_gpio_init( &wifi_control_pin_drivers[WIFI_PIN_RESET], &wifi_control_pins[WIFI_PIN_RESET], OUTPUT_PUSH_PULL );
    host_platform_reset_wifi( true );  /* Start wifi chip in reset */
#endif
    
#if defined ( MICO_USE_WIFI_POWER_PIN )
    platform_gpio_init( &wifi_control_pin_drivers[WIFI_PIN_POWER], &wifi_control_pins[WIFI_PIN_RESET],  OUTPUT_PUSH_PULL );
    host_platform_power_wifi( false ); /* Start wifi chip with regulators off */
#endif

    return kNoErr;
}

OSStatus host_platform_deinit( void )
{
#if defined ( MICO_USE_WIFI_RESET_PIN )
    platform_gpio_init( &wifi_control_pin_drivers[WIFI_PIN_RESET], &wifi_control_pins[WIFI_PIN_RESET], OUTPUT_PUSH_PULL );
    host_platform_reset_wifi( true );  /* Start wifi chip in reset */
#endif

#if defined ( MICO_USE_WIFI_POWER_PIN )
    platform_gpio_init( &wifi_control_pin_drivers[WIFI_PIN_POWER], &wifi_control_pins[WIFI_PIN_POWER], OUTPUT_PUSH_PULL );
    host_platform_power_wifi( false ); /* Start wifi chip with regulators off */
#endif

    host_platform_deinit_wlan_powersave_clock( );

    return kNoErr;
}

bool host_platform_is_in_interrupt_context( void )
{
    /* From the ARM Cortex-M3 Techinical Reference Manual
     * 0xE000ED04   ICSR    RW [a]  Privileged  0x00000000  Interrupt Control and State Register */
    uint32_t active_interrupt_vector = (uint32_t)( SCB ->ICSR & 0x3fU );

    if ( active_interrupt_vector != 0 )
    {
        return true;
    }
    else
    {
        return false;
    }
}


/* Used to give a 32k clock to EMW1062 wifi rf module */
OSStatus host_platform_init_wlan_powersave_clock( void )
{
#if defined ( MICO_USE_WIFI_32K_CLOCK_MCO ) && defined ( MICO_USE_WIFI_32K_PIN )
    platform_gpio_set_alternate_function( wifi_control_pins[WIFI_PIN_32K_CLK].port, wifi_control_pins[WIFI_PIN_32K_CLK].pin_number, GPIO_OType_PP, GPIO_PuPd_NOPULL, GPIO_AF_MCO );

    /* enable LSE output on MCO1 */
    RCC_MCO1Config( RCC_MCO1Source_LSE, RCC_MCO1Div_1 );
    return kNoErr;
#elif defined ( MICO_USE_WIFI_32K_PIN )
    return host_platform_deinit_wlan_powersave_clock( );
#else
    return kNoErr;
#endif
}

OSStatus host_platform_deinit_wlan_powersave_clock( void )
{
#ifndef MICO_USE_WIFI_32K_PIN
    return kNoErr;
#else
    /* Tie the pin to ground */
    platform_gpio_init( &wifi_control_pin_drivers[WIFI_PIN_32K_CLK], &wifi_control_pins[WIFI_PIN_32K_CLK], OUTPUT_PUSH_PULL );
    platform_gpio_output_low( &wifi_control_pin_drivers[WIFI_PIN_32K_CLK] );
    return kNoErr;
#endif
}

#ifndef BOOTLOADER

#ifdef MICO_USE_BUILTIN_RF_DRIVER
extern uint32_t wifi_firmware_image_size;
extern unsigned char wifi_firmware_image[];

uint32_t platform_get_wifi_image_size(void)
{
  return wifi_firmware_image_size;
}

uint32_t platform_get_wifi_image(unsigned char* buffer, uint32_t size, uint32_t offset)
{
  uint32_t buffer_size;
  buffer_size = MIN(size, (platform_get_wifi_image_size() - offset));
  memcpy(buffer, &wifi_firmware_image[offset], buffer_size);

  return buffer_size;
}

//////////////////////////////////////////////////////////////////////////////////////////
#else
static uint32_t image_size = 0x0;

uint32_t platform_get_wifi_image_size(void)
{
#define READ_LEN 2048
    mico_logic_partition_t *driver_partition = MicoFlashGetInfo( MICO_PARTITION_RF_FIRMWARE );
    uint32_t offset = driver_partition->partition_length;
    uint32_t *p;
    uint32_t *buf = (uint32_t *)malloc(READ_LEN);

    image_size = driver_partition->partition_length;
    do {
        offset -= READ_LEN; // Next block
        MicoFlashRead( MICO_PARTITION_RF_FIRMWARE, &offset, (uint8_t *)buf, READ_LEN);
        offset -= READ_LEN; // MicoFlashRead will increase FlashAddress READ_LEN, move back.
        p = buf + (READ_LEN - 4)/sizeof(uint32_t);
        while(p >= buf) {
            if (*p != 0xFFFFFFFF) {
                goto EXIT;
            }
            p--;
            image_size -= 4;
        }
    } while (offset > 0);

EXIT:
    free(buf);
    return image_size;
}


uint32_t platform_get_wifi_image(unsigned char* buffer, uint32_t size, uint32_t offset)
{
    uint32_t buffer_size;
    uint32_t read_address = offset;
    mico_logic_partition_t *driver_partition = MicoFlashGetInfo( MICO_PARTITION_RF_FIRMWARE );

    if( image_size == 0)
      image_size = driver_partition->partition_length;

    buffer_size = MIN(size, (image_size - offset));

    MicoFlashRead( MICO_PARTITION_RF_FIRMWARE, &read_address, buffer, buffer_size);
    return buffer_size;
}
#endif


#endif

////TODO
void update_rx_time(void)
{

}

#ifdef __cplusplus
}
#endif



