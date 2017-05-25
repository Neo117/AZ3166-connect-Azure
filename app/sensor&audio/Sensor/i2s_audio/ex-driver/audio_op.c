
/* Includes ------------------------------------------------------------------*/
#include "audio_op.h"

AUDIO_DrvTypeDef *audio_drv;
I2S_HandleTypeDef haudio_i2s; /* for Audio_OUT and Audio_IN_analog mic */
I2S_HandleTypeDef haudio_in_i2sext; /* for Analog mic with full duplex mode  */
AUDIOIN_ContextTypeDef hAudioIn;

/* Buffers for right and left samples */
int32_t *pScratchBuff[DEFAULT_AUDIO_IN_CHANNEL_NBR];
int32_t ScratchSize;


/* Application Buffer Trigger */
__IO uint32_t AppBuffTrigger = 0;
__IO uint32_t AppBuffHalf = 0;

/**
 * @}
 */
void BSP_AUDIO_IN_MspInit( void *Params );
static void I2Sx_In_MspInit( void );
static void I2Sx_In_MspDeInit( void );

static void I2Sx_Out_Init( uint32_t AudioDataFormat,uint32_t AudioFreq );
static void I2Sx_Out_DeInit( void );

extern void AUDIO_OUT_I2Sx_DMAx_IRQHandler( void );
extern void AUDIO_IN_I2Sx_DMAx_IRQHandler( void );


/**
 * @}
 */

/**
 * @brief  Configures the audio peripherals.
 * @param  OutputDevice: OUTPUT_DEVICE_SPEAKER, OUTPUT_DEVICE_HEADPHONE,
 *                       or OUTPUT_DEVICE_BOTH.
 * @param  Volume: Initial volume level (from 0 (Mute) to 100 (Max))
 * @param  AudioFreq: Audio frequency used to play the audio stream.
 * @note   The I2S PLL input clock must be done in the user application.
 * @retval AUDIO_OK if correct communication, else wrong communication
 */
uint8_t BSP_AUDIO_IN_OUT_Init( uint16_t OutputDevice, uint32_t AudioDataFormat, uint32_t AudioFreq )
{
    uint8_t ret = AUDIO_ERROR;
    uint32_t deviceid = 0x00;
    uint16_t buffer_fake[16] = { 0x0 };

    I2Sx_Out_DeInit( );
    AUDIO_IO_DeInit( );

    /* PLL clock is set depending on the AudioFreq (44.1 kHz vs 48kHz groups) */
    BSP_AUDIO_OUT_ClockConfig( &haudio_i2s, AudioFreq, NULL );

    /* Configure the I2S peripheral */
    haudio_i2s.Instance = AUDIO_OUT_I2Sx;
    if ( HAL_I2S_GetState( &haudio_i2s ) == HAL_I2S_STATE_RESET )
         {
        /* Initialize the I2S Msp: this __weak function can be rewritten by the application */
        BSP_AUDIO_OUT_MspInit( &haudio_i2s, NULL );
        BSP_AUDIO_IN_MspInit( NULL );

    }

    I2Sx_Out_Init( AudioDataFormat,AudioFreq );

    AUDIO_IO_Init( );

    /* nau88c10 codec initialization */
    deviceid = nau88c10_drv.ReadID( AUDIO_I2C_READ_ADDRESS );
//    printf( "deviceid is 0x%02x \r\n", deviceid );

    if ( deviceid == NAU88C10_ID )
    {
        /* Reset the Codec Registers */
        nau88c10_drv.Reset( AUDIO_I2C_WRITE_ADDRESS );
        /* Initialize the audio driver structure */
        audio_drv = &nau88c10_drv;
        ret = AUDIO_OK;
    }
    else
    {
        ret = AUDIO_ERROR;
    }

    if ( ret == AUDIO_OK )
    {
        /* Send fake I2S data in order to generate MCLK needed by nau88c10 to set its registers
         * MCLK is generated only when a data stream is sent on I2S */
        HAL_I2S_Transmit_DMA( &haudio_i2s, buffer_fake, 16 );
        /* Initialize the codec internal registers */
        audio_drv->Init( AUDIO_I2C_WRITE_ADDRESS, OutputDevice, AudioFreq );

        /*for loopback test mode*/
//        audio_drv->WriteRegister(AUDIO_I2C_WRITE_ADDRESS);
//        printf( "register is 0x%x \r\n",audio_drv->ReadRegister(AUDIO_I2C_READ_ADDRESS) );
        /* Stop sending fake I2S data */
        HAL_I2S_DMAStop( &haudio_i2s );
    }

    return ret;
}

