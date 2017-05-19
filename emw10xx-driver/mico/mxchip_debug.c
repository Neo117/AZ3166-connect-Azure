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

#include <stdarg.h>

#include "mico_rtos.h"
#include "mxchip_debug.h"

debug_printf pPrintffunc = NULL;
int debug_level;


/* Enable library printf debug msg to callback function. 
 * debug level is from 0 to 7. 0 is the highest debug level. 
 * only system debug msg level is lower than level, the callbcak is called.
 */
void system_debug_enable(int level, debug_printf callback)
{
    debug_level = level;
    pPrintffunc = callback;
}




