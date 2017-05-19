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

#ifndef PLATFORM_TOOLCHAIN_H
#define PLATFORM_TOOLCHAIN_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/
#ifndef WEAK
#define WEAK __weak
#endif

#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE
#endif

#ifndef USED
#define USED __root
#endif
  
#ifndef MAY_BE_UNUSED
#define MAY_BE_UNUSED
#endif

#ifndef NORETURN
#define NORETURN
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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
//void *memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen );
void *memrchr( const void *s, int c, size_t n );
void iar_set_msp(void*);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* #ifndef INCLUDED_WWD_TOOLCHAIN_H */
