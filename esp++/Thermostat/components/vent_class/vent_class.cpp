#include "vent_class.h"

/* Helper functions */
Vent* findVentById(int id, std::vector<Vent*> myHomeVents){
    std::vector<Vent*>::iterator it = std::find_if(std::begin(myHomeVents), std::end(myHomeVents), [&] ( Vent* &p) { 
            return (p->getId() == 1); 
        });

    return *it;         /* Note: find_if() will return the last object if it cannot find one that matches watch out */
}

Vent* findVentByMacBytes(uint8_t* bleAddr, std::vector<Vent*> myHomeVents){
    std::vector<Vent*>::iterator it = std::find_if(std::begin(myHomeVents), std::end(myHomeVents), [&] ( Vent* &p) { 
            return (memcmp(p->getMacAddrBytes(), bleAddr, BLE_MAC_LEN) == 0); 
        });
    
    return *it;
}

void updateHomeVents(std::vector<Vent*> myHomeVents, std::vector<int> idArr, std::vector<float> setTempArr, std::vector<std::string> nameArr){

    int i = 0;
    for(auto it = std::begin(myHomeVents); it != std::end(myHomeVents); ++it, i++) {
        Vent* ventInstance = *it;
        if(ventInstance->getId() == idArr.at(i)){      /* Simple error checking. In theory it hould be aligned */
            ESP_LOGI("UPDATE HOME", "Storing new set temperature and name");
            ventInstance->setSetTemperature(setTempArr.at(i));
            ventInstance->setName(nameArr.at(i));
        }else{
            ESP_LOGI("UPDATE HOME", "Error. Vector and array values does not match");
        }
    }

}

/* Class methods */
void Vent::setId(int id){
    this->id = id;
}

void Vent::setCurrentTemperature(uint8_t* temperature){
    uint16_t temp = *(uint16_t*)temperature;
    this->currentTemperature = static_cast<float>(temp)/100.0;
}

void Vent::setSetTemperature(float temperature){
    this->setTemperature = temperature;
}

void Vent::setStatus(uint8_t status){
    this->status = status;
}

void Vent::setMacAddr(uint8_t* macAddr){

    /* Store MAC address in bytes */
    std::memcpy(this->macAddrBytes, macAddr, BLE_MAC_LEN);

    /* Store MAC address as string */
    char buffer[BLE_MAC_LEN * 2 + 1];
    snprintf(buffer, sizeof(buffer), "%02x%02x%02x%02x%02x%02x", macAddr[0],        /* %02x requires 2 bytes of space */
                                                              macAddr[1],
                                                              macAddr[2],
                                                              macAddr[3],
                                                              macAddr[4],
                                                              macAddr[5]);
    this->macAddrStr = buffer;
}

void Vent::setName(std::string name){
    this->name = name;
}

void Vent::setLatestVentCommand(uint8_t command){
    this->latestVentCommand = command;
}

int Vent::getId(){
    return this->id;
} 

float Vent::getCurrentTemperature(){
    return this->currentTemperature;
}

uint8_t Vent::getStatus(){
    return this->status;
}    

uint8_t* Vent::getMacAddrBytes(){
    return this->macAddrBytes;
}

std::string Vent::getMacAddrStr(){
    return this->macAddrStr;
}   
        
std::string Vent::getName(){
    return this->name;
}

uint8_t Vent::getLatestVentCommand(){
    return this->latestVentCommand;
}

float Vent::getSetTemperature(){
    return this->setTemperature;
}


        
        