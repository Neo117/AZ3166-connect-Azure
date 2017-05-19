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
#include "StringUtils.h"

#include "system.h"

#if (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK) || (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_SOFT_AP) ||  \
    (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_USER) ||  (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP)

// EasyLink Soft AP mode, HTTP configuration message define
#define kEasyLinkURLAuth          "/auth-setup"

/* Internal vars and functions */
static mico_semaphore_t easylink_sem; /**< Used to suspend thread while easylink. */
static mico_semaphore_t easylink_connect_sem; /**< Used to suspend thread while connection. */
static bool easylink_success = false; /**< true: connect to wlan, false: start soft ap mode or roll back to previous settings */
static uint32_t easylinkIndentifier = 0; /**< Unique for an easylink instance. */
static mico_thread_t easylink_thread_handler = NULL;
static bool easylink_thread_force_exit = false;

/* Perform easylink and connect to wlan */
static void easylink_thread( uint32_t inContext );

#if ( MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK ) || (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP)
static mico_config_source_t source = CONFIG_BY_NONE;
static uint8_t airkiss_random = 0x0;
static void airkiss_broadcast_thread( uint32_t arg );
#endif

/* MiCO callback when WiFi status is changed */
static void EasyLinkNotify_WifiStatusHandler( WiFiEvent event, system_context_t * const inContext )
{
    system_log_trace();
    require( inContext, exit );

    switch ( event )
    {
        case NOTIFY_STATION_UP:
            /* Connected to AP, means that the wlan configuration is right, update configuration in flash and update
             bongjour txt record with new "easylinkIndentifier" */
#if defined MICO_COMPLETE_SYSTEM && (MICO_COMPLETE_SYSTEM == 1)
            mico_easylink_bonjour_update( Station, inContext );
#endif
            inContext->flashContentInRam.micoSystemConfig.configured = allConfigured;
            mico_system_context_update( &inContext->flashContentInRam ); //Update Flash content
            mico_rtos_set_semaphore( &easylink_connect_sem ); //Notify Easylink thread
            break;
        case NOTIFY_AP_DOWN:
            /* Remove bonjour service under soft ap interface */
#if defined MICO_COMPLETE_SYSTEM && (MICO_COMPLETE_SYSTEM == 1)
            mdns_suspend_record( "_easylink_config._tcp.local.", Soft_AP, true );
#endif
            break;
        default:
            break;
    }
    exit:
    return;
}

#if ( MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK ) || (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP)
/* MiCO callback when EasyLink is finished step 1, return SSID and KEY */
static void EasyLinkNotify_EasyLinkCompleteHandler( network_InitTypeDef_st *nwkpara, system_context_t * const inContext )
{
    system_log_trace();
    OSStatus err = kNoErr;

    require_action( inContext, exit, err = kParamErr );
    require_action( nwkpara, exit, err = kTimeoutErr );

    /* Store SSID and KEY*/
    mico_rtos_lock_mutex( &inContext->flashContentInRam_mutex );
    memcpy( inContext->flashContentInRam.micoSystemConfig.ssid, nwkpara->wifi_ssid, maxSsidLen );
    memset( inContext->flashContentInRam.micoSystemConfig.bssid, 0x0, 6 );
    memcpy( inContext->flashContentInRam.micoSystemConfig.user_key, nwkpara->wifi_key, maxKeyLen );
    inContext->flashContentInRam.micoSystemConfig.user_keyLength = strlen( nwkpara->wifi_key );
    inContext->flashContentInRam.micoSystemConfig.dhcpEnable = true;
    mico_rtos_unlock_mutex( &inContext->flashContentInRam_mutex );
    system_log("Get SSID: %s, Key: %s", inContext->flashContentInRam.micoSystemConfig.ssid, inContext->flashContentInRam.micoSystemConfig.user_key);

    source = (mico_config_source_t) nwkpara->wifi_retry_interval;
    exit:
    if ( err != kNoErr )
    {
        /*EasyLink timeout or error*/
        system_log("EasyLink step 1 ERROR, err: %d", err);
        easylink_success = false;
        mico_rtos_set_semaphore( &easylink_sem );
    }
    return;
}

