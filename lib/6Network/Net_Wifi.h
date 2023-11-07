// resources:
// https://docs.espressif.com/projects/esp-idf/en/v4.3-beta2/esp32/api-guides/wifi.html
// https://espressif-docs.readthedocs-hosted.com/projects/esp-faq/en/latest/software-framework/wifi.html

#ifdef ESP32
    #include <WiFi.h>
    #include <esp_wifi.h>
#else
    #include <ESP8266WiFi.h>
    #include <WiFiClient.h>
#endif

class Net_Wifi: public Loggable {
    char hostName[21] = "fakeHostName";

    void setHostName() {
        #ifdef ESP32
            WiFi.setHostname(hostName);
        #else
            WiFi.hostname(hostName);
        #endif
    }
    
    void beginWifi(const char* ssid, const char* passw) {
        bool error = String(ssid).isEmpty() || String(passw).isEmpty();
        xLogStatus(__func__, !error);

        // ESP.eraseConfig();
        WiFi.persistent(false);
        WiFi.disconnect();
        WiFi.mode(WIFI_AP_STA);
        startAP(false);

        if (!error) {
            WiFi.begin(ssid, passw);
        }
    }
    
    public:
        Net_Wifi(): Loggable("Wifi") {} 

        bool isConnected() { return WiFi.status() == WL_CONNECTED; }

        String localIp() { 
            return isConnected() ? WiFi.localIP().toString() : "0.0.0.0"; 
        }

        const char* getHostName() { return hostName; }

        void setup(const char* ssid, const char* passw) {
            uint8_t mac[6];
            WiFi.macAddress(mac);
            sprintf(hostName, "B_%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

            xLog(__func__, hostName);
            setHostName();
            beginWifi(ssid, passw);
        }

        void startAP(bool forceReset, int channel = 0) {
            xLog(__func__);

            if (forceReset) {
                WiFi.persistent(false);
                WiFi.disconnect();
                WiFi.mode(WIFI_AP_STA);
            //    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));     // required for captive portal
            } 

            if (channel == 0) {
                WiFi.softAP(hostName, "11223344");
            } else {
                WiFi.softAP(hostName, "11223344", channel, true);
            }

            xLog("hostName", hostName);
        }

        void setTxPower(uint16_t power) {
            #ifdef ESP32
                WiFi.setTxPower(WIFI_POWER_19_5dBm);
            #else 
                WiFi.setOutputPower(power);
            #endif
        }
};



