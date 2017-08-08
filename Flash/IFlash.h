#ifndef _FRAGMENTATION_FLASH_H
#define _FRAGMENTATION_FLASH_H

#include "mbed.h"

class IFlash {
public:
    virtual ~IFlash() {}

    virtual int write(uint32_t addr, uint8_t* buffer, size_t size) = 0;
    virtual int read(uint32_t addr, uint8_t* buffer, size_t size) = 0;
    virtual int erase(uint32_t addr, size_t size) = 0;
};

#endif
