/* EMW10xx implementation of NetworkInterfaceAPI
 * Copyright (c) 2015 ARM Limited
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


#include "EMW10xxInterface.h"
#include "emw10xx_lwip_stack.h"
#include "mico.h"

static bool _emw10xx_inited = false;

#define EMW10xx_DRIVER_INITED   if( _emw10xx_inited == false ) { \
                                    mico_board_init(); \
                                    mico_system_init( (mico_Context_t *)mico_system_context_init( 0 ) ); \
                                    mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)(&EMW10xxInterface::_wlan_status_cb_by_mico), this );\
                                    _emw10xx_inited = true;}


/* Interface implementation */
EMW10xxInterface::EMW10xxInterface(): _scan_res(0), _scan_sem(0), _scan_cnt(0), _interface(Station),
    _conn_sem(0), _is_sta_connected(false), _is_ap_connected(false)
{

}

int EMW10xxInterface::set_interface( wlan_if_t interface )
{
    _interface = interface;
    return NSAPI_ERROR_OK;
}

int EMW10xxInterface::connect(const char *ssid, const char *pass, nsapi_security_t security,
                                        uint8_t channel)
{
    if (channel != 0) {
        return NSAPI_ERROR_UNSUPPORTED;
    }

    set_credentials(ssid, pass, security);
    return connect();
}


void EMW10xxInterface::_wlan_status_cb_by_mico( WiFiEvent event, void *inContext )
{
    EMW10xxInterface *handler = (EMW10xxInterface*) inContext;
    handler->_wlan_status_cb( event );
}

void EMW10xxInterface::_wlan_status_cb( WiFiEvent event )
{
    switch ( event ) {
        case NOTIFY_STATION_UP:
            _conn_sem.release( );
            _is_sta_connected = true;
            break;
        case NOTIFY_STATION_DOWN:
            _is_sta_connected = false;
            break;
        case NOTIFY_AP_UP:
            _conn_sem.release( );
            _is_ap_connected = true;
            break;
        case NOTIFY_AP_DOWN:
            _is_ap_connected = false;
            break;
        default:
            break;
    }
    return;
}



int EMW10xxInterface::connect_ap( void )
{
    network_InitTypeDef_adv_st  wNetConfigAdv;

    EMW10xx_DRIVER_INITED;

    /* Initialize wlan parameters */
    memset( &wNetConfigAdv, 0x0, sizeof(wNetConfigAdv) );
    strcpy((char*)wNetConfigAdv.ap_info.ssid, ap_ssid);
    memcpy((char*)wNetConfigAdv.key, ap_pass, 64);
    wNetConfigAdv.key_len = ( ap_pass[63] != 0x0 )? 64:strlen(ap_pass);
    wNetConfigAdv.ap_info.security = SECURITY_TYPE_AUTO;
    wNetConfigAdv.ap_info.channel = ap_ch;
    wNetConfigAdv.dhcpMode = DHCP_Client;
    wNetConfigAdv.wifi_retry_interval = 100;

    /* Connect Now! */
    /* Register user function when wlan scan is completed */
    mbed_lwip_bringup();
    micoWlanStartAdv(&wNetConfigAdv);

    /* 10 seconds timeout */
    while( 0 != _conn_sem.wait(0) );
    _conn_sem.wait(20*1000);

    if( _is_sta_connected == true )
        return NSAPI_ERROR_OK;
    else{
        micoWlanSuspendStation();
        return NSAPI_ERROR_NO_CONNECTION;
    }
}

int EMW10xxInterface::establish_ap( void )
{
    EMW10xx_DRIVER_INITED;

    network_InitTypeDef_st wNetConfig;
    /* Initialize wlan parameters */
    memset( &wNetConfig, 0x0, sizeof(wNetConfig) );
    strcpy( (char*) wNetConfig.wifi_ssid, ap_ssid );
    strcpy( (char*) wNetConfig.wifi_key, ap_pass );
    wNetConfig.dhcpMode = DHCP_Server;
    wNetConfig.wifi_mode = Soft_AP;
    wNetConfig.wifi_retry_interval = 100;
    strcpy( (char*) wNetConfig.local_ip_addr, "192.168.0.1" );
    strcpy( (char*) wNetConfig.net_mask, "255.255.255.0" );
    strcpy( (char*) wNetConfig.dnsServer_ip_addr, "192.168.0.1" );
    /* Start Now! */
    micoWlanStart( &wNetConfig );

    /* 10 seconds timeout */
    while ( 0 != _conn_sem.wait( 0 ) );
    _conn_sem.wait( 20 * 1000 );

    if ( _is_ap_connected == true )
        return NSAPI_ERROR_OK;
    else {
        micoWlanSuspendSoftAP( );
        return NSAPI_ERROR_NO_CONNECTION;
    }
}

int EMW10xxInterface::connect()
{
    if ( _interface == Station ) return connect_ap();
    else if( _interface == Soft_AP ) return establish_ap();
    else return NSAPI_ERROR_NO_CONNECTION;
}

