#include <WiFi.h>
#include "config.h"
#include "secrets.h"
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

// The onboard Neopixel is useful in confirming proper start-up and
// conneciton to WiFi, and also optionally (via the USE_NEOPIXEL define
// in config.h) in debugging remote command processing
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixel(1,PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);


/*
 * Home Assistant YAML configuration for each light
  light:
    - name: "Signal Tower Red"
      object_id: "signaltower_01_red"
      state_topic: "site/signaltower/red/status"
      command_topic: "site/signaltower/red/switch"
      unique_id: "GENERATE-UNIQUE-UUID-HERE"
      optimistic: false

    - name: "Signal Tower Yellow"
      object_id: "signaltower_01_yellow"
      state_topic: "site/signaltower/yellow/status"
      command_topic: "site/signaltower/yellow/switch"
      unique_id: "GENERATE-UNIQUE-UUID-HERE"
      optimistic: false

    - name: "Signal Tower Green"
      object_id: "signaltower_01_green"
      state_topic: "site/signaltower/green/status"
      command_topic: "site/signaltower/green/switch"
      unique_id: "GENERATE-UNIQUE-UUID-HERE"
      optimistic: false

    - name: "Signal Tower Blue"
      object_id: "signaltower_01_blue"
      state_topic: "site/signaltower/blue/status"
      command_topic: "site/signaltower/blue/switch"
      unique_id: "GENERATE-UNIQUE-UUID-HERE"
      optimistic: false
*/

#define LAMPPIN_RED 33
#define LAMPPIN_YELLOW 15
#define LAMPPIN_GREEN 32
#define LAMPPIN_BLUE  14

#ifdef DWEET
  extern void post_dweet(uint8_t tower_state, String message);
  long timeLastDweet = -(dweetInterval * 1000);  // Set to trigger on first loop()
#endif 

// MQTT uses WiFiClient class to create TCP connections
WiFiClient client;

// Control and state reporting topics for each light
#define RED_STATE_TOPIC      "deerfield/signaltower/red/status"
#define RED_COMMAND_TOPIC    "deerfield/signaltower/red/switch"
#define GREEN_STATE_TOPIC    "deerfield/signaltower/green/status"
#define GREEN_COMMAND_TOPIC  "deerfield/signaltower/green/switch"
#define BLUE_STATE_TOPIC     "deerfield/signaltower/blue/status"
#define BLUE_COMMAND_TOPIC   "deerfield/signaltower/blue/switch"
#define YELLOW_STATE_TOPIC   "deerfield/signaltower/yellow/status"
#define YELLOW_COMMAND_TOPIC "deerfield/signaltower/yellow/switch"

const char* LIGHT_ON = "ON";
const char* LIGHT_OFF = "OFF";

// Masks for each light's status bits in the overall status indicator
uint8_t tower_state;
uint8_t red_mask    = 0b1000;
uint8_t yellow_mask = 0b0100;
uint8_t green_mask  = 0b0010;
uint8_t blue_mask   = 0b0001;

// Separate status and command topics for the overall tower (though
// not part of Home Assistant integration).
#define PUBLISH_TOPIC   "deerfield/signaltower/tower/status"
#define SUBSCRIBE_TOPIC "deerfield/signaltower/tower/command"

// Create the MQTT client object used for publish and subscribe, binding it to the
// appropriate MQTT broker as defined in secrets.h
Adafruit_MQTT_Client ha_mqtt(&client, MQTT_BROKER, MQTT_PORT, DEVICE_ID, MQTT_USER, MQTT_PASS);
Adafruit_MQTT_Publish   towerPublish   = Adafruit_MQTT_Publish(&ha_mqtt,PUBLISH_TOPIC); 
Adafruit_MQTT_Subscribe towerCommand   = Adafruit_MQTT_Subscribe(&ha_mqtt,SUBSCRIBE_TOPIC);
Adafruit_MQTT_Subscribe redCommand     = Adafruit_MQTT_Subscribe(&ha_mqtt,RED_COMMAND_TOPIC);
Adafruit_MQTT_Publish   redStatePub    = Adafruit_MQTT_Publish(&ha_mqtt,RED_STATE_TOPIC);
Adafruit_MQTT_Subscribe greenCommand   = Adafruit_MQTT_Subscribe(&ha_mqtt,GREEN_COMMAND_TOPIC);
Adafruit_MQTT_Publish   greenStatePub  = Adafruit_MQTT_Publish(&ha_mqtt,GREEN_STATE_TOPIC);
Adafruit_MQTT_Subscribe blueCommand    = Adafruit_MQTT_Subscribe(&ha_mqtt,BLUE_COMMAND_TOPIC);
Adafruit_MQTT_Publish   blueStatePub   = Adafruit_MQTT_Publish(&ha_mqtt,BLUE_STATE_TOPIC);
Adafruit_MQTT_Subscribe yellowCommand  = Adafruit_MQTT_Subscribe(&ha_mqtt,YELLOW_COMMAND_TOPIC);
Adafruit_MQTT_Publish   yellowStatePub = Adafruit_MQTT_Publish(&ha_mqtt,YELLOW_STATE_TOPIC);

