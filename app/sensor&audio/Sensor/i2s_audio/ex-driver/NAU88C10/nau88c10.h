/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __NAU88C10_H
#define __NAU88C10_H

/* Includes ------------------------------------------------------------------*/
#include "../Common/audio.h"

/** @addtogroup BSP
  * @{
  */ 

/** @addtogroup Component
  * @{
  */ 
  
/** @addtogroup NAU88C10
  * @{
  */

/** @defgroup NAU88C10_Exported_Types
  * @{
  */

/**
  * @}
  */

/** @defgroup NAU88C10_Exported_Constants
  * @{
  */ 

/******************************************************************************/
/***************************  Codec User defines ******************************/
/******************************************************************************/
/* Codec output DEVICE */
#define OUTPUT_DEVICE_SPEAKER                 ((uint16_t)0x0001)
#define OUTPUT_DEVICE_HEADPHONE               ((uint16_t)0x0002)
#define OUTPUT_DEVICE_BOTH                    ((uint16_t)0x0003)
#define OUTPUT_DEVICE_AUTO                    ((uint16_t)0x0004)
#define INPUT_DEVICE_DIGITAL_MICROPHONE_1     ((uint16_t)0x0100)
#define INPUT_DEVICE_DIGITAL_MICROPHONE_2     ((uint16_t)0x0200)
#define INPUT_DEVICE_INPUT_LINE_1             ((uint16_t)0x0300)
#define INPUT_DEVICE_INPUT_LINE_2             ((uint16_t)0x0400)
#define INPUT_DEVICE_DIGITAL_MIC1_MIC2        ((uint16_t)0x0800)

/* Volume Levels values */
#define DEFAULT_VOLMIN                0x00
#define DEFAULT_VOLMAX                0xFF
#define DEFAULT_VOLSTEP               0x04

#define AUDIO_PAUSE                   0
#define AUDIO_RESUME                  1

/* Codec POWER DOWN modes */
#define CODEC_PDWN_HW                 1
#define CODEC_PDWN_SW                 2

/* MUTE commands */
#define AUDIO_MUTE_ON                 1
#define AUDIO_MUTE_OFF                0

/* AUDIO FREQUENCY */
#define AUDIO_FREQUENCY_192K          ((uint32_t)192000)
#define AUDIO_FREQUENCY_96K           ((uint32_t)96000)
#define AUDIO_FREQUENCY_48K           ((uint32_t)48000)
#define AUDIO_FREQUENCY_44K           ((uint32_t)44100)
#define AUDIO_FREQUENCY_32K           ((uint32_t)32000)
#define AUDIO_FREQUENCY_22K           ((uint32_t)22050)
#define AUDIO_FREQUENCY_16K           ((uint32_t)16000)
#define AUDIO_FREQUENCY_11K           ((uint32_t)11025)
#define AUDIO_FREQUENCY_8K            ((uint32_t)8000)  

#define VOLUME_CONVERT(Volume)        (((Volume) > 100)? 100:((uint8_t)(((Volume) * 63) / 100)))
#define VOLUME_IN_CONVERT(Volume)     (((Volume) >= 100)? 239:((uint8_t)(((Volume) * 240) / 100)))

/******************************************************************************/
/****************************** REGISTER MAPPING ******************************/
/******************************************************************************/
/** 
  * @brief  NAU88C10 ID
  */  
#define  NAU88C10_ID    0x00ca

/**
  * @brief Device ID Register: Reading from this register will indicate device 
  *                            family ID 8994h
  */
#define nau88c10_CHIPID_ADDR                  0x80 //0x40 << 1


/**
  * @}
  */ 

/** @defgroup nau88c10_Exported_Macros
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup nau88c10_Exported_Functions
  * @{
  */
    
/*------------------------------------------------------------------------------
                           Audio Codec functions 
------------------------------------------------------------------------------*/
/* High Layer codec functions */
uint32_t nau88c10_Init(uint16_t DeviceAddr, uint16_t OutputInputDevice, uint32_t AudioFreq);
void     nau88c10_DeInit(void);
uint32_t nau88c10_ReadID(uint16_t DeviceAddr);
uint32_t nau88c10_ReadRegister(uint16_t DeviceAddr);
uint32_t nau88c10_WriteRegister(uint16_t DeviceAddr);
uint32_t nau88c10_Play(uint16_t DeviceAddr, uint16_t* pBuffer, uint16_t Size);
uint32_t nau88c10_Pause(uint16_t DeviceAddr);
uint32_t nau88c10_Resume(uint16_t DeviceAddr);
uint32_t nau88c10_Stop(uint16_t DeviceAddr, uint32_t Cmd);
uint32_t nau88c10_SetVolume(uint16_t DeviceAddr, uint8_t Volume);
uint32_t nau88c10_SetMute(uint16_t DeviceAddr, uint32_t Cmd);
uint32_t nau88c10_SetOutputMode(uint16_t DeviceAddr, uint8_t Output);
uint32_t nau88c10_SetFrequency(uint16_t DeviceAddr, uint32_t AudioFreq);
uint32_t nau88c10_Reset(uint16_t DeviceAddr);

/* AUDIO IO functions */
void    AUDIO_IO_Init(void);
void    AUDIO_IO_DeInit(void);
void    AUDIO_IO_Write(uint8_t Addr, uint16_t Reg, uint16_t Value);
uint8_t AUDIO_IO_Read(uint8_t Addr, uint16_t Reg);
void    AUDIO_IO_Delay(uint32_t Delay);

/* Audio driver structure */
extern AUDIO_DrvTypeDef   nau88c10_drv;

#endif /* __nau88c10_H */
