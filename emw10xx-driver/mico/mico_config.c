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

#include "mico_common.h"
#include "mico_board.h"
#include "mico_board_conf.h"

#ifdef MICO_DEFAULT_APPLICATION_STACK_SIZE
uint32_t  app_stack_size = MICO_DEFAULT_APPLICATION_STACK_SIZE; 
#else
uint32_t  app_stack_size = 1500;
#endif


#ifdef  MICO_DEFAULT_TICK_RATE_HZ
const uint32_t  mico_tick_rate_hz = MICO_DEFAULT_TICK_RATE_HZ;
#else 
const uint32_t  mico_tick_rate_hz = 1000; // Default OS tick is 1000Hz
#endif

#ifndef MCU_CLOCK_HZ
	#error "MCU_CLOCK_HZ not defined in device header!"
#else
	const uint32_t  mico_cpu_clock_hz = MCU_CLOCK_HZ;
#endif

#ifndef CORTEX_NVIC_PRIO_BITS
	#define CORTEX_NVIC_PRIO_BITS          4
	#warning "CORTEX_NVIC_PRIO_BITS not defined in device header file; using default!"
#endif

const int CFG_PRIO_BITS = CORTEX_NVIC_PRIO_BITS;

const uint32_t  mico_timer_queue_len = 5;

const uint32_t mico_nmode_enable = true;

#ifdef DEBUG
int mico_debug_enabled = 1;
#else
int mico_debug_enabled = 0;
#endif

#ifdef SDIO_1_BIT
int sdio_1_bit_mode = 1;
#else
int sdio_1_bit_mode = 0;
#endif