// Callback to be invoked when we the Command subscription receives a
// message.  Will be registered via the MQTT_Subscribe object on the
// appropriate topic.
// Command value passed in is an integer with the low order four bits indicating light
// status in accordance with the mask values defined above.
void towercmdcallback(char *data, uint16_t len) {
  Serial.print("New command received, value (string) is: ");
  Serial.println(data);

  // Set all lignts based on the command data value
  processCommand(atoi(data));

  // Publish the new light state info (for all four lights) using the same Command syntax
  Serial.println("Publishing new command");
  towerPublish.publish(String(tower_state).c_str());  // Use actual state, not just data received
  #ifdef DWEET
    // Dweet status update, given things changed
      post_dweet(tower_state, "Tower command");
      timeLastDweet = millis();  // Reset dweet heartbeat timing
  #endif
}
void redcmdcallback(char *data, uint16_t len) {
  Serial.print("New Red command received, value (string) is: ");
  Serial.println(data);

  if(strcmp(data,LIGHT_ON) == 0) {
    redOn(true);  // And publish any status change
  }
  else if(strcmp(data,LIGHT_OFF) == 0) {
    redOff(true); // And publish any status change
  }
}

void greencmdcallback(char *data, uint16_t len) {
  Serial.print("New Green command received, value (string) is: ");
  Serial.println(data);

  if(strcmp(data,LIGHT_ON) == 0) {
    greenOn(true);  // And publish any status change
  }
  else if(strcmp(data,LIGHT_OFF) == 0) {
    greenOff(true);  // And publish any status change
  }
}


void bluecmdcallback(char *data, uint16_t len) {
  Serial.print("New Blue command received, value (string) is: ");
  Serial.println(data);

  if(strcmp(data,LIGHT_ON) == 0) {
    blueOn(true);  // And publish any status change
  }
  else if(strcmp(data,LIGHT_OFF) == 0) {
    blueOff(true);  // And publish any status change
  }
}

void yellowcmdcallback(char *data, uint16_t len) {
  Serial.print("New Yellow command received, value (string) is: ");
  Serial.println(data);

  if(strcmp(data,LIGHT_ON) == 0) {
    yellowOn(true);  // And publish any status change
  }
  else if(strcmp(data,LIGHT_OFF) == 0) {
    yellowOff(true);  // And publish any status change
  }
}

void setup() {

  Serial.begin(115200);
  // wait for serial port to open
  while (!Serial) {
    delay(10);
  }
 
  // Enable LED output pins
  pinMode(LAMPPIN_RED,OUTPUT);
  pinMode(LAMPPIN_YELLOW,OUTPUT);
  pinMode(LAMPPIN_GREEN,OUTPUT);
  pinMode(LAMPPIN_BLUE,OUTPUT);


  // Initialize the Neopixel and set it to red at startup
  pixel.begin();
  pixel.setBrightness(50);  // About 1/5 maximum (0-255);
  pixel.clear(); pixel.setPixelColor(0,pixel.Color(50,0,0)); pixel.show();

  // Connect to WiFi so we can pub/sub with MQTT
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());

  // Green to indicate WiFi connected
  pixel.clear(); pixel.setPixelColor(0,pixel.Color(0,50,0)); pixel.show();

  // Enable command notification callback, and
  // subscribe to receive those notifications.  
  // Must configure before connecting to MQTT Broker.
  towerCommand.setCallback(towercmdcallback);   ha_mqtt.subscribe(&towerCommand);
  redCommand.setCallback(redcmdcallback);       ha_mqtt.subscribe(&redCommand);
  greenCommand.setCallback(greencmdcallback);   ha_mqtt.subscribe(&greenCommand);
  blueCommand.setCallback(bluecmdcallback);     ha_mqtt.subscribe(&blueCommand);
  yellowCommand.setCallback(yellowcmdcallback); ha_mqtt.subscribe(&yellowCommand);

  // Start up lamp sequence
  processCommand(0b0000);  delay(500);  tower_state = 0;  // Initialized all OFF
  processCommand(0b0001);  delay(500);
  processCommand(0b0010);  delay(500);
  processCommand(0b0100);  delay(500);
  processCommand(0b1000);  delay(500);
  processCommand(0b1111);  // All lights on
  // Report all lights on (start-up state)
  redStatePub.publish(LIGHT_ON);
  greenStatePub.publish(LIGHT_ON);
  blueStatePub.publish(LIGHT_ON);
  yellowStatePub.publish(LIGHT_ON);

  // Report initialized and online (with start-up state)
  publishUpdate("Initialized!");

  Serial.println("Processing MQTT commands");
  // Turn Neopixel off (it may get used later, though)
  pixel.clear(); pixel.setPixelColor(0,pixel.Color(0,0,0)); pixel.show();
}

uint32_t pubcnt = 0;

