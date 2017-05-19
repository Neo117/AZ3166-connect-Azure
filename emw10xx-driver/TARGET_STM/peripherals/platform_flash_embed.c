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


/* Includes ------------------------------------------------------------------*/
#include "platform_logging.h"
#include "platform_peripheral.h"

#include "mico_board.h"
#include "mico_board_conf.h"


/* Private constants --------------------------------------------------------*/
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbyte */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbyte */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbyte */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbyte */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbyte */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbyte */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbyte */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbyte */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbyte */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbyte */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbyte */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbyte */

/* End of the Flash address */
#define FLASH_START_ADDRESS     (uint32_t)0x08000000  
#define FLASH_END_ADDRESS       (uint32_t)0x080FFFFF
#define FLASH_SIZE              (FLASH_END_ADDRESS -  FLASH_START_ADDRESS + 1)
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static uint32_t _GetSector( uint32_t Address );

#ifdef MCU_ENABLE_FLASH_PROTECT
static uint32_t _GetWRPSector(uint32_t Address);
static OSStatus internalFlashProtect(uint32_t StartAddress, uint32_t EndAddress, bool enable);
#endif

int iflash_init( void )
{
    HAL_FLASH_Unlock( );
    /* Clear pending flags (if any) */
    __HAL_FLASH_CLEAR_FLAG(
        FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR );
    return 0;
}

OSStatus iflash_erase( uint32_t StartAddress, uint32_t EndAddress )
{
  OSStatus err = kNoErr;
  uint32_t sector_err = 0;
  HAL_StatusTypeDef hal_status;
  FLASH_EraseInitTypeDef pEraseInit;

  pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
  pEraseInit.Sector = _GetSector(StartAddress);
  pEraseInit.NbSectors = _GetSector(EndAddress) - _GetSector(StartAddress) + 1;
  pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

  hal_status = HAL_FLASHEx_Erase(&pEraseInit, &sector_err);
  require_action( HAL_OK == hal_status, exit, err =  kWriteErr);

  exit:
    return err;
}

OSStatus iflash_write_byte( volatile uint32_t* FlashAddress, uint8_t* Data, uint32_t DataLength )
{
    uint32_t i = 0, dataInRam;
    OSStatus err = kNoErr;
    HAL_StatusTypeDef hal_status;

    for ( i = 0; (i < DataLength) && (*FlashAddress <= (FLASH_END_ADDRESS )); i++ ) {
        dataInRam = *(uint8_t*) (Data + i);
        hal_status = HAL_FLASH_Program( FLASH_TYPEPROGRAM_BYTE, *FlashAddress, dataInRam );
        require_action( hal_status == HAL_OK, exit, err = kWriteErr );
        /* Readout and check */
        require_action( *(uint8_t* )*FlashAddress == dataInRam, exit, err = kChecksumErr );
        *FlashAddress += 1;
    }

exit:
    return err;
}


OSStatus iflash_write(volatile uint32_t* FlashAddress, uint32_t* Data ,uint32_t DataLength)
{
    OSStatus err = kNoErr;
    HAL_StatusTypeDef hal_status;
    uint32_t i = 0;
    uint32_t dataInRam;
    uint8_t startNumber;
    uint32_t DataLength32 = DataLength;

    /*First bytes that are not 32bit align*/
    if ( *FlashAddress % 4 ) {
        startNumber = 4 - (*FlashAddress) % 4;
        err = iflash_write_byte( FlashAddress, (uint8_t *) Data, startNumber );
        require_noerr( err, exit );
        DataLength32 = DataLength - startNumber;
        Data = (uint32_t *) ((uint32_t) Data + startNumber);
    }

    /*Program flash by words*/
    for ( i = 0; (i < DataLength32 / 4) && (*FlashAddress <= (FLASH_END_ADDRESS - 3)); i++ ) {
        /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
         be done by word */
        dataInRam = *(Data + i);
        hal_status = HAL_FLASH_Program( FLASH_TYPEPROGRAM_WORD, *FlashAddress, dataInRam );
        require_action( hal_status == HAL_OK, exit, err = kWriteErr );
        /* Readout and check */
        require_action( *(uint32_t* )*FlashAddress == dataInRam, exit, err = kChecksumErr );
        /* Increment FLASH destination address */
        *FlashAddress += 4;
    }

    /*Last bytes that cannot be write by a 32 bit word*/
    err = iflash_write_byte( FlashAddress, (uint8_t *) Data + i * 4, DataLength32 - i * 4 );
    require_noerr( err, exit );

exit:
    return err;
}


#ifdef MCU_EBANLE_FLASH_PROTECT
OSStatus internalFlashProtect(uint32_t StartAddress, uint32_t EndAddress, bool enable)
{
  OSStatus err = kNoErr;
  uint16_t WRP = 0x0;
  uint32_t StartSector, EndSector, i = 0;
  bool needupdate = false;
  
  /* Get the sector where start the user flash area */
  StartSector = _GetWRPSector(StartAddress);
  EndSector = _GetWRPSector(EndAddress);
  
  for(i = StartSector; i <= EndSector; i=i<<1)
  {
    WRP = FLASH_OB_GetWRP();

    if( ( enable == true && (WRP & i) == 0x0 ) ||
        ( enable == false && (WRP & i) ) ) {
      continue;
    }
    if( needupdate == false){
      FLASH_OB_Unlock( );
      needupdate = true;
    }
    if( enable == true )
      FLASH_OB_WRPConfig( i, ENABLE );
    else
      FLASH_OB_WRPConfig( i, DISABLE );
  }
  
  if( needupdate == true){
    FLASH_OB_Launch( );
    FLASH_OB_Lock( );
  }

  return err;
}
#endif


/**
* @brief  Gets the sector of a given address
* @param  Address: Flash address
* @retval The sector of a given address
*/
static uint32_t _GetSector(uint32_t Address)
{
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_7;
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_8;
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_9;
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_10;
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
  {
    sector = FLASH_SECTOR_11;
  }
  return sector;
}


#ifdef MCU_EBANLE_FLASH_PROTECT
/**
* @brief  Gets the sector of a given address
* @param  Address: Flash address
* @retval The sector of a given address
*/
static uint32_t _GetWRPSector(uint32_t Address)
{
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = OB_WRP_Sector_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = OB_WRP_Sector_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = OB_WRP_Sector_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = OB_WRP_Sector_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = OB_WRP_Sector_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = OB_WRP_Sector_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = OB_WRP_Sector_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = OB_WRP_Sector_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = OB_WRP_Sector_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = OB_WRP_Sector_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = OB_WRP_Sector_10;  
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
  {
    sector = OB_WRP_Sector_11;  
  }
  return sector;
}
#endif

