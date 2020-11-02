#include "mma.h"

#define MMA_ADDR 0x3A

#define REG_XHI 0x01
#define REG_XLO 0x02
#define REG_YHI 0x03
#define REG_YLO 0x04
#define REG_ZHI 0x05
#define REG_ZLO 0x06

#define REG_WHOAMI 0x0D
#define REG_CTRL1  0x2A
#define REG_CTRL4  0x2D

#define WHOAMI 0x1A

#define UINT14_MAX        16383

MMAUnit::MMAUnit(I2C* i2c) {
    this->i2c = i2c;
}

int MMAUnit::mmaInit(){
    //check for device
    char data[2];
    data[0] = REG_WHOAMI;
    i2c->write(MMA_ADDR, data, 1, true);
    i2c->read(MMA_ADDR, data, 1);

    if(data[0] == WHOAMI)   {
    //wait(10);
        //set active mode, 14 bit samples and 100 Hz ODR (0x19)
        data[0] = REG_CTRL1;
        data[1] = 0x01;
        i2c->write(MMA_ADDR, data, 2);
        return 1;
    }
    //else error
    return 0;
}
    
void MMAUnit::mmaReadAcc() {
    char data[6];
    
    
    data[0] = REG_XHI;
    i2c->write(MMA_ADDR, data, 1, true);
    i2c->read(MMA_ADDR, data, 6);
    
    accX = (data[0] << 6) | (data[1] >> 2);
    if (accX > UINT14_MAX/2)
        accX -= UINT14_MAX;
        
    accY = (data[2] << 6) | (data[3] >> 2);
    if (accY > UINT14_MAX/2)
        accY -= UINT14_MAX;
        
    accZ = (data[4] << 6) | (data[5] >> 2);
    if (accZ > UINT14_MAX/2)
        accZ -= UINT14_MAX;
}
    
int16_t MMAUnit::getAccX() {
    return accX;
}

int16_t MMAUnit::getAccY() {
    return accY;
}

int16_t MMAUnit::getAccZ() {
    return accZ;    
}