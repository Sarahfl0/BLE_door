#include "BLE.h"


// int RSSI_data;

// char MAC_data[17];



// String EmployeeMacs[3] = { "ac:23:3f:a1:79:49", "ac:23:3f:a1:7b:07",
//                              "ac:23:3f:a1:79:3d" };
// int length = sizeof(EmployeeMacs)/sizeof(EmployeeMacs[0]);
BLE::BLE()
{

}

void BLE:: init()
{
  

}

void BLE::Stop()
{
  this->stop->stop();
}

void BLE::Scan() 
{
  BLEDevice::init("BLE");
  this->scan = BLEDevice::getScan(); //create new scan-
  // this->scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  this->scanTime=1;
  this->scan->setActiveScan(true);
  this->foundDevices = this->scan->start(scanTime);
  this->device_count = this->foundDevices.getCount();
  // this->stop->stop();
  
  // Serial.printf("Number of devices: %d\n", this->device_count);
  // Serial.print("Devices found: \n");
  // int count = 0;
// ******************The new lines

  // this->setInterval(1349);
  // this->setWindow(449);
  // this->setActiveScan(true);



  // *****************
  for (uint32_t i = 0; i < this->device_count; i++)
  {
    BLEAdvertisedDevice currentdevice = foundDevices.getDevice(i);
    // strcpy( MAC_data, currentdevice.getAddress().toString().c_str());
    // Serial.printf("MAC address = %s --- ", MAC_data);
    // Serial.printf("\n");
    // for(int j = 0; j < length; j++){
    //   // Serial.printf("EmployeeMacs %d ",j);
    //   // Serial.printf(EmployeeMacs[j].c_str());
    //   if( EmployeeMacs[j].equals(MAC_data)){
    //     this->devices[count]=this->foundDevices.getDevice(i);
    //     // Serial.printf("\nMAC address = %s is an employee device", MAC_data);

    //     count++;
    //   }
      // Serial.printf("\n");
    }
  }


    // this->devices[i]=this->foundDevices.getDevice(i);
    // this->device_properties[i].RSSI = this->devices[i].getRSSI();
    // this->device_properties[i].Address = this->devices[i].getAddress().toString().c_str();
    // RSSI_data = this->devices[i].getRSSI();
    

    
    // Serial.printf("RSSI =  %d --- ", RSSI_data);
    // Serial.printf("\n");
 
  // return this->foundDevices
