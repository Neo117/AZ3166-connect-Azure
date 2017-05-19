# Build EMW10xx module wlan driver library 
 

    mbed compile -m MK3166 -t GCC_ARM --source=emw10xx-driver/wlan/ --library --profile=emw10xx-driver/wlan/profiles/develop.json 

    mbed compile -m MK3239 -t GCC_ARM --source=emw10xx-driver/wlan/ --library --profile=emw10xx-driver/wlan/profiles/develop.json 