int EMW10xxInterface::set_credentials(const char *ssid, const char *pass, nsapi_security_t security)
{
    memset(ap_ssid, 0, sizeof(ap_ssid));
    strncpy(ap_ssid, ssid, sizeof(ap_ssid));

    memset(ap_pass, 0, sizeof(ap_pass));
    strncpy(ap_pass, pass, sizeof(ap_pass));

    ap_sec = security;

    return 0;
}

int EMW10xxInterface::set_channel(uint8_t channel)
{
    ap_ch = channel;
    return NSAPI_ERROR_OK;
}


int EMW10xxInterface::disconnect()
{
    EMW10xx_DRIVER_INITED;

    if( _interface == Station )
    {
        micoWlanSuspendStation();
        _is_sta_connected = false;
    }
    else if( _interface == Soft_AP )
    {
        micoWlanSuspendSoftAP();
        _is_ap_connected = false;
    }

    return NSAPI_ERROR_OK;
}

const char *EMW10xxInterface::get_ip_address()
{
    IPStatusTypedef outNetpara;
    if( _is_sta_connected == false ) return 0;
    micoWlanGetIPStatus(&outNetpara, Station);
    strncpy( _ip_address, outNetpara.ip, NSAPI_IPv4_SIZE);
    return _ip_address;
}

const char *EMW10xxInterface::get_mac_address( )
{
    EMW10xx_DRIVER_INITED;
    uint8_t mac_hex[6];
    mico_wlan_get_mac_address( mac_hex );
    sprintf(_mac, "%02x:%02x:%02x:%02x:%02x:%02x", mac_hex[0], mac_hex[1], mac_hex[2], mac_hex[3], mac_hex[4], mac_hex[5]);
    return _mac;
}

const char *EMW10xxInterface::get_gateway()
{
    IPStatusTypedef outNetpara;
    if( _is_sta_connected == false ) return 0;
    micoWlanGetIPStatus(&outNetpara, Station);
    strncpy( _gateway, outNetpara.gate, NSAPI_IPv4_SIZE);
    return _gateway;
}

const char *EMW10xxInterface::get_netmask()
{
    IPStatusTypedef outNetpara;
    if( _is_sta_connected == false ) return 0;
    micoWlanGetIPStatus(&outNetpara, Station);
    strncpy( _netmask, outNetpara.mask, NSAPI_IPv4_SIZE);
    return _netmask;
}

int8_t EMW10xxInterface::get_rssi()
{
    LinkStatusTypeDef link_status;
    if( _is_sta_connected == false ) return 0;

    micoWlanGetLinkStatus( &link_status );
    return link_status.signal_strength;
}


void EMW10xxInterface::_scan_complete_cb_by_mico( ScanResult_adv *pApList, void *inContext )
{
    EMW10xxInterface *handler = (EMW10xxInterface*) inContext;
    handler->_scan_complete_cb( pApList );
}

void EMW10xxInterface::_scan_complete_cb( ScanResult_adv *pApList )
{
    unsigned i = 0;
    nsapi_wifi_ap_t ap;

    if ( _scan_cnt != 0 ) {
        _scan_cnt = (pApList->ApNum > _scan_cnt) ? _scan_cnt : pApList->ApNum;
        for ( i = 0; i < _scan_cnt; i++ ) {
            memset( &ap, 0x0, sizeof(nsapi_wifi_ap_t));
            strncpy( ap.ssid, pApList->ApList[i].ssid, 32);
            memcpy( ap.bssid, pApList->ApList[i].bssid, 6 );
            ap.rssi = pApList->ApList[i].signal_strength;
            ap.channel = pApList->ApList[i].channel;

            switch( pApList->ApList[i].security ) {
                case SECURITY_TYPE_NONE:
                    ap.security = NSAPI_SECURITY_NONE;
                    break;
                case SECURITY_TYPE_WEP:
                    ap.security = NSAPI_SECURITY_WEP;
                    break;
                case SECURITY_TYPE_WPA_TKIP:
                case SECURITY_TYPE_WPA_AES:
                    ap.security = NSAPI_SECURITY_WPA;
                    break;
                case SECURITY_TYPE_WPA2_TKIP:
                case SECURITY_TYPE_WPA2_AES:
                case SECURITY_TYPE_WPA2_MIXED:
                    ap.security = NSAPI_SECURITY_WPA2;
                    break;
            }
            _scan_res[i] = WiFiAccessPoint(ap);
        }
    }
    else{
        _scan_cnt = pApList->ApNum;
    }
    _scan_sem.release( );
}

int EMW10xxInterface::scan(WiFiAccessPoint *res, unsigned count)
{
    EMW10xx_DRIVER_INITED;

    _scan_cnt = count;
    _scan_res = res;

    /* Register user function when wlan scan is completed */
    mico_system_notify_register( mico_notify_WIFI_SCAN_ADV_COMPLETED, (void *)(&EMW10xxInterface::_scan_complete_cb_by_mico), this );

    micoWlanStartScanAdv();
    _scan_sem.wait();

    mico_system_notify_remove( mico_notify_WIFI_SCAN_ADV_COMPLETED, (void *)(&EMW10xxInterface::_scan_complete_cb_by_mico) );
    return _scan_cnt;
}

NetworkStack *EMW10xxInterface::get_stack()
{
    EMW10xx_DRIVER_INITED;
    return nsapi_create_stack(&lwip_stack);
}