/**
 * @brief  Starts playing audio stream from a data buffer for a determined size.
 * @param  pBuffer: Pointer to the buffer
 * @param  Size: Number of audio data BYTES.
 * @retval AUDIO_OK if correct communication, else wrong communication
 */
uint8_t BSP_AUDIO_In_Out_Transfer( uint16_t* pBuffer, uint16_t* pBuffer_read, uint32_t Size )
{
    /* Update the Media layer and enable it for play */
    HAL_I2SEx_TransmitReceive_DMA( &haudio_i2s, pBuffer, pBuffer_read, DMA_MAX( Size ) );

    return AUDIO_OK;
}

void BSP_AUDIO_In_Out_Stop()
{
    HAL_I2S_DMAStop(&haudio_i2s);
}

/**
 * @brief  Sends n-Bytes on the I2S interface.
 * @param  pData: pointer on data address
 * @param  Size: number of data to be written
 */
void BSP_AUDIO_OUT_ChangeBuffer( uint16_t *pData, uint16_t* pBuffer_read, uint16_t Size )
{
    HAL_I2SEx_TransmitReceive_DMA( &haudio_i2s, pData, pBuffer_read, DMA_MAX( Size / AUDIODATA_SIZE ) );
}

/**
 * @brief  This function Pauses the audio file stream. In case
 *         of using DMA, the DMA Pause feature is used.
 * @note   When calling BSP_AUDIO_OUT_Pause() function for pause, only
 *          BSP_AUDIO_OUT_Resume() function should be called for resume (use of BSP_AUDIO_OUT_Play()
 *          function for resume could lead to unexpected behavior).
 * @retval AUDIO_OK if correct communication, else wrong communication
 */
uint8_t BSP_AUDIO_OUT_Pause( void )
{
    /* Call the Audio Codec Pause/Resume function */
    if ( audio_drv->Pause( AUDIO_I2C_ADDRESS ) != 0 ) {
        return AUDIO_ERROR;
    }
    else
    {
        /* Call the Media layer pause function */
        HAL_I2S_DMAPause( &haudio_i2s );

        /* Return AUDIO_OK when all operations are correctly done */
        return AUDIO_OK;
    }
}

/**
 * @brief  This function  Resumes the audio file stream.
 * @note   When calling BSP_AUDIO_OUT_Pause() function for pause, only
 *          BSP_AUDIO_OUT_Resume() function should be called for resume (use of BSP_AUDIO_OUT_Play()
 *          function for resume could lead to unexpected behavior).
 * @retval AUDIO_OK if correct communication, else wrong communication
 */
uint8_t BSP_AUDIO_OUT_Resume( void )
{
    /* Call the Media layer pause/resume function */
    /* DMA stream resumed before accessing nau88c10 register as nau88c10 needs the MCLK to be generated to access its registers
     * MCLK is generated only when a data stream is sent on I2S */
    HAL_I2S_DMAResume( &haudio_i2s );

    /* Call the Audio Codec Pause/Resume function */
    if ( audio_drv->Resume( AUDIO_I2C_ADDRESS ) != 0 )
         {
        return AUDIO_ERROR;
    }
    else
    {
        /* Return AUDIO_OK when all operations are correctly done */
        return AUDIO_OK;
    }
}

/**
 * @brief  Stops audio playing and Power down the Audio Codec.
 * @param  Option: could be one of the following parameters
 *           - CODEC_PDWN_SW: for software power off (by writing registers).
 *                            Then no need to reconfigure the Codec after power on.
 *           - CODEC_PDWN_HW: completely shut down the codec (physically).
 *                            Then need to reconfigure the Codec after power on.
 * @retval AUDIO_OK if correct communication, else wrong communication
 */
