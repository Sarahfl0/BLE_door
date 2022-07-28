#ifndef EVENT_H
#define EVENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <BLEScan.h>
#include <map>
#include<NTPClient.h>
#include<WiFi.h>
#define MAX_NUMBER_OF_DEVICES 100

struct Device_Properties
{
    int RSSI;
    String Address;
    int employee_no;
};

class EVENT{
    public:
        std::map<String, bool> prev_status, cur_status;
        // Device_Properties devices[MAX_NUMBER_OF_DEVICES];

        String createJson(BLEScanResults result, String esp_mac);
        String createTimeStamp();
};  

#endif