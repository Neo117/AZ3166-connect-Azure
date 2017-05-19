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

#include "mico.h"

#include "StringUtils.h"  
#include "system.h"      

#define SYS_LED_TRIGGER_INTERVAL 100 
#define SYS_LED_TRIGGER_INTERVAL_AFTER_EASYLINK 500 

static mico_timer_t _Led_EL_timer;
static bool _Led_EL_timer_initialized = false;

static void _led_EL_Timeout_handler( void* arg )
{
  (void)(arg);
  mico_gpio_output_trigger((mico_gpio_t)MICO_SYS_LED);
}

WEAK void mico_system_delegate_config_will_start( void )
{
  /*Led trigger*/
  if(_Led_EL_timer_initialized == true)
  {
    mico_stop_timer(&_Led_EL_timer);
    mico_deinit_timer( &_Led_EL_timer );
    _Led_EL_timer_initialized = false;
  }

  mico_init_timer(&_Led_EL_timer, SYS_LED_TRIGGER_INTERVAL, _led_EL_Timeout_handler, NULL);
  mico_start_timer(&_Led_EL_timer);
  _Led_EL_timer_initialized = true;
  return;
}

WEAK void mico_system_delegate_soft_ap_will_start( void )
{
  return;
}

WEAK void mico_system_delegate_config_will_stop( void )
{
  if(_Led_EL_timer_initialized == true)
  {
    mico_stop_timer(&_Led_EL_timer);
    mico_deinit_timer( &_Led_EL_timer );
    _Led_EL_timer_initialized = false;
  }
  MicoSysLed(true);
  return;
}

WEAK void mico_system_delegate_config_recv_ssid ( char *ssid, char *key )
{
  UNUSED_PARAMETER(ssid);
  UNUSED_PARAMETER(key);

  if(_Led_EL_timer_initialized == true)
  {
    mico_stop_timer(&_Led_EL_timer);
    mico_deinit_timer( &_Led_EL_timer );
    _Led_EL_timer_initialized = false;
  }

  mico_init_timer(&_Led_EL_timer, SYS_LED_TRIGGER_INTERVAL_AFTER_EASYLINK, _led_EL_Timeout_handler, NULL);
  mico_start_timer(&_Led_EL_timer);
  _Led_EL_timer_initialized = true;
  return;
}

WEAK void mico_system_delegate_config_success( mico_config_source_t source )
{
  //system_log( "Configed by %d", source );
  UNUSED_PARAMETER(source);
  return;
}


WEAK OSStatus mico_system_delegate_config_recv_auth_data(char * anthData  )
{
  (void)(anthData);
  return kNoErr;
}
