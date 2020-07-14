#ifndef VENT_CLASS_H
#define VENT_CLASS_H

#include <iostream>
#include <string>
#include <stdio.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include "esp_log.h"


#define BLE_MAC_LEN 6

class Vent
{
    private:
        int id;                                 /* Database Id */
        float currentTemperature;               /* Current vent temperature */
        float setTemperature;                   /* Desired temperature */
        uint8_t status;                         /* Current vent position  */
        std::string macAddrStr;                 /* MAC translated into string. Comes from server */
        uint8_t macAddrBytes[BLE_MAC_LEN];      /* MAC address retrieved by UART */
        std::string name;                       /* Vent location i.e living room*/
        int batteryLevel;                       /* To be implemented */
        uint8_t latestVentCommand;              /* Last command given to vent i.e open, close or do nothing */

    public:
        void setId(int id);
        void setCurrentTemperature(uint8_t* temperature);
        void setSetTemperature(float temperature);
        void setStatus(uint8_t status);
        void setMacAddr(uint8_t* macAddr);
        void setName(std::string name);
        void setLatestVentCommand(uint8_t command);

        int getId();
        float getSetTemperature();
        float getCurrentTemperature();
        uint8_t getStatus();
        uint8_t* getMacAddrBytes();
        std::string getMacAddrStr();
        std::string getName();
        uint8_t getLatestVentCommand();
};

Vent* findVentById(int id, std::vector<Vent*> myHomeVents);

Vent* findVentByMacBytes(uint8_t* bleAddr, std::vector<Vent*> myHomeVents);

void updateHomeVents(std::vector<Vent*> myHomeVents, std::vector<int> idArr, std::vector<float> setTempArr, std::vector<std::string> nameArr);

#endif