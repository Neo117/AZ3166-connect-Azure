#include "mbed.h"
#include "mico.h"
#include "EMW10xxInterface.h"

#define TARGET_AZ3166

#ifdef TARGET_AZ3166

#include "oled.h"
#include "lps22hb.h"
#include "HTS221Sensor.h"
#include "lis2mdl_class.h"
#include "LSM6DSLSensor.h"
#include "stlogo.h"
#include "nau88c10.h"
#include "stm32f4xx_hal.h"
#include "audio_config.h"
#include "audio_op.h"

extern float hts221_humidity = 0;
extern float hts221_temp = 0;
extern int32_t p_data[3] = { 0 };
extern int32_t x_axes[3] = { 0 };
extern int32_t g_axes[3] = { 0 };

typedef enum
{
    JOY_NONE = 0,
    JOY_CENTER = 1,
    JOY_DOWN = 2,
    JOY_LEFT = 3,
    JOY_RIGHT = 4,
    JOY_UP = 5
} JOY_State_TypeDef;

// Serial pc( STDIO_UART_TX, STDIO_UART_RX, 115200 );

#define app_log(format, ...)  custom_log("MiCOKit_STmems", format, ##__VA_ARGS__)

DigitalOut led1( LED1 );
DigitalOut led2( LED2 );
DigitalOut led3( LED3 );

PwmOut rgb_r( RGB_R );
PwmOut rgb_g( RGB_G );
PwmOut rgb_b( RGB_B );

IRDA_HandleTypeDef IrdaHandle;

char page = 0;

typedef enum
{
    AUDIO_STATE_IDLE = 0,
    AUDIO_STATE_INIT,
    AUDIO_STATE_PLAYING,
} AUDIO_PLAYBACK_StateTypeDef;

static AUDIO_PLAYBACK_StateTypeDef audio_state;

uint16_t buf_tx_rx[2][0xFFF] = { 0 };
char step_one_complete = 0;
char step_two_complete = 0;
char print_log = 0;
char need_clear = 0;
char first_time = 1;

void USART3_IRQHandler( void )
{
    HAL_IRDA_IRQHandler( &IrdaHandle );
}

void HAL_IRDA_RxCpltCallback( IRDA_HandleTypeDef *hirda )
{
    uint8_t data;
    data = HAL_IRDA_Receive( hirda, &data, 1, 0 );
    led3 = !led3;
}

static OSStatus irda_init( )
{
    OSStatus err = kNoErr;

    pinmap_pinout( PB_10, PinMap_UART_TX );
    pin_mode( PB_10, PushPullNoPull );

    __HAL_RCC_USART3_CLK_ENABLE( );

    __HAL_RCC_USART3_FORCE_RESET( );
    __HAL_RCC_USART3_RELEASE_RESET( );

    IrdaHandle.Instance = USART3;

    IrdaHandle.Init.BaudRate = 38400;
    IrdaHandle.Init.WordLength = UART_WORDLENGTH_8B;
    IrdaHandle.Init.Parity = UART_PARITY_NONE;
    IrdaHandle.Init.Mode = UART_MODE_TX_RX;
    IrdaHandle.Init.Prescaler = 1;
    IrdaHandle.Init.IrDAMode = IRDA_POWERMODE_NORMAL;

    /* Enable and set I2Sx Interrupt to a lower priority */
    HAL_NVIC_SetPriority( USART3_IRQn, 0x0F, 0x00 );
    NVIC_SetVector( USART3_IRQn, (uint32_t) & USART3_IRQHandler );
    HAL_NVIC_EnableIRQ( USART3_IRQn );

    err = HAL_IRDA_Init( &IrdaHandle );
    require_noerr( err, exit );
    exit:
    return err;
}

void BUTTON_A_CB( )
{
    step_one_complete = 1;
    page = 1;
    need_clear = 1;

}

void BUTTON_B_CB( )
{
    step_two_complete = 1;
    page = 2;
    need_clear = 1;

}

void do_nothing()
{
    int i = 0;
    i++;
}

