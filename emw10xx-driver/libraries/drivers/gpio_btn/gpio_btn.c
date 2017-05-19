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

#include "mico_platform.h"
#include "gpio_btn.h"

#define keys_log(M, ...) custom_log("USER_KEYS", M, ##__VA_ARGS__)
#define keys_log_trace() custom_log_trace("USER_KEYS")

/*-------------------------------- VARIABLES ---------------------------------*/

/*------------------------------ USER INTERFACES -----------------------------*/

typedef struct _button_context_t{
  mico_gpio_t gpio;
  int timeout;
  mico_timer_t _user_button_timer;
  button_pressed_cb pressed_func;
  button_long_pressed_cb long_pressed_func;
  uint32_t start_time;
} button_context_t;

static button_context_t context[5];

//typedef void (*_button_irq_handler)( void* arg );

static void button_irq_handler( void* arg )
{
  button_context_t *_context = arg;

  int interval = -1;
  
  if ( mico_gpio_input_get( _context->gpio ) == 0 ) {
    mico_gpio_enable_irq( _context->gpio, IRQ_TRIGGER_RISING_EDGE, button_irq_handler, _context );
    _context->start_time = mico_rtos_get_time()+1;
    mico_rtos_start_timer(&_context->_user_button_timer);
  } else {
    interval = (int)mico_rtos_get_time() + 1 - _context->start_time ;
    if ( (_context->start_time  != 0) && interval > 50 && interval < _context->timeout){
      /* button clicked once */
      if( _context->pressed_func != NULL )
        (_context->pressed_func)();
    }
    mico_gpio_enable_irq( _context->gpio, IRQ_TRIGGER_FALLING_EDGE, button_irq_handler, _context );
    mico_rtos_stop_timer(&_context->_user_button_timer);
    _context->start_time  = 0;
  }
}

void (*button_irq_handler_array[5])() = {button_irq_handler, button_irq_handler, button_irq_handler, button_irq_handler, button_irq_handler};


static void button_timeout_handler( void* arg )
{
  button_context_t *_context = arg;

  _context->start_time = 0;
  if( _context->long_pressed_func != NULL )
    (_context->long_pressed_func)();
}

void (*button_timeout_handler_array[5])() = {button_timeout_handler, button_timeout_handler, button_timeout_handler, button_timeout_handler, button_timeout_handler};



void button_init( int index, button_init_t init)
{
  context[index].gpio = init.gpio;
  context[index].start_time = 0;
  context[index].timeout = init.long_pressed_timeout;
  context[index].pressed_func = init.pressed_func;
  context[index].long_pressed_func = init.long_pressed_func;

  mico_gpio_initialize( init.gpio, INPUT_PULL_UP );
  mico_rtos_init_timer( &context[index]._user_button_timer, init.long_pressed_timeout, button_timeout_handler_array[index], &context[index] );
  mico_gpio_enable_irq( init.gpio, IRQ_TRIGGER_FALLING_EDGE, button_irq_handler_array[index], &context[index] );
}




