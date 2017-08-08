/* mbed AT45 Library, for driving the Atmel AT45 series Dataflash with Serial Interface (SPI)
 * Copyright (c) 2012, Created by Steen Joergensen (stjo2809) inspired by Chris Styles AT45 library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *
 * This driver supports 021,041,081,161,321,641 variants of the AT45DBxxx family
 *
 * || Code || Density  || Page size || Pages || Package ||
 * || 021  || 2        || 256       || 1024  || 8 SOIC  ||
 * || 041  || 4        || 256       || 2048  || 8 SOIC  ||
 * || 081  || 8        || 256       || 4096  || 8 SOIC  ||
 * || 161  || 16       || 512       || 4096  || 8 SOIC  ||
 * || 321  || 32       || 512       || 8192  || 8 SOIC  ||
 * || 641  || 64       || 1024      || 8192  || 28 TSOP ||
 */

#include "mbed.h"

#ifndef AT45_H
#define AT45_H

//=============================================================================
// Functions Declaration
//=============================================================================

/** Interface to the Atmel AT45 series Dataflash with Serial Interface (SPI)
 *
 *  Using the driver:
 *   - remenber to setup SPI in main routine.
 *   - remenber to setup Chipselect in main routine.
 *   - remenber to use FAT functions set pagesize to binary.
 *
 *  Limitations of using this driver:
 *   - can't use lockdown functions.
 *   - can't use protections functions.
 *   - can't use security functions.
 *
 */
class AT45 {
public:        
       /** Create an instance of the AT45 connected to specfied SPI pins, with the specified address.
        *
        * @param spi The mbed SPI instance (make in main routine)
        * @param nCs The SPI chip select pin.
        */
       AT45(SPI* spi, PinName ncs);
       
       /** Read a byte.
        *
        * @param address The address of the byte to read.
        * @return The data in the byte.
        */
       char read_byte(int address);

       /** Read a page.
        *
        * @param data The data is pointer to a userdefined array that the page is read into.
        * @param page The page number of the page to read (0 to device page size).
        * @return Returns "0" or "-1" for error.
        */
       int read_page(char* data, int page);
       
       /** Read a block (from 1 dimension array).
        *
        * @param data The data is pointer to a userdefined array that holds 4096 bytes of the data that is read into.
        * @param block The block number of the block to read (0 to device block size).
        * @return Returns "0" or "-1" for error.
        */        
       int read_block(char *data, int block);
       
       /** Read a block (from 2 dimension array).
        *
        * Then using 2 dimension array, the array shall have a starting point like "read_block(data[0],0)"
        * @param data The data is pointer to a userdefined array that holds 8 x 512 bytes of the data that is read into.
        * @param block The block number of the block to read (0 to device block size).
        * @return Returns "0" or "-1" for error.
        */       
       int read_block(char *data[], int block);

       /** Write a byte.
        *
        * @param address The address to where the data is storage in the flash.
        * @param data The data to write into the flash.
        */       
       void write_byte(int address, char data);
       
       /** Write a page.
        *
        * @param data The data is pointer to a userdefined array that holds the data to write into.
        * @param page The page number of the page to write into (0 to device page size).
        * @return Returns "0" or "-1" for error.
        */
       int write_page(char* data, int page);

       /** Write a block (from 1 dimension array).
        *
        * @param data The data is pointer to a userdefined array that holds 4096 bytes of the data to write into.
        * @param block The block number of the block to write into (0 to device block size).
        * @return Returns "0" or "-1" for error.
        */       
       int write_block(char *data, int block);
        
       /** Write a block (from 2 dimension array).
        *
        * Then using 2 dimension array, the array shall have a starting point like "write_block(data[0],0)"
        * @param data The data is pointer to a userdefined array that holds 8 x 512 bytes of the data to write into (remenber it has to be a 2 dimension array).
        * @param block The block number of the block to write into (0 to device block size).
        * @return Returns "0" or "-1" for error.
        */       
       int write_block(char *data[], int block);

