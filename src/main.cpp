#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "EVENT.h"
#include "BLE.h"
#include <string>
#include <algorithm>
#include <Keypad.h>


#define pushButton_pin   33// ESP32 pin GIOP22 connected to button's pin


#define RELAY_PIN   27 // ESP32 pin GIOP27 connected to relay's pin
#define ROW_NUM     4  // four rows
#define COLUMN_NUM  4  // four columns
#define DOORTIME  30000 // the time door remains open after open command
#define CLIENTID "123456789"
#define MAX_MSG_SIZE    100
#define MAX_OTP_SIZE    20

// The below are variables, which can be changed
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
int lastButtonState;    // the previous state of button
int currentButtonState; // the current state of button


byte pin_rows[ROW_NUM] = {19, 18, 5, 17};    // GIOP19, GIOP18, GIOP5, GIOP17 connect to the row pins
byte pin_column[COLUMN_NUM] = {16, 4, 0, 2}; // GIOP16, GIOP4, GIOP0, GIOP2 connect to the column pins


String wifiSSID="dma-gulshan2.4";
String wifiPassword="dmabd987";
String mqttBroker = "dma.com.bd";


WiFiClient client;
PubSubClient mqtt(client);
const char* topic_to_publish = "DMA/door_gateway";
const char* topic_to_subscribe = "DMA/door_server";
// const char* topic_to_publish = "workforce/pub";
EVENT badge_event;
BLE myBLE;
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

char* input_password;
int input_otp;
bool keypad_input = false;
bool new_message = 0;
char msg[MAX_MSG_SIZE];
char otp[MAX_OTP_SIZE];
int otp_length = 4;
int key_count = 0;

void connectWifi();
void connect_mqtt();
void keypad_control(char* input_password);
void on_message(char *topic, byte *payload, unsigned int length);
void reconnect();
unsigned long getKeypadIntegerMulti();
void executeCommand(char* command);
void door_open();
void door_close();
void IRAM_ATTR button_open();



void setup()
{   
    Serial.begin(9600);
    connectWifi();
    mqtt.setServer(mqttBroker.c_str(), 1883);
    mqtt.setCallback(on_message);// Initialize the callback routine

    pinMode(pushButton_pin, INPUT_PULLUP); // set ESP32 pin to input pull-up mode
    pinMode(RELAY_PIN, OUTPUT);



}

void loop()
{

      // debouncing



  if(keypad_input)
  {
    if(key_count == otp_length)
    {
      otp[key_count] = '\0';
      key_count = 0;
      keypad_input = false;
      //send input otp to mqtt
    }
    else
    {
      char c = keypad.getKey();
      if (c >= '0' && c <= '9')
      {
        otp[key_count++] = c;
      }
       
    }
  }

  if(new_message)
  {
    new_message = false;
    executeCommand(msg);
    msg[0] = '\0';
  }

  
  if (!mqtt.connected())
  {
      connect_mqtt();
      Serial.println("MQTT Connected");
      mqtt.publish(topic_to_publish, "ESP 32 Online!");
  }
  

  myBLE.Scan();
  String esp_mac = WiFi.macAddress();
//   String dev_name = esp_mac.erase(remove(esp_mac.begin(), esp_mac.end(), 'A'), esp_mac.end()); 
  String serialized_json = badge_event.createJson(myBLE.foundDevices,esp_mac);
  
  if(serialized_json.length() > 0)
  {
    mqtt.publish(topic_to_publish, serialized_json.c_str());
  }

  mqtt.loop();

  // delay(1000);
}


void connectWifi()
{
    Serial.println("Connecting To Wifi");
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println("Wifi Connected");
    Serial.println(WiFi.SSID());
    Serial.println(WiFi.RSSI());
    Serial.println(WiFi.macAddress());
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.gatewayIP());
    Serial.println(WiFi.dnsIP());
}

void executeCommand(char* command)
{
  if(!strcmp(command, "Open"))
  {
    door_open();
  }
  else if(!strcmp(command, "Close"))
  {
    door_close();
  }
  else if(!strcmp(strncpy(NULL, command, 6), "Keypad"))
  {
    char length[MAX_OTP_SIZE];
    strncpy(length, command + 7, sizeof(command));
    otp_length = atoi(length);
    keypad_input = true;
    mqtt.publish(topic_to_publish, otp);

  }
  else
  {
    Serial.println("Invalid Command");
  }
}
void door_open()
{
  digitalWrite(RELAY_PIN, LOW); 
  delay(DOORTIME);
}
void door_close()
{
  digitalWrite(RELAY_PIN, HIGH); 
}