uint8_t BSP_AUDIO_OUT_Stop( uint32_t Option )
{
    /* Call the Media layer stop function */
    HAL_I2S_DMAStop( &haudio_i2s );

    /* Call Audio Codec Stop function */
    if ( audio_drv->Stop( AUDIO_I2C_ADDRESS, Option ) != 0 )
         {
        return AUDIO_ERROR;
    }
    else
    {
        if ( Option == CODEC_PDWN_HW )
        {
            /* Wait at least 100us */
            HAL_Delay( 1 );
        }
        /* Return AUDIO_OK when all operations are correctly done */
        return AUDIO_OK;
    }
}

/**
 * @brief  Controls the current audio volume level.
 * @param  Volume: Volume level to be set in percentage from 0% to 100% (0 for
 *         Mute and 100 for Max volume level).
 * @retval AUDIO_OK if correct communication, else wrong communication
 */
uint8_t BSP_AUDIO_OUT_SetVolume( uint8_t Volume )
{
    /* Call the codec volume control function with converted volume value */
    if ( audio_drv->SetVolume( AUDIO_I2C_ADDRESS, Volume ) != 0 )
         {
        return AUDIO_ERROR;
    }
    else
    {
        /* Return AUDIO_OK when all operations are correctly done */
        return AUDIO_OK;
    }
}

/**
 * @brief  Enables or disables the MUTE mode by software
 * @param  Cmd: Could be AUDIO_MUTE_ON to mute sound or AUDIO_MUTE_OFF to
 *         unmute the codec and restore previous volume level.
 * @retval AUDIO_OK if correct communication, else wrong communication
 */
uint8_t BSP_AUDIO_OUT_SetMute( uint32_t Cmd )
{
    /* Call the Codec Mute function */
    if ( audio_drv->SetMute( AUDIO_I2C_ADDRESS, Cmd ) != 0 )
         {
        return AUDIO_ERROR;
    }
    else
    {
        /* Return AUDIO_OK when all operations are correctly done */
        return AUDIO_OK;
    }
}

/**
 * @brief  Switch dynamically (while audio file is played) the output target
 *         (speaker or headphone).
 * @param  Output: The audio output target: OUTPUT_DEVICE_SPEAKER,
 *         OUTPUT_DEVICE_HEADPHONE or OUTPUT_DEVICE_BOTH
 * @retval AUDIO_OK if correct communication, else wrong communication
 */
uint8_t BSP_AUDIO_OUT_SetOutputMode( uint8_t Output )
{
    /* Call the Codec output device function */
    if ( audio_drv->SetOutputMode( AUDIO_I2C_ADDRESS, Output ) != 0 )
         {
        return AUDIO_ERROR;
    }
    else
    {
        /* Return AUDIO_OK when all operations are correctly done */
        return AUDIO_OK;
    }
}

/**
 * @brief  Updates the audio frequency.
 * @param  AudioFreq: Audio frequency used to play the audio stream.
 * @note   This API should be called after the BSP_AUDIO_OUT_Init() to adjust the
 *         audio frequency.
 */
void BSP_AUDIO_OUT_SetFrequency( uint32_t AudioFreq )
{
    /* PLL clock is set depending by the AudioFreq (44.1khz vs 48khz groups) */
    BSP_AUDIO_OUT_ClockConfig( &haudio_i2s, AudioFreq, NULL );

    /* Disable I2S peripheral to allow access to I2S internal registers */
    __HAL_I2S_DISABLE( &haudio_i2s );

    /* Update the I2S audio frequency configuration */
    haudio_i2s.Init.AudioFreq = AudioFreq;
    HAL_I2S_Init( &haudio_i2s );
    
    /* Enable I2S peripheral to generate MCLK */
    __HAL_I2S_ENABLE( &haudio_i2s );
}

/**
 * @brief  Deinit the audio peripherals.
 */
void BSP_AUDIO_OUT_DeInit( void )
{
    I2Sx_Out_DeInit( );
    /* DeInit the I2S MSP : this __weak function can be rewritten by the application */
    BSP_AUDIO_OUT_MspDeInit( &haudio_i2s, NULL );
}

/**
 * @brief  Tx Transfer completed callbacks.
 * @param  hi2s: I2S handle
 */
void HAL_I2S_TxCpltCallback( I2S_HandleTypeDef *hi2s )
{
    /* Manage the remaining file size and new address offset: This function
     should be coded by user (its prototype is already declared in stm32412g_discovery_audio.h) */
    BSP_AUDIO_OUT_TransferComplete_CallBack( );
}

