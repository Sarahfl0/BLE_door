#include "EVENT.h"


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600);

// String EmployeeMacs[13] = { "ac:23:3f:a1:7b:09","ac:23:3f:a1:7b:10","ac:23:3f:a1:7b:15","ac:23:3f:a1:7b:13", "ac:23:3f:a1:79:27", "ac:23:3f:a1:7b:08", "ac:23:3f:a1:79:48", "ac:23:3f:a1:79:25",  "ac:23:3f:a1:79:c2", "ac:23:3f:a1:79:4b","ac:23:3f:a1:79:49","ac:23:3f:a1:7b:07", "ac:23:3f:a1:79:3d" };
String EmployeeMacs[4] = {"ac:23:3f:a1:7b:09", "ac:23:3f:a1:7b:10", "ac:23:3f:a1:7b:15","ac:23:3f:a1:7b:13" };
int length = sizeof(EmployeeMacs)/sizeof(EmployeeMacs[0]);
char MAC_data[17];

String EVENT::createTimeStamp()
{
    timeClient.update();

    unsigned long epochTime = timeClient.getEpochTime();


    String formattedTime = timeClient.getFormattedTime();
 

    byte currentHour = timeClient.getHours() + 5; //should be +6, but it seems +5 works
 

    byte currentMinute = timeClient.getMinutes();


    byte currentSecond = timeClient.getSeconds();

    struct tm *ptm = gmtime ((time_t *)&epochTime); 

    byte monthDay = ptm->tm_mday;

    byte currentMonth = ptm->tm_mon+1;


    int currentYear = ptm->tm_year+1900;


    return String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay) + " " 
            + String(currentHour) + ":" + String(currentMinute) + ":" + String(currentSecond);
}

String EVENT::createJson(BLEScanResults result, String esp_mac)
{
    int number_of_connected_devices = result.getCount();
    BLEAdvertisedDevice device; 
    String mac_add;

    StaticJsonDocument<1024> doc;
    doc["device_id"] = esp_mac;
    doc["time_stamp"] = this->createTimeStamp();
    // doc["device_count"] = number_of_connected_devices;

    JsonObject event = doc.createNestedObject("event");

    // JsonArray event_missing = event.createNestedArray("missing");
    JsonObject event_new = event.createNestedObject("new");

    
    JsonArray event_new_mac_id = event_new.createNestedArray("mac_id");
    JsonArray event_new_rssi = event_new.createNestedArray("rssi");
    // JsonArray event_new_battery = event_new.createNestedArray("battery");
    // JsonArray event_new_employee_no = event_new.createNestedArray("employee_no");

    this->cur_status.clear();
    // bool new_miss = false;
    int count = 0;
    for(int i = 0; i < number_of_connected_devices; i++)
    {
        device = result.getDevice(i);
        mac_add = device.getAddress().toString().c_str();
        for(int j = 0; j < length; j++)
        {
            // if( EmployeeMacs[j].equals(mac_add))
            if(mac_add[0]=='a' && mac_add[1]=='c')
            {
              
                this->devices[count].RSSI = device.getRSSI();
                this->devices[count].Address = mac_add;
                this->devices[count].employee_no = j;
                strcpy( MAC_data, device.getAddress().toString().c_str());
                Serial.printf(" MAC address = %s is an employee device of employee no %d\n", MAC_data,j);

                count++;
                if(!this->prev_status[mac_add])
                    {
                        // new_miss = true;
                        event_new_mac_id.add(mac_add);
                        event_new_rssi.add(device.getRSSI());
                        // event_new_battery.add(device.getTXPower());
                        // event_new_employee_no.add(j);
                    }
                    this->cur_status[mac_add] = 1;
            }
        }
        doc["device_count"] = count;

    }
    // for(auto mac: this->prev_status)
    // {
        
    //     if(!this->cur_status[mac.first])
    //     {
    //         new_miss = true;
    //         event_missing.add(mac.first);
    //     }
    // }
    Serial.println();
    // delay(10000);

    this->prev_status.clear();
    this->prev_status = this->cur_status;

    String serialized;
    serializeJson(doc, serialized);
    // Serial.println("New Event? " + String(new_miss ? "yes" : "no"));

    // if(new_miss)
    // {
    //     serializeJsonPretty(doc,Serial);
    //     Serial.println();
    //     return serialized;
    // }
    return serialized;

}