int app_audio( )
{
    OSStatus err = kNoErr;

    uint8_t cur_rgb = 0;
    uint8_t joy_status = JOY_NONE;


    float lps22hb_temp_data = 0;
    float lps22hb_pres_data = 0;
    char oled_show_line[OLED_DISPLAY_MAX_CHAR_PER_ROW + 1] = { '\0' };   // max char each line

    /*init HTS221*/
    DevI2C devI2c( I2C_SDA, I2C_SCL );
    HTS221Sensor hts221( devI2c );
    


 
    /*init Magnetometer*/
    LIS2MDL lis2mdl( devI2c );
    // int32_t p_data[3] = { 0 };
    lis2mdl.init(NULL);

    /*init Accelerometer*/
    LSM6DSLSensor lsm6dsl( devI2c, I2C_SDA, I2C_SCL );
    // int32_t x_axes[3] = { 0 };
    // int32_t g_axes[3] = { 0 };
    while(lsm6dsl.init( NULL ));
    lsm6dsl.enable_x( );
    lsm6dsl.enable_g( );


   /*init LPS22HB */
    err = lps25hb_sensor_init( );
    printf("error code is %d\r\n",err);
    require_noerr_string( err, exit, "ERROR: Init LPS22HB Error" );


for(int  i = 8; i > 0; i--){

  hts221.get_humidity( &hts221_humidity );
  hts221.get_temperature( &hts221_temp );
  printf("humidity = %f\r\n",hts221_humidity);
  printf("temp = %f\r\n",hts221_temp);

  lsm6dsl.get_x_axes( (int32_t*) x_axes );
  lsm6dsl.get_g_axes( (int32_t*) g_axes );

  lis2mdl.get_m_axes( p_data );

  printf ("magnet = %d, %d, %d\r\n", p_data[0], p_data[1], p_data[2]);
  printf( "LSM6DSL [acc/mg]: %d,%d,%d\r\n",x_axes[0],x_axes[1],x_axes[2] );
  printf( "LSM6DSL [gyro/mdps]: %d,%d,%d\r\n",g_axes[0],g_axes[1],g_axes[2] );

}
printf("return\r\n");
return 0;



    /*init Magnetometer*/
    // LIS2MDL lis2mdl( devI2c );
    // int32_t p_data[3] = { 0 };

    /*init Accelerometer*/
    // LSM6DSLSensor lsm6dsl( devI2c, I2C_SDA, I2C_SCL );
    // int32_t x_axes[3] = { 0 };
    // int32_t g_axes[3] = { 0 };

//     EMW10xxInterface wlan_blink;

//     /*button callback*/
//     InterruptIn _interrupt_BUTTON_A( USER_BUTTON_A );
//     InterruptIn _interrupt_BUTTON_B( USER_BUTTON_B );

//     _interrupt_BUTTON_A.rise( callback( &BUTTON_A_CB ) );
//     _interrupt_BUTTON_B.rise( callback( &BUTTON_B_CB ) );
//     _interrupt_BUTTON_B.fall( callback( &do_nothing ) );



//     rgb_r.period( 0.001 );
//     rgb_g.period( 0.001 );
//     rgb_b.period( 0.001 );

//     irda_init( );

//     // init OLED
//     OLED_Init( );
//     /*1.oled all light*/
//     OLED_FillAll( );
// //    Thread::wait( 2000 );
// //    OLED_Clear( );

//     /*init LPS22HB */
    // err = lps25hb_sensor_init( );
//     printf("error code is %d\r\n",err);
//     require_noerr_string( err, exit, "ERROR: Init LPS22HB Error" );

    // hts221.init( NULL );
    // hts221.enable( );
    
    // lis2mdl.init(NULL);

    // lsm6dsl.init( NULL );
    // lsm6dsl.enable_g( );
    // lsm6dsl.enable_x( );

//     /*init audio*/
//     audio_state = AUDIO_STATE_INIT;
//     BSP_AUDIO_IN_OUT_Init( OUTPUT_DEVICE_AUTO, I2S_DATAFORMAT_16B, I2S_AUDIOFREQ_8K );

//     /*record*/
//     audio_state = AUDIO_STATE_PLAYING;
//     BSP_AUDIO_In_Out_Transfer( buf_tx_rx[0], buf_tx_rx[1], 0xfff );

//     /*2.wifi scan */
//      while ( true )
//      {
//         if ( cur_rgb % 3 == 0 )
//             led1 = !led1;
//         if ( cur_rgb % 3 == 1 )
//             led2 = !led2;
//         if ( cur_rgb % 3 == 2 )
//             led3 = !led3;

//         rgb_r = (cur_rgb % 3 == 0) ? 0.5 : 0;
//         rgb_g = (cur_rgb % 3 == 1) ? 0.5 : 0;
//         rgb_b = (cur_rgb % 3 == 2) ? 0.5 : 0;

//         if(need_clear)
//         {
//             OLED_Clear();
//             need_clear = 0;
//         }
//         switch ( page )
//         {
//             case 0:
//                 snprintf( oled_show_line, OLED_DISPLAY_MAX_CHAR_PER_ROW + 1, "%s", MODEL );
//                 OLED_ShowString( OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_1, oled_show_line );

//                 err = lps25hb_Read_Data( &lps22hb_temp_data, &lps22hb_pres_data );
//                 require_noerr_string( err, exit, "ERROR: Can't Read LPS22HB Data" );
//                 sprintf( oled_show_line, "pre%.2fm", lps22hb_pres_data );
//                 OLED_ShowString( 0, OLED_DISPLAY_ROW_2, oled_show_line );
//                 /*acc glo eeprom*/
// //                snprintf( oled_show_line, OLED_DISPLAY_MAX_CHAR_PER_ROW + 1, "%s", MODEL );
// //                OLED_ShowString( OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_1, oled_show_line );
// //
               // lsm6dsl.get_x_axes( (int32_t*) x_axes );
               // lsm6dsl.get_g_axes( (int32_t*) g_axes );
// //                sprintf( oled_show_line, "acc%02d,%02d,%02d", x_axes[0], x_axes[1], x_axes[2] );
// //                OLED_ShowString( 0, OLED_DISPLAY_ROW_2, oled_show_line );
// //
// //                sprintf( oled_show_line, "glo%02d,%02d,%02d", g_axes[0], g_axes[1], g_axes[2] );
// //                OLED_ShowString( 0, OLED_DISPLAY_ROW_3, oled_show_line );

//                 break;
//             case 1:
//                 /*magnetometer pressure*/
//                 snprintf( oled_show_line, OLED_DISPLAY_MAX_CHAR_PER_ROW + 1, "%s", MODEL );
//                 OLED_ShowString( OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_1, oled_show_line );

                // lis2mdl.get_m_axes( p_data );
//                 sprintf( oled_show_line, "mag%02d,%02d,%02d", (int)p_data[0], (int)p_data[1], (int)p_data[2] );
//                 OLED_ShowString( 0, OLED_DISPLAY_ROW_2, oled_show_line );

                // err = lps25hb_Read_Data( &lps22hb_temp_data, &lps22hb_pres_data );
                // require_noerr_string( err, exit, "ERROR: Can't Read LPS22HB Data" );
//                 sprintf( oled_show_line, "pre%.2fm", lps22hb_pres_data );
//                 OLED_ShowString( 0, OLED_DISPLAY_ROW_3, oled_show_line );
//                 break;

//             case 2:
//                 /*hum temp*/
//                 hts221.get_humidity( &hts221_humidity );
//                 hts221.get_temperature( &hts221_temp );
//                 snprintf( oled_show_line, OLED_DISPLAY_MAX_CHAR_PER_ROW + 1, "%s", MODEL );
//                 OLED_ShowString( OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_1, oled_show_line );

//                 sprintf( oled_show_line, "%s", "hum&temp" );
//                 OLED_ShowString( 0, OLED_DISPLAY_ROW_2, oled_show_line );

//                 sprintf( oled_show_line, "%.2f%%,%.2fC", hts221_humidity, hts221_temp );
//                 OLED_ShowString( 0, OLED_DISPLAY_ROW_3, oled_show_line );
//                 break;
//             default:
//                 break;
//         }

//         if ( step_one_complete && step_two_complete )
//         {
//             step_one_complete = 0;
//             step_two_complete = 0;
//             print_log = 1;
//         }

//         if ( print_log &&first_time )
//         {
//             WiFiAccessPoint *ap;
//             int count = wlan_blink.scan( NULL, 0 );
//             /* Limit number of network arbitrary to 15 */
//             count = count < 15 ? count : 15;

//             ap = new WiFiAccessPoint[count];
//             count = wlan_blink.scan( ap, count );
//             for ( int i = 0; i < count; i++ )
//                 {
//                 printf( "SSID:%s,RSSI:%hhd\r\n",
//                         ap[i].get_ssid( ),
//                         ap[i].get_rssi( ) );
//             }

//             lsm6dsl.get_x_axes( (int32_t*) x_axes );
//             lsm6dsl.get_g_axes( (int32_t*) g_axes );
//             printf( "LSM6DSL [acc/mg]: %02d,%02d,%02d\r\n",x_axes[0],x_axes[1],x_axes[2] );
//             printf( "LSM6DSL [gyro/mdps]: %02d,%02d,%02d\r\n",g_axes[0],g_axes[1],g_axes[2] );

//             printf("magnetometer: %02d,%02d,%02d\r\n", (int)p_data[0],(int)p_data[1],(int)p_data[2]);
//             lps25hb_Read_Data( &lps22hb_temp_data, &lps22hb_pres_data );

//             printf("Pressure: %.2f\r\n", lps22hb_pres_data);
//             hts221.get_humidity( &hts221_humidity );
//             hts221.get_temperature( &hts221_temp );
//             printf("Humidity: %.2f%,%.2fC\r\n", hts221_humidity,hts221_temp);
//             printf("#");
//             fflush(stdout);
//             print_log = 0;
//             first_time = 0;
//         }

//         joy_status = cur_rgb % 5 + 1;
//         HAL_IRDA_Transmit( &IrdaHandle, &joy_status, 1, 100 );

//         Thread::wait( 500 );

//         cur_rgb++;
//     }

    exit:
     return 0;
}