void HAL_I2S_RxCpltCallback( I2S_HandleTypeDef *hi2s )
{
    BSP_AUDIO_IN_TransferComplete_CallBack( );
}
/**
 * @brief  I2S error callbacks.
 * @param  hi2s: I2S handle
 */
void HAL_I2S_ErrorCallback( I2S_HandleTypeDef *hi2s )
{
    BSP_AUDIO_OUT_Error_CallBack( );
}

/**
 * @brief  Initializes BSP_AUDIO_OUT MSP.
 * @param  hi2s: I2S handle
 * @param  Params : pointer on additional configuration parameters, can be NULL.
 */
__weak void BSP_AUDIO_OUT_MspInit( I2S_HandleTypeDef *hi2s, void *Params )
{
    static DMA_HandleTypeDef hdma_i2s_tx;
    GPIO_InitTypeDef GPIO_InitStruct;
    /* Peripheral clock enable */
    __HAL_RCC_SPI2_CLK_ENABLE()
                ;
    __HAL_RCC_GPIOB_CLK_ENABLE()
                ;
    __HAL_RCC_GPIOC_CLK_ENABLE()
                ;

    /**I2S2 GPIO Configuration
     PB12     ------> I2S2_WS
     PB13     ------> I2S2_CK
     PB14     ------> I2S2_ext_SD
     PB15     ------> I2S2_SD
     PC6     ------> I2S2_MCK
     */
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init( GPIOB, &GPIO_InitStruct );

    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI2;
    HAL_GPIO_Init( GPIOB, &GPIO_InitStruct );

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init( GPIOC, &GPIO_InitStruct );

    /* USER CODE BEGIN SPI2_MspInit 1 */
    /* Enable the DMA clock */
    AUDIO_OUT_I2Sx_DMAx_CLK_ENABLE()
    ;

    if ( hi2s->Instance == AUDIO_OUT_I2Sx )
    {
        /* Configure the hdma_i2s_rx handle parameters */
        hdma_i2s_tx.Init.Channel = AUDIO_OUT_I2Sx_DMAx_CHANNEL;
        hdma_i2s_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_i2s_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_i2s_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_i2s_tx.Init.PeriphDataAlignment = AUDIO_OUT_I2Sx_DMAx_PERIPH_DATA_SIZE;
        hdma_i2s_tx.Init.MemDataAlignment = AUDIO_OUT_I2Sx_DMAx_MEM_DATA_SIZE;
        hdma_i2s_tx.Init.Mode = DMA_NORMAL;
        hdma_i2s_tx.Init.Priority = DMA_PRIORITY_HIGH;
        hdma_i2s_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        hdma_i2s_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        hdma_i2s_tx.Init.MemBurst = DMA_MBURST_SINGLE;
        hdma_i2s_tx.Init.PeriphBurst = DMA_MBURST_SINGLE;

        hdma_i2s_tx.Instance = AUDIO_OUT_I2Sx_DMAx_STREAM;

        /* Associate the DMA handle */
        __HAL_LINKDMA( hi2s, hdmatx, hdma_i2s_tx );

        /* Deinitialize the Stream for new transfer */
        HAL_DMA_DeInit( &hdma_i2s_tx );

        /* Configure the DMA Stream */
        HAL_DMA_Init( &hdma_i2s_tx );
    }

    /* Enable and set I2Sx Interrupt to a lower priority */
    HAL_NVIC_SetPriority( SPI2_IRQn, 0x0F, 0x00 );
    HAL_NVIC_EnableIRQ( SPI2_IRQn );

    /* I2S DMA IRQ Channel configuration */
    HAL_NVIC_SetPriority( AUDIO_OUT_I2Sx_DMAx_IRQ, AUDIO_OUT_IRQ_PREPRIO, 0 );
    NVIC_SetVector( AUDIO_OUT_I2Sx_DMAx_IRQ, AUDIO_OUT_I2Sx_DMAx_IRQHandler );
    HAL_NVIC_EnableIRQ( AUDIO_OUT_I2Sx_DMAx_IRQ );

}

/**
 * @brief  Deinitializes I2S MSP.
 * @param  Params : pointer on additional configuration parameters, can be NULL.
 * @param  hi2s: I2S handle
 */
