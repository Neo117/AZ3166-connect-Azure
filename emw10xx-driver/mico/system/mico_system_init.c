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

#include "system_internal.h"

#ifdef AIRKISS_DISCOVERY_ENABLE
#include "easylink/airkiss_discovery.h"
#endif

#if MICO_WLAN_CONFIG_MODE == CONFIG_MODE_AWS
#include "alink_aws.h"
#endif


extern system_context_t* sys_context;
#ifndef  EasyLink_Needs_Reboot
static mico_worker_thread_t wlan_autoconf_worker_thread;
#endif


/******************************************************
 *               Variables Definitions
 ******************************************************/

static OSStatus system_config_mode_worker( void *arg )
{
    OSStatus err = kNoErr;
    system_context_t* in_context = system_context();
    require( in_context, exit );

    micoWlanPowerOn();
#if (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK) || \
    (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_SOFT_AP) ||  \
    (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_USER) ||  \
    (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP)
    err = system_easylink_start( in_context );
    require_noerr( err, exit );
#elif ( MICO_WLAN_CONFIG_MODE == CONFIG_MODE_WAC)
    err = system_easylink_wac_start( in_context );
    require_noerr( err, exit );
#elif ( MICO_WLAN_CONFIG_MODE == CONFIG_MODE_AWS)
    err = start_aws_config_mode( );
    require_noerr( err, exit );
#else
    #error "Wi-Fi configuration mode is not defined"
#endif
exit:
    return err;
}

OSStatus mico_system_wlan_start_autoconf( void )
{
  /* Enter auto-conf mode only once in reboot mode, use MICO_NETWORKING_WORKER_THREAD to save ram */
#ifdef  EasyLink_Needs_Reboot
    return mico_rtos_send_asynchronous_event( MICO_NETWORKING_WORKER_THREAD, system_config_mode_worker, NULL );
#else
    return mico_rtos_send_asynchronous_event( &wlan_autoconf_worker_thread, system_config_mode_worker, NULL );
#endif
}


OSStatus mico_system_init( mico_Context_t* in_context )
{
  OSStatus err = kNoErr;

  require_action( in_context, exit, err = kNotPreparedErr );

  /* Initialize mico notify system */
  err = system_notification_init( sys_context );
  require_noerr( err, exit ); 

#ifdef MICO_SYSTEM_MONITOR_ENABLE
  /* MiCO system monitor */
  err = mico_system_monitor_daemen_start( );
  require_noerr( err, exit ); 
#endif

#ifdef MICO_CLI_ENABLE
  /* MiCO command line interface */
  cli_init();
#endif

  /* Network PHY driver and tcp/ip static init */
  err = system_network_daemen_start( sys_context );
  require_noerr( err, exit ); 

#ifdef MICO_WLAN_CONNECTION_ENABLE
#ifndef  EasyLink_Needs_Reboot
  /* Create a worker thread for user handling wlan auto-conf event  */
  err = mico_rtos_create_worker_thread( &wlan_autoconf_worker_thread, MICO_APPLICATION_PRIORITY, 0x300, 1 );
  require_noerr_string( err, exit, "ERROR: Unable to start the autoconf worker thread." );
#endif

  if( sys_context->flashContentInRam.micoSystemConfig.configured == unConfigured){
#if (MICO_WLAN_CONFIG_MODE_TRIGGER) &&  (MICO_WLAN_CONFIG_MODE_TRIGGER != CONFIG_MODE_TRIGGER_AUTO )
    system_log("Empty configuration. Start configuration mode by external trigger");
    micoWlanPowerOff();
#else
    system_log("Empty configuration. Starting configuration mode...");
    err = mico_system_wlan_start_autoconf( );
    require_noerr( err, exit );
#endif
  }
#ifdef EasyLink_Needs_Reboot
  else if( sys_context->flashContentInRam.micoSystemConfig.configured == unConfigured2 ){
      system_log("Empty configuration. Starting configuration mode by external trigger");
      err = mico_system_wlan_start_autoconf( );
      require_noerr( err, exit );
  }
  else if( sys_context->flashContentInRam.micoSystemConfig.configured == wLanUnConfigured ){
      system_log("Re-config wlan configuration. Starting configuration mode...");
      err = mico_system_wlan_start_autoconf( );
      require_noerr( err, exit );
  }
#endif
#ifdef MFG_MODE_AUTO
  else if( sys_context->flashContentInRam.micoSystemConfig.configured == mfgConfigured ){
    system_log( "Enter MFG mode automatically" );
    mico_mfg_test( in_context );
    mico_thread_sleep( MICO_NEVER_TIMEOUT );
  }
#endif
  else{
    system_log("Available configuration. Starting Wi-Fi connection...");
    system_connect_wifi_fast( sys_context );
  }
#endif
  
  /* System discovery */
#ifdef MICO_SYSTEM_DISCOVERY_ENABLE
  system_discovery_init( sys_context );
#endif

  /*Local configuration server*/
#ifdef MICO_CONFIG_SERVER_ENABLE
  config_server_start( );
#endif
  
#ifdef AIRKISS_DISCOVERY_ENABLE
  err = airkiss_discovery_start( AIRKISS_APP_ID, AIRKISS_DEVICE_ID );
  require_noerr( err, exit );
#endif

exit:
  return err;
}