static volatile char flag = 0;
void BSP_AUDIO_IN_TransferComplete_CallBack( void )
{
    if ( audio_state == AUDIO_STATE_PLAYING )
         {
        if ( 0 == flag ) {
            BSP_AUDIO_In_Out_Transfer( buf_tx_rx[1], buf_tx_rx[0], 0xfff );
            flag = 1;
        }
        else {
            BSP_AUDIO_In_Out_Transfer( buf_tx_rx[0], buf_tx_rx[1], 0xfff );
            flag = 0;
        }
    }
}

void BSP_AUDIO_OUT_TransferComplete_CallBack( void )
{

}

void BSP_AUDIO_OUT_Error_CallBack( void )
{

}

#else

DigitalOut led1(MBED_SYS_LED);
Serial pc(STDIO_UART_TX, STDIO_UART_RX, 115200);

#define os_helloworld_log(format, ...)  custom_log("helloworld", format, ##__VA_ARGS__)

mico_semaphore_t sem = NULL;

void easylink_pressed_callback(void *arg)
{
    mico_rtos_set_semaphore( &sem );
    mico_gpio_output_trigger( MICO_RF_LED );
}

/* MiCO GPIO IO driver demo */
int app_blink( )
{
    mico_gpio_initialize( MICO_RF_LED, OUTPUT_PUSH_PULL );
    mico_gpio_initialize( EasyLink_BUTTON, INPUT_PULL_UP );

    mico_gpio_enable_irq( EasyLink_BUTTON, IRQ_TRIGGER_FALLING_EDGE, easylink_pressed_callback, NULL );

    mico_rtos_init_semaphore( &sem, 1 );

    pc.printf( "Helloworld\r\n" );

    while ( true ) {
        if ( kNoErr != mico_rtos_get_semaphore( &sem, 5000 ) ) {
            pc.printf( "Get semaphore timeout\r\n" );
        }
        else {
            pc.printf(" Get semaphore success\r\n");
            led1 = !led1;
        }
    }
}
#endif