__weak void BSP_AUDIO_OUT_MspDeInit( I2S_HandleTypeDef *hi2s, void *Params )
{
    GPIO_InitTypeDef gpio_init_structure;

    /* I2S DMA IRQ Channel deactivation */
    HAL_NVIC_DisableIRQ( AUDIO_OUT_I2Sx_DMAx_IRQ );

    if ( hi2s->Instance == AUDIO_OUT_I2Sx )
    {
        /* Deinitialize the DMA stream */
        HAL_DMA_DeInit( hi2s->hdmatx );
    }

    /* Disable I2S peripheral */
    __HAL_I2S_DISABLE( hi2s );

    /* Deactives CODEC_I2S pins MCK, SCK, WS and SD by putting them in input mode */
    gpio_init_structure.Pin = AUDIO_OUT_I2Sx_MCK_PIN;
    HAL_GPIO_DeInit( AUDIO_OUT_I2Sx_MCK_GPIO_PORT, gpio_init_structure.Pin );

    gpio_init_structure.Pin = AUDIO_OUT_I2Sx_SCK_PIN;
    HAL_GPIO_DeInit( AUDIO_OUT_I2Sx_SCK_GPIO_PORT, gpio_init_structure.Pin );

    gpio_init_structure.Pin = AUDIO_OUT_I2Sx_WS_PIN;
    HAL_GPIO_DeInit( AUDIO_OUT_I2Sx_WS_GPIO_PORT, gpio_init_structure.Pin );

    gpio_init_structure.Pin = AUDIO_OUT_I2Sx_SD_PIN;
    HAL_GPIO_DeInit( AUDIO_OUT_I2Sx_SD_GPIO_PORT, gpio_init_structure.Pin );

    /* Disable I2S clock */
    AUDIO_OUT_I2Sx_CLK_DISABLE();

    /* GPIO pins clock and DMA clock can be shut down in the application
     by surcharging this __weak function */
}

/**
 * @brief  Clock Config.
 * @param  hi2s: might be required to set audio peripheral predivider if any.
 * @param  AudioFreq: Audio frequency used to play the audio stream.
 * @param  Params : pointer on additional configuration parameters, can be NULL.
 * @note   This API is called by BSP_AUDIO_OUT_Init() and BSP_AUDIO_OUT_SetFrequency()
 *         Being __weak it can be overwritten by the application
 */
__weak void BSP_AUDIO_OUT_ClockConfig( I2S_HandleTypeDef *hi2s, uint32_t AudioFreq, void *Params )
{
    RCC_PeriphCLKInitTypeDef rcc_ex_clk_init_struct;
    
    HAL_RCCEx_GetPeriphCLKConfig( &rcc_ex_clk_init_struct );
    
    /* Set the PLL configuration according to the audio frequency */
    if ( (AudioFreq == AUDIO_FREQUENCY_11K) || (AudioFreq == AUDIO_FREQUENCY_22K)
         || (AudioFreq == AUDIO_FREQUENCY_44K) )
         {
        /* Configure PLLI2S prescalers */
        rcc_ex_clk_init_struct.PeriphClockSelection = (RCC_PERIPHCLK_I2S_APB1 | RCC_PERIPHCLK_PLLI2S);
        rcc_ex_clk_init_struct.I2sApb1ClockSelection = RCC_I2SAPB1CLKSOURCE_PLLI2S;
        rcc_ex_clk_init_struct.PLLI2SSelection = RCC_PLLI2SCLKSOURCE_PLLSRC;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SM = 8;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SN = 271;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SR = 2;

        HAL_RCCEx_PeriphCLKConfig( &rcc_ex_clk_init_struct );
    }
    else if ( AudioFreq == AUDIO_FREQUENCY_96K ) /* AUDIO_FREQUENCY_96K */
    {
        /* I2S clock config */
        rcc_ex_clk_init_struct.PeriphClockSelection = (RCC_PERIPHCLK_I2S_APB1 | RCC_PERIPHCLK_PLLI2S);
        rcc_ex_clk_init_struct.I2sApb1ClockSelection = RCC_I2SAPB1CLKSOURCE_PLLI2S;
        rcc_ex_clk_init_struct.PLLI2SSelection = RCC_PLLI2SCLKSOURCE_PLLSRC;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SM = 8;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SN = 344;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SR = 2;

        HAL_RCCEx_PeriphCLKConfig( &rcc_ex_clk_init_struct );
    }
    else /* AUDIO_FREQUENCY_8K, AUDIO_FREQUENCY_16K, AUDIO_FREQUENCY_48K */
    {
        /* I2S clock config
         PLLI2S_VCO: VCO_344M
         I2S_CLK(first level) = PLLI2S_VCO/PLLI2SR = 344/7 = 49.142 Mhz
         I2S_CLK_x = I2S_CLK(first level)/PLLI2SDIVR = 49.142/1 = 49.142 Mhz */
        rcc_ex_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_I2S_APB1 | RCC_PERIPHCLK_PLLI2S;
        rcc_ex_clk_init_struct.I2sApb1ClockSelection = RCC_I2SAPB1CLKSOURCE_PLLI2S;
        rcc_ex_clk_init_struct.PLLI2SSelection = RCC_PLLI2SCLKSOURCE_PLLSRC;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SM = 26;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SN = 344;
        rcc_ex_clk_init_struct.PLLI2S.PLLI2SR = 7;

        HAL_RCCEx_PeriphCLKConfig( &rcc_ex_clk_init_struct );
    }
}

