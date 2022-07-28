#ifndef BLE_H
#define BLE_H

#include<Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "BLEBeacon.h"
#define MAX_NUMBER_OF_DEVICES 100


// int RSSI_data;
// int device_count = 0;
// char MAC_data[17];
// char query[128];
// const char *dev_name;
// const char *uuid_string;

struct ID{
  std::string id[10];
};

struct BLE_Device_Properties
{
    int RSSI;
    String Address;
    int employee_no;
};


// int count=0;
// class MyCallbacks: public BLECharacteristicCallbacks 
// {

// };


class BLE
{
    public:
        struct ID employee;
        uint32_t scanTime;
        int device_count;
        BLEScan *scan;// = BLEDevice::getScan();
        BLEScanResults foundDevices;
        BLEAdvertisedDevice devices[MAX_NUMBER_OF_DEVICES];
        BLE_Device_Properties device_properties[MAX_NUMBER_OF_DEVICES];
        
        
        BLE();
        void init();
        void Scan();

};





#endif