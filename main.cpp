#include "mbed.h"
#include "mico.h"
//#include "NTPClient.h"
//#include "easy-connect.h"
#include "iothub_client_sample_mqtt.h"
#include "TCPSocket.h"
#include "EMW10xxInterface.h"


#include "TCPServer.h"
#include "TCPSocket.h"

#define HTTP_STATUS_LINE "HTTP/1.0 200 OK"
#define HTTP_HEADER_FIELDS "Content-Type: text/html; charset=utf-8"
#define HTTP_MESSAGE_BODY ""                                     \
"<html>" "\r\n"                                                  \
"  <body style=\"display:flex;text-align:center\">" "\r\n"       \
"    <div style=\"margin:auto\">" "\r\n"                         \
"      <h1>Hello World</h1>" "\r\n"                              \
"      <p>It works !</p>" "\r\n"                                 \
"    </div>" "\r\n"                                              \
"  </body>" "\r\n"                                               \
"</html>"

#define HTTP_RESPONSE HTTP_STATUS_LINE "\r\n"   \
                      HTTP_HEADER_FIELDS "\r\n" \
                      "\r\n"                    \
                      HTTP_MESSAGE_BODY "\r\n"


#define RUN_APPLICATION( app )     extern int app_##app(void);  app_##app();
Serial pc(USBTX, USBRX, 9600);
AnalogIn ain(A3);
PwmOut pout(D9);

NetworkInterface* network;
EMW10xxInterface wlan;

extern float hts221_humidity;
extern float hts221_temp;
extern int32_t p_data[3];
extern int32_t x_axes[3];
extern int32_t g_axes[3];

int main( void )
{
    /* APPLICATION can be assigned to the folder names under folder "APP" */
    //RUN_APPLICATION( iperf );
    //RUN_APPLICATION( blink );
    //RUN_APPLICATION( mbed_wifi );
    //RUN_APPLICATION( mbed_tls_client );

    printf("Start testing\n");

    RUN_APPLICATION( audio );

    // printf("-----------------------hts221_humidity%f",hts221_humidity);
    
    iothub_client_sample_mqtt_run();
    //network = easy_connect(true); /* has 1 argument, enable_logging (pass in true to log to serial port) */
    /* if (!network)
    {
        printf("Connecting to the network failed... See serial output.\r\n");
        return 1;
    }
    else
    {
        printf("Wifi connected successfully\n");
    }*/

//     printf( "start soft ap!\r\n" );

//     wlan.set_interface( Station );
//     int ret = wlan.connect( "mxchip-offices", "88888888", NSAPI_SECURITY_WPA_WPA2, 0 );

//     if ( ret != NSAPI_ERROR_OK ) {
//         printf("Soft station creation failed\r\n");
//         return -1;
//     }

//     printf("Success\r\n\r\n");
//     printf("MAC: %s\r\n", wlan.get_mac_address());
//     printf("IP: %s\r\n", wlan.get_ip_address());
//     printf("Netmask: %s\r\n", wlan.get_netmask());
//     printf("Gateway: %s\r\n", wlan.get_gateway());
//     printf("RSSI: %d\r\n\r\n", wlan.get_rssi());


// printf("The target IP address is '%s'\n", wlan.get_ip_address());
    
    // TCPServer srv;
    // TCPSocket clt_sock;
    // SocketAddress clt_addr;
    
    // /* Open the server on ethernet stack */
    // srv.open(&wlan);
    
    //  Bind the HTTP port (TCP 80) to the server 
    // srv.bind(wlan.get_ip_address(), 80);
    
    // /* Can handle 5 simultaneous connections */
    // srv.listen(5);
    
    // while (true) {
    //     srv.accept(&clt_sock, &clt_addr);
    //     printf("accept %s:%d\n", clt_addr.get_ip_address(), clt_addr.get_port());
    //     clt_sock.send(HTTP_RESPONSE, strlen(HTTP_RESPONSE));
    // }
    // printf("accept Success\n");

    // iothub_client_sample_mqtt_run();

	// while(1)
	// {


 //       wait(1);
 //       float f=ain.read();
 //       printf("value is: %f\n",f);
	// }
	
    /*while (true)
    {
         NTPClient ntp(*network);
         ntp.setTime("0.pool.ntp.org");
         wait(1);
    }*/

    return 0;
}