/*******************************************************************************
 Static Functions
 *******************************************************************************/

/**
 * @brief  Initializes the Audio Codec audio interface (I2S)
 * @note   This function assumes that the I2S input clock
 *         is already configured and ready to be used.
 * @param  AudioFreq: Audio frequency to be configured for the I2S peripheral.
 */
static void I2Sx_Out_Init( uint32_t AudioDataFormat,uint32_t AudioFreq )
{
    /* Initialize the hAudioInI2s Instance parameter */
    haudio_i2s.Instance = AUDIO_OUT_I2Sx;
    haudio_in_i2sext.Instance = I2S2ext;

    /* Disable I2S block */
    __HAL_I2S_DISABLE( &haudio_i2s );
    __HAL_I2S_DISABLE( &haudio_in_i2sext );

    /* I2S peripheral configuration */
    haudio_i2s.Init.AudioFreq = AudioFreq;
    haudio_i2s.Init.ClockSource = I2S_CLOCK_PLL;
    haudio_i2s.Init.CPOL = I2S_CPOL_LOW;
    haudio_i2s.Init.DataFormat = AudioDataFormat;
    haudio_i2s.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
    haudio_i2s.Init.Mode = I2S_MODE_MASTER_TX;
    haudio_i2s.Init.Standard = I2S_STANDARD_PHILIPS;
    haudio_i2s.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_ENABLE;

    /* Init the I2S */
    HAL_I2S_Init( &haudio_i2s );

    /* Enable I2S block */
    __HAL_I2S_ENABLE( &haudio_i2s );
    __HAL_I2S_ENABLE( &haudio_in_i2sext );
}

/**
 * @brief  Deinitializes the Audio Codec audio interface (I2S).
 */
static void I2Sx_Out_DeInit( void )
{
    /* Initialize the hAudioInI2s Instance parameter */
    haudio_i2s.Instance = AUDIO_OUT_I2Sx;

    /* Disable I2S block */
    __HAL_I2S_DISABLE( &haudio_i2s );

    /* DeInit the I2S */
    HAL_I2S_DeInit( &haudio_i2s );
}

void AUDIO_OUT_I2Sx_DMAx_IRQHandler( void )
{
    HAL_DMA_IRQHandler( haudio_i2s.hdmatx );
}

void AUDIO_IN_I2Sx_DMAx_IRQHandler( void )
{
    HAL_DMA_IRQHandler( haudio_i2s.hdmarx );
}

/**
 * @brief  Initializes the I2S MSP.
 */