/* MiCO callback when EasyLink is finished step 2, return extra data 
 data format: AuthData#Identifier(localIp/netMask/gateWay/dnsServer)
 Auth data: Provide to application, application will decide if this is a proter configuration for currnet device
 Identifier: Unique id for every easylink instance send by easylink mobile app
 localIp/netMask/gateWay/dnsServer: Device static ip address, use DHCP if not exist
 */
static void EasyLinkNotify_EasyLinkGetExtraDataHandler( int datalen, char* data,
                                                        system_context_t * const inContext )
{
    system_log_trace();
    OSStatus err = kNoErr;
    int index;
    uint32_t *identifier, ipInfoCount;
    char *debugString;
    struct in_addr ipv4_addr;

    require_action( inContext, exit, err = kParamErr );

    if ( source == CONFIG_BY_AIRKISS )
    {
        airkiss_random = *data;
        goto exit;
    }

    debugString = DataToHexStringWithSpaces( (const uint8_t *) data, datalen );
    system_log("Get user info: %s", debugString);
    free( debugString );

    /* Find '#' that seperate anthdata and identifier*/
    for ( index = datalen - 1; index >= 0; index-- )
    {
        if ( data[index] == '#' && ((datalen - index) == 5 || (datalen - index) == 25) )
            break;
    }
    require_action( index >= 0, exit, err = kParamErr );

    /* Check auth data by device */
    data[index++] = 0x0;
    err = mico_system_delegate_config_recv_auth_data( data );
    require_noerr( err, exit );

    /* Read identifier */
    identifier = (uint32_t *) &data[index];
    easylinkIndentifier = *identifier;

    /* Identifier: 1 x uint32_t or Identifier/localIp/netMask/gateWay/dnsServer: 5 x uint32_t */
    ipInfoCount = (datalen - index) / sizeof(uint32_t);
    require_action( ipInfoCount >= 1, exit, err = kParamErr );

    mico_rtos_lock_mutex( &inContext->flashContentInRam_mutex );

    if ( ipInfoCount == 1 )
    { //Use DHCP to obtain local ip address
        inContext->flashContentInRam.micoSystemConfig.dhcpEnable = true;
        system_log("Get auth info: %s, EasyLink identifier: %lx", data, easylinkIndentifier);
    } else
    { //Use static ip address
        inContext->flashContentInRam.micoSystemConfig.dhcpEnable = false;
        ipv4_addr.s_addr = *(identifier+1);
        strcpy( (char *) inContext->micoStatus.localIp, inet_ntoa( ipv4_addr ) );
        ipv4_addr.s_addr = *(identifier+2);
        strcpy( (char *) inContext->micoStatus.netMask, inet_ntoa( ipv4_addr ) );
        ipv4_addr.s_addr = *(identifier+3);
        strcpy( (char *) inContext->micoStatus.gateWay, inet_ntoa( ipv4_addr ) );
        ipv4_addr.s_addr = *(identifier+4);
        strcpy( (char *) inContext->micoStatus.dnsServer, inet_ntoa( ipv4_addr ) );

        system_log("Get auth info: %s, EasyLink identifier: %lx, local IP info:%s %s %s %s ", data, easylinkIndentifier, inContext->flashContentInRam.micoSystemConfig.localIp,
            inContext->flashContentInRam.micoSystemConfig.netMask, inContext->flashContentInRam.micoSystemConfig.gateWay,inContext->flashContentInRam.micoSystemConfig.dnsServer);
    }
    mico_rtos_unlock_mutex( &inContext->flashContentInRam_mutex );
    source = CONFIG_BY_EASYLINK_V2;

    exit:
    if ( err != kNoErr )
    {
        /*EasyLink error*/
        system_log("EasyLink step 2 ERROR, err: %d", err);
        easylink_success = false;
    } else
        /* Easylink success after step 1 and step 2 */
        easylink_success = true;

    mico_rtos_set_semaphore( &easylink_sem );
    return;
}
#endif

