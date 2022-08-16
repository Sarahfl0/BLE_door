#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "EVENT.h"
#include "BLE.h"
#include <string>
#include <algorithm>
#include <Keypad.h>

#define pushButton_pin 33 // ESP32 pin GIOP22 connected to button's pin
#define RELAY_PIN  BUILTIN_LED//13	  // ESP32 pin GIOP27 connected to relay's pin
#define ROW_NUM 4		  // four rows
#define COLUMN_NUM 4	  // four columns
#define DOORTIME_OPEN_INTERVAL 	 1
#define DOORTIME_CLOSE_INTERVAL 	5
#define DOOR_KEEP_OPEN_TIME			30000
#define STEPS				1
#define CLIENTID "nhjfanjknjkefnjjhfuhawiuhu"
#define MAX_MSG_SIZE 100
#define MAX_OTP_SIZE 20

#define MAX_PWM_WRITE		255

char keys[ROW_NUM][COLUMN_NUM] = {
	{'1', '2', '3', 'A'},
	{'4', '5', '6', 'B'},
	{'7', '8', '9', 'C'},
	{'*', '0', '#', 'D'}};

byte pin_rows[ROW_NUM]      = {21, 19, 18, 5}; // GIOP21, GIOP19, GIOP18, GIOP5  connect to the row pins
byte pin_column[COLUMN_NUM] = {17, 16, 4, 0};   // GIOP17, GIOP16, GIOP4, GIOP0 connect to the column pins
char *subscription_message;
uint8_t subscription_message_length = 0;

// The below are the variables, which can be changed

// wifi & mqttt variable
// String wifiSSID="dma-gulshan2.4";
// String wifiPassword="dmabd987";
String wifiSSID = "Faiza";
String wifiPassword = "faizafaiza";
String mqttBroker = "broker.hivemq.com";
unsigned long wifi_interval = 30000;
const char *topic_to_publish = "DMA/BLE/ESP_PUB";
// const char* topic_to_publish = "DMA/door_server";
const char *topic_to_subscribe = "DMA/BLE/SUB_";
char *mqtt_command;

// pushbutton variables
int lastButtonState;	// the previous state of button
int currentButtonState; // the current state of button
bool pushbutton_pressed = false;
unsigned long previousMillis = 0;
int button_pressing_time = 5000; // milli seconds

// keypad variables
String input_otp;
String otp_msg;
bool keypad_input = false;
bool new_message = 0;
char msg[MAX_MSG_SIZE];
// bool door_status = 0;
// char input_otp[MAX_OTP_SIZE];

int otp_length = 4;
int key_count = 0;

// objects
WiFiClient client;
PubSubClient mqtt(client);
EVENT badge_event;
BLE myBLE;
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// functions
void connectWifi();
void connect_mqtt();
void on_message(char *topic, byte *payload, unsigned int length);
void mqtt_reconnect();
void clear_input_otp();
// unsigned long getKeypadIntegerMulti();
void executeCommand(char *command);
void door_open();
void door_close();
void wifi_reconnect();
void IRAM_ATTR buttonPressed();

void setup()
{
	Serial.begin(9600);
	input_otp.reserve(32);
	connectWifi();
	// connect_mqtt();
	mqtt.setServer(mqttBroker.c_str(), 1883);
	mqtt.setBufferSize(512);
	mqtt.setCallback(on_message); // Initialize the callback routine

	pinMode(pushButton_pin, INPUT_PULLUP); // set ESP32 pin to input pull-up mode
	pinMode(RELAY_PIN, OUTPUT);
	attachInterrupt(digitalPinToInterrupt(pushButton_pin), buttonPressed, CHANGE);
	subscription_message = NULL;
	door_close();
}

