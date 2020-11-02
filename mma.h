#ifndef _MMA_H_
#define _MMA_H_

#include "mbed.h"

class MMAUnit {

private:
    int16_t accX, accY, accZ;
    I2C* i2c;
    
public:
    
    MMAUnit(I2C* i2c);
    
    int mmaInit();
    
    void mmaReadAcc();
    
    int16_t getAccX();
    int16_t getAccY();
    int16_t getAccZ();

};

#endif //_MMA_H_