OSStatus system_easylink_start( system_context_t * const inContext )
{
    system_log_trace();
    OSStatus err = kUnknownErr;
#if defined MICO_COMPLETE_SYSTEM && (MICO_COMPLETE_SYSTEM == 1)
    mdns_record_state_t easylink_service_state = mdns_get_record_status( "_easylink_config._tcp.local.", Station);

    /* Remove previous _easylink_config service if existed */
    if ( easylink_service_state != RECORD_REMOVED )
    {
        UnSetTimer( remove_bonjour_for_easylink );
        remove_bonjour_for_easylink( );
        mico_rtos_delay_milliseconds( 1500 );
    }

#if (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP)
    /* Start config server */
    err = config_server_start( );
    require_noerr( err, exit );
#endif
#endif

    /* Start easylink thread, existed? stop and re-start! */
    if( easylink_thread_handler )
    {
        system_log("EasyLink processing, force stop..");
        easylink_thread_force_exit = true;
        if( easylink_sem ) mico_rtos_set_semaphore( &easylink_sem );
        if( easylink_connect_sem ) mico_rtos_set_semaphore( &easylink_connect_sem );
        //mico_rtos_thread_force_awake( &easylink_thread_handler );
        mico_rtos_thread_join( &easylink_thread_handler );
    }

    err = mico_rtos_create_thread( &easylink_thread_handler, MICO_APPLICATION_PRIORITY, "EASYLINK", easylink_thread,
                                   0x1000, (mico_thread_arg_t) inContext );
    require_noerr_string( err, exit, "ERROR: Unable to start the EasyLink thread." );

    /* Make sure easylink thread is running */
    mico_rtos_delay_milliseconds(20);

exit:
    return err;
}

void easylink_thread( uint32_t inContext )
{
    system_log_trace();
    MAY_BE_UNUSED OSStatus err = kNoErr;
    MAY_BE_UNUSED system_context_t *Context = (system_context_t *) inContext;
#if ( MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP ) || (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_SOFT_AP)
    network_InitTypeDef_st wNetConfig;
    easylinkIndentifier = 0x0;
    easylink_success = false;
#endif
    easylink_thread_force_exit = false;

#if ( MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK ) || (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP)
    source = CONFIG_BY_NONE;
    mico_system_notify_register( mico_notify_EASYLINK_WPS_COMPLETED,
                                 (void *) EasyLinkNotify_EasyLinkCompleteHandler,
                                 (void *) inContext );
    mico_system_notify_register( mico_notify_EASYLINK_GET_EXTRA_DATA,
                                 (void *) EasyLinkNotify_EasyLinkGetExtraDataHandler,
                                 (void *) inContext );
#endif
    mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED,
                                 (void *) EasyLinkNotify_WifiStatusHandler, (void *) inContext );

    mico_rtos_init_semaphore( &easylink_sem, 1 );
    mico_rtos_init_semaphore( &easylink_connect_sem, 1 );


#if ( MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK ) || (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP)
    /* Skip Easylink mode */
    if ( Context->flashContentInRam.micoSystemConfig.easyLinkByPass == EASYLINK_BYPASS )
    {
        Context->flashContentInRam.micoSystemConfig.easyLinkByPass = EASYLINK_BYPASS_NO;
        mico_system_context_update( &Context->flashContentInRam );
        system_connect_wifi_fast( Context );
        goto exit;
    }
    /* If use CONFIG_MODE_SOFT_AP only, skip easylink mode, establish soft ap directly */
restart:
    mico_system_delegate_config_will_start( );
    system_log("Start easylink combo mode");
    micoWlanStartEasyLinkPlus( EasyLink_TimeOut / 1000 );
    while( mico_rtos_get_semaphore( &easylink_sem, 0 ) == kNoErr );
    err = mico_rtos_get_semaphore( &easylink_sem, MICO_WAIT_FOREVER );

    /* Easylink force exit by user, clean and exit */
    if( easylink_thread_force_exit )
    {
        system_log("EasyLink waiting for terminate");
        micoWlanStopEasyLinkPlus( );
        mico_rtos_get_semaphore( &easylink_sem, 3000 );
        system_log("EasyLink canceled by user");
        easylink_thread_force_exit = false;
        goto exit;
    }

    /* EasyLink Success */
    if ( easylink_success == true )
    {
        mico_system_delegate_config_recv_ssid( Context->flashContentInRam.micoSystemConfig.ssid,
                                               Context->flashContentInRam.micoSystemConfig.user_key );
        system_connect_wifi_normal( Context );

        /* Wait for station connection */
        while( mico_rtos_get_semaphore( &easylink_connect_sem, 0 ) == kNoErr );
        err = mico_rtos_get_semaphore( &easylink_connect_sem, EasyLink_ConnectWlan_Timeout );
        /* Easylink force exit by user, clean and exit */
        if( easylink_thread_force_exit )
        {
            micoWlanSuspend();
            system_log("EasyLink connection canceled by user");
            easylink_thread_force_exit = false;
            goto exit;
        }

        /*SSID or Password is not correct, module cannot connect to wlan, so restart EasyLink again*/
        require_noerr_action_string( err, restart, micoWlanSuspend(), "Re-start easylink commbo mode" );
        mico_system_delegate_config_success( source );

#if defined MICO_COMPLETE_SYSTEM && (MICO_COMPLETE_SYSTEM == 1)
        /* Start bonjour service for new device discovery */
        err = mico_easylink_bonjour_start( Station, Context );
        require_noerr( err, exit );
        SetTimer( 60 * 1000, remove_bonjour_for_easylink );
#endif
        if ( source == CONFIG_BY_AIRKISS )
        {
            err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "AIRKISS", airkiss_broadcast_thread, 0x800, 0 );
            require_noerr_string( err, exit, "ERROR: Unable to start the EasyLink thread." );
        }

        goto exit;
    }
