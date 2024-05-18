/*
  Project:      realtime_co2
  Description:  write sensor data to DWEET
*/

#include "Arduino.h"

// hardware and internet configuration parameters
#include "config.h"
#include "secrets.h"
// private credentials for network, MQTT, weather provider
// #include "secrets.h"

#ifdef DWEET
  #include <HTTPClient.h> 

  // Post a dweet to report the overal tower state (all lights, in bitmask form) along
  // with a message that provides some context
  void post_dweet(uint8_t tower_state, String message)
  {
    WiFiClient dweet_client;

    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("Lost network connection to " + String(WIFI_SSID) + "!");
      return;
    }

    // Use our WiFiClient to connect to dweet
    if (!dweet_client.connect(DWEET_HOST, 80)) {
      Serial.println("Dweet connection failed!");
      return;
    }

    // Get battery voltage so we can post that too
    float battery_voltage = analogReadMilliVolts(A13);
    battery_voltage *= 2;    // we divided by 2, so multiply back
    battery_voltage /= 1000; // convert to volts!

    // Transmit Dweet as HTTP post with a data payload as JSON
    String postdata = "{\"rssi\":\""    + String(WiFi.RSSI())                + "\"," +
                          "\"ipaddr\":\"" + WiFi.localIP().toString()        + "\"," +
                          "\"battery_voltage\":\"" + String(battery_voltage) + "\"," + 
                          "\"message\":\"" + message                         + "\"," +
                          "\"towerstate\":\""   + String(tower_state)        + "\"}";


    // Note that the dweet device 'name' gets set here, is needed to fetch values
    dweet_client.println("POST /dweet/for/" + String(DWEET_DEVICE) + " HTTP/1.1");
    dweet_client.println("Host: dweet.io");
    dweet_client.println("User-Agent: ESP32/ESP8266 (orangemoose)/1.0");
    dweet_client.println("Cache-Control: no-cache");
    dweet_client.println("Content-Type: application/json");
    dweet_client.print("Content-Length: ");
    dweet_client.println(postdata.length());
    dweet_client.println();
    dweet_client.println(postdata);
    Serial.println("Dweet POST:");
    Serial.println(postdata);

    delay(1500);  

    // Read all the lines of the reply from server (if any) and print them to Serial Monitor
    #ifdef DEBUG
      Serial.println("Dweet server response:");
      while(dweet_client.available()){
        String line = dweet_client.readStringUntil('\r');
        Serial.println(line);
      }
      Serial.println("-----");
    #endif
    
    // Close client connection to dweet server
    dweet_client.stop();
  }
#endif