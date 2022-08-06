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



char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pin_rows[ROW_NUM] = {19, 18, 5, 17};    // GIOP19, GIOP18, GIOP5, GIOP17 connect to the row pins
byte pin_column[COLUMN_NUM] = {16, 4, 0, 2}; // GIOP16, GIOP4, GIOP0, GIOP2 connect to the column pins


// The below are the variables, which can be changed

// wifi & mqttt variable 
String wifiSSID="dma-gulshan2.4";
String wifiPassword="dmabd987";
String mqttBroker = "dma.com.bd";
unsigned long wifi_interval = 30000;
const char* topic_to_publish = "DMA/door_gateway";
const char* topic_to_subscribe = "DMA/door_server";
char* mqtt_command;


// pushbutton variables
int lastButtonState;    // the previous state of button
int currentButtonState; // the current state of button
bool pushbutton_pressed = false;
unsigned long previousMillis = 0;
int button_pressing_time = 5000; //milli seconds

// keypad variables
char* input_password;
// int input_otp;
bool keypad_input = false;
bool new_message = 0;
char msg[MAX_MSG_SIZE];
char input_otp[MAX_OTP_SIZE];
int otp_length = 4;
int key_count = 0;

// objects
WiFiClient client;
PubSubClient mqtt(client);
EVENT badge_event;
BLE myBLE;
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );


// functions
void connectWifi();
void connect_mqtt();
void keypad_control(char* input_password);
void on_message(char *topic, byte *payload, unsigned int length);
void mqtt_reconnect();
// unsigned long getKeypadIntegerMulti();
void executeCommand(char* command);
void door_open();
void door_close();
void wifi_reconnect();
void IRAM_ATTR buttonPressed();



void setup()
{   
    Serial.begin(9600);
    connectWifi();
    mqtt.setServer(mqttBroker.c_str(), 1883);
    mqtt.setCallback(on_message);// Initialize the callback routine

    pinMode(pushButton_pin, INPUT_PULLUP); // set ESP32 pin to input pull-up mode
    pinMode(RELAY_PIN, OUTPUT);
	  attachInterrupt(digitalPinToInterrupt(pushButton_pin), buttonPressed, CHANGE);

}

void loop()
{
  if(buttonPressed)
    {
      door_open();
      delay(DOORTIME);
      door_close();
      pushbutton_pressed = false;
    }
  if(keypad_input)
  {
    if(key_count == otp_length)
    {
      input_otp[key_count] = '\0';
      key_count = 0;
      keypad_input = false;
      //send input otp to mqtt
      // mqtt.publish(topic_to_publish,input_otp);
    }
    else
    {
      char c = keypad.getKey();
      if (c >= '0' && c <= '9')
      {
        input_otp[key_count++] = c;
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
  String serialized_json = badge_event.createJson(myBLE.foundDevices,esp_mac);
  
  if(serialized_json.length() > 0)
  {
    mqtt.publish(topic_to_publish, serialized_json.c_str());
  }

  mqtt.loop();
  wifi_reconnect();
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
void connect_mqtt()
{
    while (!mqtt.connected())
    {
        Serial.println("Connecting with MQTT...");
        if (mqtt.connect(CLIENTID))
        {
            mqtt.subscribe(topic_to_subscribe);
            // mqtt.subscribe("workforce/pub");
        }
    }
}
// Reconnect to client
void mqtt_reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(CLIENTID)) {
      Serial.println("connected");
      Serial.print("Publishing to: ");
      Serial.println(topic_to_publish);
      Serial.println('\n');

    } else {
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void wifi_reconnect() {
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=wifi_interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
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
    mqtt.publish(topic_to_publish, input_otp);

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
void on_message(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic_to_subscribe);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)mqtt_command[i]);
    messageTemp += (char)mqtt_command[i];
  }
  Serial.println();
  new_message = true;
}

void IRAM_ATTR buttonPressed()
{
  if(!digitalRead(pushbutton_pressed))
  {
    previousMillis = millis();
  }
  else if(millis() - previousMillis >= button_pressing_time)
  {
    pushbutton_pressed = true;
  }
  else
  {
    pushbutton_pressed = false;
  }
}