#endif
    /* EasyLink failed or disabled*/
    if ( easylink_success == false )
    {
#if defined MICO_COMPLETE_SYSTEM && (MICO_COMPLETE_SYSTEM == 1)
#if ( MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP ) || (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_SOFT_AP) //start soft ap mode 
        micoWlanSuspend();
        mico_thread_msleep( 20 );

        mico_system_delegate_soft_ap_will_start( );

        memset( &wNetConfig, 0, sizeof(network_InitTypeDef_st) );
        wNetConfig.wifi_mode = Soft_AP;
        snprintf( wNetConfig.wifi_ssid, 32, "EasyLink_%c%c%c%c%c%c", Context->micoStatus.mac[9],
                  Context->micoStatus.mac[10],
                  Context->micoStatus.mac[12],
                  Context->micoStatus.mac[13],
                  Context->micoStatus.mac[15],
                  Context->micoStatus.mac[16] );
        strcpy( (char*) wNetConfig.wifi_key, "" );
        strcpy( (char*) wNetConfig.local_ip_addr, "10.10.10.1" );
        strcpy( (char*) wNetConfig.net_mask, "255.255.255.0" );
        strcpy( (char*) wNetConfig.gateway_ip_addr, "10.10.10.1" );
        wNetConfig.dhcpMode = DHCP_Server;
        micoWlanStart( &wNetConfig );
        system_log("Establish soft ap: %s.....", wNetConfig.wifi_ssid);

        /* Start bonjour service for device discovery under soft ap mode */
        err = mico_easylink_bonjour_start( Soft_AP, Context );
        require_noerr( err, exit );

        /* Wait for station connection */
        while( mico_rtos_get_semaphore( &easylink_connect_sem, 0 ) == kNoErr );
        err = mico_rtos_get_semaphore( &easylink_connect_sem, MICO_WAIT_FOREVER );
        /* Easylink force exit by user, clean and exit */
        if( err != kNoErr && easylink_thread_force_exit )
        {
            micoWlanSuspend();
            system_log("EasyLink connection canceled by user");
            easylink_thread_force_exit = false;
            goto exit;
        }

        /* Start bonjour service for easylink mode */
        err = mico_easylink_bonjour_start( Station, Context );
        require_noerr( err, exit );
        SetTimer( 60 * 1000, remove_bonjour_for_easylink );

        mico_system_delegate_config_success( CONFIG_BY_SOFT_AP );

#elif ( MICO_WLAN_CONFIG_MODE == CONFIG_MODE_USER) //Connect wlan by user defined method
        system_log("Start easylink user mode");
        mico_system_delegate_config_will_start( );

        /* Developer should save the ssid/key to system_context_t and connect to AP.
         * NOTIFY_STATION_UP event will save the new ssid, key to flash and wake up the easylink routine */
        while( mico_rtos_get_semaphore( &easylink_connect_sem, 0 ) == kNoErr );
        err = mico_rtos_get_semaphore( &easylink_connect_sem, MICO_WAIT_FOREVER );

        /* Easylink force exit by user, clean and exit */
        if( err != kNoErr && easylink_thread_force_exit )
        {
            micoWlanSuspend();
            system_log("EasyLink connection canceled by user");
            easylink_thread_force_exit = false;
            goto exit;
        }

        /* Start bonjour service for easylink mode */
        err = mico_easylink_bonjour_start( Station, Context );
        require_noerr( err, exit );
        SetTimer( 60 * 1000, remove_bonjour_for_easylink );

        mico_system_delegate_config_success( CONFIG_BY_USER );
#else // CONFIG_MODE_EASYLINK mode
#ifdef EasyLink_Needs_Reboot
        /*so roll back to unconfig mode if easylink is triggered by external */
        if( Context->flashContentInRam.micoSystemConfig.configured == unConfigured2 )
        {
            Context->flashContentInRam.micoSystemConfig.configured = unConfigured;
            mico_system_context_update( &Context->flashContentInRam );
        }

#endif

        /*so roll back to previous settings  (if it has) and connect*/
        if(Context->flashContentInRam.micoSystemConfig.configured != unConfigured)
        {
            MICOReadConfiguration( Context );
#ifdef EasyLink_Needs_Reboot
            Context->flashContentInRam.micoSystemConfig.configured = allConfigured;
            mico_system_context_update( &Context->flashContentInRam );
#endif
            system_connect_wifi_normal( Context );
        } else
        {
            /*module should power down in default setting*/
            system_log("Wi-Fi power off");
            micoWlanPowerOff();
        }
#endif
#else // MICO_COMPLETE_SYSTEM && (MICO_COMPLETE_SYSTEM == 1)

#ifdef EasyLink_Needs_Reboot
        /*so roll back to unconfig mode if easylink is triggered by external */
        if( Context->flashContentInRam.micoSystemConfig.configured == unConfigured2 )
        {
            Context->flashContentInRam.micoSystemConfig.configured = unConfigured;
            mico_system_context_update( &Context->flashContentInRam );
        }

#endif

        /*so roll back to previous settings  (if it has) and connect*/
        if(Context->flashContentInRam.micoSystemConfig.configured != unConfigured)
        {
            MICOReadConfiguration( Context );
#ifdef EasyLink_Needs_Reboot
            Context->flashContentInRam.micoSystemConfig.configured = allConfigured;
            mico_system_context_update( &Context->flashContentInRam );
#endif
            system_connect_wifi_normal( Context );
        } else
        {
            /*module should power down in default setting*/
            system_log("Wi-Fi power off");
            micoWlanPowerOff();
        }

#endif
    }

