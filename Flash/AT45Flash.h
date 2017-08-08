#ifndef _FRAGMENTATION_FLASH_AT45_H_
#define _FRAGMENTATION_FLASH_AT45_H_

#include "mbed.h"
#include "IFlash.h"
#include "AT45.h"

class AT45Flash : public IFlash {
public:
    AT45Flash() : spi(SPI_MOSI, SPI_MISO, SPI_SCK, SPI_NSS), at45(spi, SPI_NSS) {
        pagesize = at45.pagesize();

        // should this move to an init call? can't throw in ctor
        pagebuffer = (char*)calloc(pagesize, 1);
        if (!pagebuffer) {
            printf("[AT45Flash] Failed to initialize pagebuffer\n");
        }
    }

    ~AT45Flash() {
        if (pagebuffer) free(pagebuffer);
    }

    virtual int write(uint32_t addr, uint8_t* buffer, size_t size) {
        // Q: a 'global' pagebuffer makes this code not thread-safe...
        // is this a problem? don't really wanna malloc/free in every call

        // printf("write addr=%lu size=%d\n", addr, size);

        // find the page
        size_t bytes_left = size;
        while (bytes_left > 0) {
            uint32_t page = addr / pagesize; // this gets auto-rounded
            uint32_t offset = addr % pagesize; // offset from the start of the pagebuffer
            uint32_t length = pagesize - offset; // number of bytes to write in this pagebuffer
            if (length > bytes_left) length = bytes_left; // don't overflow

            // printf("Writing to page=%lu, offset=%lu, length=%lu\n", page, offset, length);

            int r;

            // retrieve the page first, as we don't want to overwrite the full page
            r = at45.read_page(pagebuffer, page);
            if (r != 0) return r;

            // printf("pagebuffer of page %d is:\n", page);
            // for (size_t ix = 0; ix < pagesize; ix++) {
            //     printf("%02x ", pagebuffer[ix]);
            // }
            // printf("\n");

            // now memcpy to the pagebuffer
            memcpy(pagebuffer + offset, buffer, length);

            // printf("pagebuffer after memcpy is:\n", page);
            // for (size_t ix = 0; ix < pagesize; ix++) {
            //     printf("%02x ", pagebuffer[ix]);
            // }
            // printf("\n");

            // and write back
            r = at45.write_page(pagebuffer, page);
            if (r != 0) return r;

            // change the page
            bytes_left -= length;
            addr += length;
            buffer += length;
        }

        return 0;
    }
    virtual int read(uint32_t addr, uint8_t* buffer, size_t size) {
        // printf("read addr=%lu size=%d\n", addr, size);

        size_t bytes_left = size;
        while (bytes_left > 0) {
            uint32_t page = addr / pagesize; // this gets auto-rounded
            uint32_t offset = addr % pagesize; // offset from the start of the pagebuffer
            uint32_t length = pagesize - offset; // number of bytes to read in this pagebuffer
            if (length > bytes_left) length = bytes_left; // don't overflow

            // printf("Reading from page=%lu, offset=%lu, length=%lu\n", page, offset, length);

            int r = at45.read_page(pagebuffer, page);
            if (r != 0) return r;

            // copy into the provided buffer
            memcpy(buffer, pagebuffer + offset, length);

            // change the page
            bytes_left -= length;
            addr += length;
            buffer += length;
        }

        return 0;
    }

    virtual int erase(uint32_t addr, size_t size) {
        // printf("erase addr=%lu size=%d\n", addr, size);

        uint32_t start_page = addr / pagesize; // this gets auto-rounded
        uint32_t end_page = (addr + size) / pagesize;

        memset(pagebuffer, 0xff, pagesize);

        for (size_t ix = start_page; ix <= end_page; ix++) {
            int r = at45.write_page(pagebuffer, ix);
            if (r != 0) return r;
        }

        return 0;
    }

private:
    SPI  spi;
    AT45 at45;
    int  pagesize;
    char* pagebuffer;
};

#endif // _FRAGMENTATION_FLASH_AT45_H_