       /** FAT Read (512 bits).
        *
        * Remenber to set page size to binary.
        * @param data The data is pointer to a userdefined array that the page is read into.
        * @param page The page number of the page to read (0 to device page size).
        * @return Returns "0".
        */       
       int FAT_read(char* data, int page);
       
       /** FAT Write (512bits).
        *
        * Remenber to set page size to binary.
        * @param data The data is pointer to a userdefined array that holds the data to write into.
        * @param page The page number of the page to write into (0 to device page size).
        * @return Returns "0" or "-1" for error.
        */       
       int FAT_write(char* data, int page); 
            
       /** Function to erase the entire chip.
        *
        *  Issue:
        *  In a certain percentage of units, the chip erase feature may not function correctly and may adversely affect device operation.
        *  Therefore, it is recommended that the chip erase commands (opcodes C7H, 94H, 80H, and 9AH) not be used.
        *  Workaround:
        *  Use block erase (opcode 50H) as an alternative. The block erase function is not affected by the chip erase issue.
        *  Resolution:
        *  The chip erase feature may be fixed with a new revision of the device. Please contact Atmel for the estimated availability of
        *  devices with the fix.
        */
       void chip_erase(void);
       
       /** Function to erase the selected block.
        *
        * @param block The selected block to erase.
        */
       void block_erase(int block);
       
       /** Function to erase the selected block.
        *
        * @param page The number of the page to erase.
        */
       void page_erase(int page);
       
       /** Device size in mbits.
        *
        * @return device size.
        */
       int device_size(void);
       
       /** Pages in flash.
        *
        * @return Numbers af pages.
        */
       int pages(void);
       
       /** Page size.
        * 
        * for 2-8 Mbits 256 or 264
        * for 16-32 Mbits 512 or 528
        * for 64 Mbits 1024 or 1056
        *
        * @return Page size.
        */
       int pagesize(void);

       /** Function to set the page size to binary.
        *
        *  Remenber is a one-time programmable configuration.
        *  Remenber to total power down after the functions has been run. 
        */       
       void set_pageszie_to_binary(void);
       
       /** blocks in flash.
        *
        * @return Numbers af blocks.
        */       
       int blocks(void);
       
       /** ID of the device.
        *
        * @return Manufacturer, Family and Density code.
        */          
       int id(void);
       
       /** Status register.
        *
        * @return The status register.
        */           
       int status(void);    // Status register
       
       /** busy ?.
        *
        * Function will want to the device is not busy.
        */           
       void busy(void); // Wait until Flash is not busy
       
       /** Deep Power Down.
        *
        * Remenber that you have to want 35uS after the wake up to use the device. 
        * @param True = Activate and False = Wake Up.
        */         
       void deep_power_down(bool onoff);
        
       /** Is the device deep power down.
        *
        * @return True = Activate and False = Awake.
        */         
       bool is_it_awake(void);

private:    
   
        SPI* _spi;
        DigitalOut _ncs;    

        int _pages;            // Integer number of pages
        int _pagesize;         // page size, in bytes 
        int _devicesize;       // device size in bytes
        int _blocks;           // Number of blocks
        bool _deep_down;       // True = the device is deep down
        bool _deep_down_onoff; // variable for deep power down function (On/Off) 
 
        // Helper routunes
        void _initialize();          
        void _select();
        void _deselect();
        void _busy (void);               
    
        // accessing SRAM buffers
        void _sramwrite (int buffer, int address, int data);
        int _sramread (int buffer, int address);
    
        // Transferring SRAM buffers to/from FLASH
        void _flashwrite (int buffer, int paddr);
        void _flashread (int buffer, int paddr);

        // Reading FLASH directly
        int _memread (int address);
    
        // Calculate page/subpage addresses
        int _getpaddr (int);
        int _getbaddr (int);
    
        // Send 3 byte address
        void _sendaddr (int address);
        
};
#endif