void loop()
{

	if (!mqtt.connected())
	{
		connect_mqtt();
		Serial.println("MQTT Connected");
		mqtt.publish(topic_to_publish, "ESP 32 Online!");
	}
	myBLE.Scan();
	String esp_mac = WiFi.macAddress();
	String serialized_json = badge_event.createJson(myBLE.foundDevices, esp_mac);
	// Serial.println(serialized_json);
	// Serial.println(serialized_json.length());

	if (serialized_json.length() > 0)
	{
		if (mqtt.publish(topic_to_publish, serialized_json.c_str()))
		{
			Serial.println("MQTT pub successful");
		}
		else
		{
			Serial.println("MQTT pub unsuccessful");
		}
	}
	if(pushbutton_pressed)
	  {
	    door_open();
	    // delay(DOORTIME_INTERVAL);
	    door_close();
	    pushbutton_pressed = false;
	  }

	// bool take_input = true;
	while (keypad_input)
	{
		char key = keypad.getKey();

		if (key) 
		{
			Serial.println(key);
			if (key == '*') 
			{
				clear_input_otp();
				// reset the input password variable
			}
			else if ((key_count+1) == otp_length) {
				input_otp += key;
				key_count = key_count+1 ;		
				Serial.print("inserted input_otp: ");
				Serial.println(input_otp);
				otp_msg ="Verify " + input_otp;
				mqtt.publish(topic_to_publish,otp_msg.c_str());
				keypad_input = false;
				clear_input_otp();
				// myBLE.Scan();

				// if the length of the given otp matches the declared otp length, send the otp to the server
			} 
			else 
			{
				if (key >= '0' && key <= '9')
				{
					input_otp += key;
					key_count = key_count+1 ;
					Serial.print("key_count: ");
					Serial.println(key_count);
					Serial.print("input_otp: ");
					Serial.println(input_otp);
										
				}
				// append new character to input password string
			}
		}

	}

	// {
	// 	if (key_count == otp_length)
	// 	{
	// 		// input_otp[key_count] = '\0';
	// 		key_count = 0;
	// 		keypad_input = false;
	// 		// send input otp to mqtt
	// 		Serial.println(input_otp);
	// 		mqtt.publish(topic_to_publish,input_otp);            
	// 	}
	// 	else
	// 	{
	// 		char c = keypad.getKey();
	// 		Serial.println(c);
	// 		if (c >= '0' && c <= '9')
	// 		{
	// 			input_otp[key_count++] = c;
	// 		}
	// 	}
	// }

	if (new_message)
	{
		new_message = false;
		Serial.print("sub message: ");
		Serial.println(msg);
		Serial.println("****************");
		executeCommand(msg);
		msg[0] = '\0';
		subscription_message_length = 0;
	}

	mqtt.loop();
	// wifi_reconnect();
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
void mqtt_reconnect()
{
	// Loop until we're reconnected
	while (!client.connected())
	{
		Serial.print("Attempting MQTT connection...");
		// Attempt to connect
		if (mqtt.connect(CLIENTID))
		{
			Serial.println("MQTT reconnected");
			Serial.print("Publishing to: ");
			Serial.println(topic_to_publish);
			Serial.println('\n');
		}
		else
		{
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}
void wifi_reconnect()
{
	unsigned long currentMillis = millis();
	// if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
	if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= wifi_interval))
	{
		Serial.print(millis());
		Serial.println("Reconnecting to WiFi...");
		WiFi.disconnect();
		WiFi.reconnect();
		previousMillis = currentMillis;
	}
}
void executeCommand(char *command)
{
	char keypad_term[8] = {'\0'};
	strncpy(keypad_term, command, 6);
	if (!strcmp(command, "Open"))
	{
		Serial.println("The door is opening");

		door_open();
		door_close();
	}
	else if (!strcmp(command, "Close"))
	{
		Serial.println("The door is closing");
		// door_close();
	}
	else if (!strcmp(keypad_term, "Keypad"))
	{
		char length[MAX_OTP_SIZE];
		strncpy(length, command + 7, sizeof(command));
		otp_length = atoi(length);
		keypad_input = true;
		Serial.print("OTP length: ");
		Serial.println(otp_length);
		// myBLE.Stop();
	}
	else
	{
		Serial.println("Invalid Command");
	}
}

void door_open()

{
	Serial.println("The door is opening");
	// door_status = 1;
	for (int i=MAX_PWM_WRITE; i>=0 ; i -= STEPS)
	{
		analogWrite(RELAY_PIN, i);
		delay(DOORTIME_OPEN_INTERVAL);
	}
	delay(DOOR_KEEP_OPEN_TIME);
}
void door_close()
{	
	// digitalWrite(RELAY_PIN, HIGH);
	// if (door_status==1){
	for (int i=0; i < MAX_PWM_WRITE ; i += STEPS)
	{
		analogWrite(RELAY_PIN, i);
		delay(DOORTIME_CLOSE_INTERVAL);
	}
	// door_status=0;
	// }

}
void on_message(char *topic, byte *payload, unsigned int length)
{
	Serial.print("Message arrived on topic: ");
	Serial.print(topic_to_subscribe);
	Serial.print(". Message: ");

	subscription_message_length = length;
	// subscription_message = (char*)payload;
	// subscription_message + subscription_message_length = '\0';
	// msg[subscription_message_length] = '\0';
	for (int i = 0; i < length; i++)
	{
		msg[i] = (char)(*(payload + i));
	}
	msg[length] = '\0';

	Serial.print(msg);
	new_message = true;
	Serial.print(". Length: ");
	Serial.println(subscription_message_length);
}

void IRAM_ATTR buttonPressed()
{
	if (!digitalRead(pushbutton_pressed))
	{
		previousMillis = millis();
	}
	else if (millis() - previousMillis >= button_pressing_time)
	{
		pushbutton_pressed = true;
	}
	else
	{
		pushbutton_pressed = false;
	}
}
void clear_input_otp()
{
		input_otp = "";
		key_count = 0;
		input_otp[0] = '\0';
}