void loop() {

  unsigned long timeCurrent = millis();
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // Sit in a tight loop processing MQTT subscriptions for a specified number
  // of milliseconds, calling callbacks for any received.
  ha_mqtt.processPackets(10000);

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  if(! ha_mqtt.ping()) {
    ha_mqtt.disconnect();
  }

  // See if we need to emit a Dweet heartbeat
  #ifdef DWEET
    timeCurrent = millis();
    if( (timeCurrent - timeLastDweet) > (dweetInterval*1000) ) {
      post_dweet(tower_state, "Dweet heartbeat");
      timeLastDweet = timeCurrent;  // Reset dweet heartbeat timing
    }
  #endif
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (ha_mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = ha_mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(ha_mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 10 seconds...");
       ha_mqtt.disconnect();
       delay(10000);  // wait 10 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

// Interpret a command for the entire tower (all four lights) received via MQTT
// Currently just turns lights on and off using low four bits, where
// 0 = off and 1 = on. Does not publish status changes, allowing that to be
// handled seperately as appropriate
void processCommand(int command)
{
  if(command & red_mask) {
    redOn(false); 
  }
  else {
    redOff(false);
  }
  if(command & yellow_mask) {
    yellowOn(false);
  }
  else {
    yellowOff(false);
  }
  if(command & green_mask) {
    greenOn(false);
  }
  else {
    greenOff(false);
  }
  if(command & blue_mask) {
    blueOn(false);
  }
  else {
    blueOff(false);
  }
}

// Handle everything associated with turning the red light on
void redOn(bool publish)
{
  digitalWrite(LAMPPIN_RED,HIGH);  // Turn the light 
  #ifdef USE_NEOPIXEL
    pixel.clear(); pixel.setPixelColor(0,pixel.Color(150,0,0)); pixel.show();
  #endif

  Serial.println("Red = ON");
  redStatePub.publish(LIGHT_ON);

  tower_state |= red_mask;   // Set red lamp indicator bit
  if(publish == true) {
    publishUpdate("Red on command");
  }
}
void redOff(bool publish)
{
  digitalWrite(LAMPPIN_RED,LOW);
  Serial.println("Red = OFF");
  redStatePub.publish(LIGHT_OFF);
  
  tower_state &= ~red_mask;  // Clear red lamp indicator bit
  if(publish == true) {
    publishUpdate("Red off command");
  }
}

void greenOn(bool publish)
{
  digitalWrite(LAMPPIN_GREEN,HIGH);
  #ifdef USE_NEOPIXEL
    pixel.clear(); pixel.setPixelColor(0,pixel.Color(0,150,0)); pixel.show();
  #endif

  Serial.println("Green = ON");
  greenStatePub.publish(LIGHT_ON);

  tower_state |= green_mask;  // Set green lamp indicator bit
  if(publish == true) {
    publishUpdate("Green on command");
  }
}

void greenOff(bool publish)
{
  digitalWrite(LAMPPIN_GREEN,LOW);
  Serial.println("Green = OFF");
  greenStatePub.publish(LIGHT_OFF);

  tower_state &= ~green_mask; // Clear green lamp indicator bit
  if(publish == true) {
    publishUpdate("Green off command");
  }
}

void blueOn(bool publish)
{
  digitalWrite(LAMPPIN_BLUE,HIGH);
  #ifdef USE_NEOPIXEL
    pixel.clear(); pixel.setPixelColor(0,pixel.Color(0,0,150)); pixel.show();
  #endif
  
  Serial.println("Blue = ON");
  blueStatePub.publish(LIGHT_ON);

  tower_state |= blue_mask;  // Set blue lamp indicator bit
  if(publish == true) {
    publishUpdate("Blue on command");
  }
}

void blueOff(bool publish)
{
  digitalWrite(LAMPPIN_BLUE,LOW);
  Serial.println("Blue = OFF");
  blueStatePub.publish(LIGHT_OFF);

  tower_state &= ~blue_mask;  // Clear blue lamp indicator bit
  if(publish == true) {
    publishUpdate("Blue off command");
  }
}

void yellowOn(bool publish)
{
  digitalWrite(LAMPPIN_YELLOW,HIGH);
  #ifdef USE_NEOPIXEL
    pixel.clear(); pixel.setPixelColor(0,pixel.Color(150,150,0)); pixel.show();
  #endif

  Serial.println("Yellow = ON");
  yellowStatePub.publish(LIGHT_ON);

  tower_state |= yellow_mask;  // Set yellow lamp indicator bit
  if(publish == true) {
    publishUpdate("Yellow on command");
  }
}

void yellowOff(bool publish)
{
  digitalWrite(LAMPPIN_YELLOW,LOW);
  Serial.println("Yellow = OFF");
  yellowStatePub.publish(LIGHT_OFF);

  tower_state &= ~yellow_mask;  // Clear yellow lamp indicator bit
  if(publish == true) {
    publishUpdate("Yellow off command");
  }
}

// Convenience function to publish updated tower state, minimally to 
// MQTT but also possibly to Dweet
void publishUpdate(String message)
{
    towerPublish.publish(String(tower_state).c_str());
    #ifdef DWEET
      // Dweet status update, given things changed
      post_dweet(tower_state, message);
      timeLastDweet = millis();  // Reset dweet heartbeat timing
    #endif
}