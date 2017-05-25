# How to use sensor&audio demo

Sensor&audio demo shows how to control st sensors and using I2S interface to record and play pcm stream.

## main components

### OLED Screen

4 line oled screen，you can use codes as follows to control it.

```c
 OLED_Init( );
 OLED_FillAll( );
 OLED_Clear( );
 OLED_ShowString( OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_1, oled_show_line );
```
### ST Sensors

#### hts221

A Humidity and Temperature sensor with I2C interfcae,can indicate hum and temp.

use HTS221Sensor class to control it.

``````c
DevI2C devI2c( I2C_SDA, I2C_SCL );
HTS221Sensor hts221( devI2c );
hts221.init( NULL );
hts221.enable( );
hts221.get_humidity( &hts221_humidity );
hts221.get_temperature( &hts221_temp );
``````

#### lis2mdl

A Magnetometer sensor with I2C interface, high performance 3-axis magnetometer.

use LIS2MDL class to control it.

```c
LIS2MDL lis2mdl( devI2c );
lis2mdl.get_m_axes( p_data );
```
#### lps22hb

The LPS22HB is an ultra-compact piezoresistive absolute pressure sensor which functions as a digital output barometer. 

use functions like follows

```c
lps25hb_sensor_init( );
lps25hb_Read_Data( &lps22hb_temp_data, &lps22hb_pres_data );
```

#### lsm6dsl

The LSM6DSL is a system-in-package featuring a 3D digital accelerometer and a 3D digital gyroscope sensor.

use LSM6DSLSensor class to control it.

```c
LSM6DSLSensor lsm6dsl( devI2c, I2C_SDA, I2C_SCL );
lsm6dsl.init( NULL );
lsm6dsl.enable_g( );
lsm6dsl.enable_x( );
lsm6dsl.get_x_axes( (int32_t*) x_axes );
lsm6dsl.get_g_axes( (int32_t*) g_axes );
```

### CODEC NAU88C10

The NAU88C10 is a cost effective low power wideband Monophonic audio CODEC.

In this example, we implemented the function of the edge recording using I2S interface.

First,use bsp_audio_in_out_init to init audio device.

```c
BSP_AUDIO_IN_OUT_Init( OUTPUT_DEVICE_AUTO, I2S_DATAFORMAT_16B, I2S_AUDIOFREQ_8K );
```

Then with BSP_AUDIO_In_Out_Transfer function ,we can transfer two buffers to transfer and receive data from MCU.

```c
BSP_AUDIO_In_Out_Transfer( buf_tx_rx[0], buf_tx_rx[1], 0xfff );
```

### IRDA

We also provide irda transmit in this demo.You can use the following function to control it.

```c
irda_init( );
HAL_IRDA_Transmit( &IrdaHandle, &joy_status, 1, 100 );
```