exit:
    mico_system_delegate_config_will_stop( );

    mico_system_notify_remove( mico_notify_WIFI_STATUS_CHANGED,
                               (void *) EasyLinkNotify_WifiStatusHandler );
#if ( MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK ) || (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP)
    mico_system_notify_remove( mico_notify_EASYLINK_WPS_COMPLETED,
                               (void *) EasyLinkNotify_EasyLinkCompleteHandler );
    mico_system_notify_remove( mico_notify_EASYLINK_GET_EXTRA_DATA,
                               (void *) EasyLinkNotify_EasyLinkGetExtraDataHandler );
#endif

#if defined MICO_COMPLETE_SYSTEM && (MICO_COMPLETE_SYSTEM == 1)
#ifndef MICO_CONFIG_SERVER_ENABLE
    err = config_server_stop( );
    require_noerr( err, exit );
#endif 
#endif

    mico_rtos_deinit_semaphore( &easylink_sem );
    mico_rtos_deinit_semaphore( &easylink_connect_sem );
    easylink_thread_handler = NULL;
    mico_rtos_delete_thread( NULL );
}

#if ( MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK ) || (MICO_WLAN_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP)
void airkiss_broadcast_thread( uint32_t arg )
{
    OSStatus err = kNoErr;
    int fd;
    struct sockaddr_in addr;
    int i = 0;

    fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    require_action( IsValidSocket( fd ), exit, err = kNoResourcesErr );

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr =htonl(INADDR_BROADCAST);
    addr.sin_port = htons(10000);

    system_log( "Send AirKiss ack to WECHAT" );

    while ( 1 )
    {
        sendto( fd, &airkiss_random, 1, 0, (struct sockaddr *)&addr, sizeof(addr) );
        mico_thread_msleep( 100 );
        i++;
        if ( i > 20 )
            break;
    }
    exit:
    if ( err != kNoErr ){
        system_log( "thread exit with err: %d", err );
    }
    close( fd );
    mico_rtos_delete_thread( NULL );
}
#endif


#endif

