#ifndef VENT_CLASS_H
#define VENT_CLASS_H

#include <iostream>
#include <string>
#include <stdio.h>
#include <cstring>
#include <vector>
#include <algorithm>

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

    public:
        void setId(int id);
        void setCurrentTemperature(uint16_t* temperature);
        void setSetTemperature(float temperature);
        void setStatus(uint8_t status);
        void setMacAddr(uint8_t* macAddr);
        void setName(std::string name);

        int getId();
        float getCurrentTemperature();
        uint8_t getStatus();
        uint8_t* getMacAddrBytes();
        std::string getMacAddrStr();
        std::string getName();


};

Vent* findVentById(int id, std::vector<Vent*> myHomeVents);

Vent* findVentByMacBytes(uint8_t* bleAddr, std::vector<Vent*> myHomeVents);

void updateHomeVents(std::vector<Vent*> myHomeVents, int* idArr, float* setTempArr);

#endif