static void I2Sx_In_MspInit( void )
{
    static DMA_HandleTypeDef hdma_i2s_rx;

    /* Enable the DMA clock */
    AUDIO_IN_I2Sx_DMAx_CLK_ENABLE()
    ;

    if ( haudio_i2s.Instance == AUDIO_IN_I2Sx ) {
        /* Configure the hdma_i2s_rx handle parameters */
        hdma_i2s_rx.Init.Channel = AUDIO_IN_I2Sx_DMAx_CHANNEL;
        hdma_i2s_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_i2s_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_i2s_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_i2s_rx.Init.PeriphDataAlignment = AUDIO_IN_I2Sx_DMAx_PERIPH_DATA_SIZE;
        hdma_i2s_rx.Init.MemDataAlignment = AUDIO_IN_I2Sx_DMAx_MEM_DATA_SIZE;
        hdma_i2s_rx.Init.Mode = DMA_NORMAL;
        hdma_i2s_rx.Init.Priority = DMA_PRIORITY_HIGH;
        hdma_i2s_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        hdma_i2s_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        hdma_i2s_rx.Init.MemBurst = DMA_MBURST_SINGLE;
        hdma_i2s_rx.Init.PeriphBurst = DMA_MBURST_SINGLE;

        hdma_i2s_rx.Instance = AUDIO_IN_I2Sx_DMAx_STREAM;

        /* Associate the DMA handle */
        __HAL_LINKDMA( &haudio_i2s, hdmarx, hdma_i2s_rx );

        /* Deinitialize the Stream for new transfer */
        HAL_DMA_DeInit( &hdma_i2s_rx );

        /* Configure the DMA Stream */
        HAL_DMA_Init( &hdma_i2s_rx );
    }

    /* I2S DMA IRQ Channel configuration */
    HAL_NVIC_SetPriority( AUDIO_IN_I2Sx_DMAx_IRQ, AUDIO_IN_IRQ_PREPRIO, 0 );
    NVIC_SetVector( AUDIO_IN_I2Sx_DMAx_IRQ, AUDIO_IN_I2Sx_DMAx_IRQHandler );
    HAL_NVIC_EnableIRQ( AUDIO_IN_I2Sx_DMAx_IRQ );
}

/**
 * @brief  De-Initializes the I2S MSP.
 */
static void I2Sx_In_MspDeInit( void )
{
    GPIO_InitTypeDef gpio_init_structure;

    /* I2S DMA IRQ Channel deactivation */
    HAL_NVIC_DisableIRQ( AUDIO_IN_I2Sx_DMAx_IRQ );

    if ( haudio_i2s.Instance == AUDIO_IN_I2Sx )
    {
        /* Deinitialize the DMA stream */
        HAL_DMA_DeInit( haudio_i2s.hdmarx );
    }

    /* Disable I2S peripheral */
    __HAL_I2S_DISABLE( &haudio_i2s );

    /* Deactives CODEC_I2S pins MCK by putting them in input mode */
    gpio_init_structure.Pin = AUDIO_OUT_I2Sx_MCK_PIN;
    HAL_GPIO_DeInit( AUDIO_OUT_I2Sx_MCK_GPIO_PORT, gpio_init_structure.Pin );

    gpio_init_structure.Pin = AUDIO_IN_I2Sx_EXT_SD_PIN;
    HAL_GPIO_DeInit( AUDIO_IN_I2Sx_EXT_SD_GPIO_PORT, gpio_init_structure.Pin );

    /* Disable I2S clock */
    AUDIO_IN_I2Sx_CLK_DISABLE();
}

/**
 * @brief  Initializes BSP_AUDIO_IN MSP.
 * @param  Params : pointer on additional configuration parameters, can be NULL.
 */
__weak
void BSP_AUDIO_IN_MspInit( void *Params )
{
    I2Sx_In_MspInit( );
}

/**
 * @brief  De-Initializes BSP_AUDIO_IN MSP.
 * @param  Params : pointer on additional configuration parameters, can be NULL.
 */
__weak
void BSP_AUDIO_IN_MspDeInit( void *Params )
{
    I2Sx_In_MspDeInit( );
}

/**
 * @brief  User callback when record buffer is filled.
 */
__weak
void BSP_AUDIO_IN_TransferComplete_CallBack( void )
{
    /* This function should be implemented by the user application.
     It is called into this driver when the current buffer is filled
     to prepare the next buffer pointer and its size. */
}

/**
 * @brief  Manages the DMA Half Transfer complete event.
 */
__weak
void BSP_AUDIO_IN_HalfTransfer_CallBack( void )
{
    /* This function should be implemented by the user application.
     It is called into this driver when the current buffer is filled
     to prepare the next buffer pointer and its size. */
}

/**
 * @brief  Audio IN Error callback function.
 */
__weak
void BSP_AUDIO_IN_Error_Callback( void )
{
    /* This function is called when an Interrupt due to transfer error on or peripheral
     error occurs. */
}
