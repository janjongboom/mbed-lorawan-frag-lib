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
 */

#include "mbed.h"
#include "AT45.h"

//=============================================================================
// Public functions
//=============================================================================

AT45::AT45(SPI* spi, PinName ncs) : _spi(spi), _ncs(ncs)
{    
    
    _pages = -1;        // number of pages
    _pagesize = -1;     // size of pages 256/264, 512/528, 1024/1056
    _devicesize = -1;   // In bytes
    _deep_down = true; // variable for deep power down function (awake ?)
    _deep_down_onoff = false; // variable for deep power down function (On/Off) 

    _initialize();     // Populate all this stuff
    
}

// This function returns the char
char AT45::read_byte(int address) 
{   
    // return byte from address
    return (_memread( address ));
}  

int AT45::read_page(char* data, int page)         
{
    int address = -1;
    
    if(_pagesize == 256)
    {
        address = page * 256;
    }
    else if(_pagesize == 264)
    {
        address = page * 512;
    }
    else if(_pagesize == 512)
    {
        address = page * 512;
    }
    else if(_pagesize == 528)
    {
        address = page * 1024;
    }
    else if(_pagesize == 1024)
    {
        address = page * 1024;
    }
    else if(_pagesize == 1056)
    {
        address = page * 2048;
    }
    else 
    {
        return (-1); // something isnt configured right
    }
                
    _busy();        
    _flashread(1,address); // read the first page of the block into SRAM buffer 1
    _busy();               // Wait until First page has loaded into buffer 1
        
    // this is done directly as we are reading back an entire buffer, and this can be done more optimally than using _sramread
    _select();
    _spi->write(0xd4);
    _sendaddr (0x0);
    _spi->write (0x0);   // dont care byte
        
    for(int i=0; i<_pagesize ;i++) 
    {
        data[i] = _spi->write (0x0);  
    }
    _deselect();            
              
    return (0);
}
int AT45::read_block(char *data, int block)  // under construction F&#65533;R CHECK AF MIC OG LERCHE
{    
    char temp_data[_pagesize];
    int page_start;

    if (block < _blocks || block == 0) {
        page_start = block * 8;

        for (int i=0; i<8; i++) {
            read_page(temp_data, page_start);
            //printf("%d Read round, Data is: %d\r\n",i,*temp_data);
            page_start = page_start + 1;
            for (int z=0; z<_pagesize ; z++) {
                data[z+(_pagesize*i)] = temp_data[z];
            }
        }
    } else {
        //do nothing
    }
    //printf("Read done, Data is: %d\r\n",*data);
    return (0);
}
     
int AT45::read_block(char *data[], int block)       // under construction F&#65533; CHECK AF MIC OG LERCHE 
{
    char temp_data[_pagesize];
    int page_start;
    
    if(block < _blocks || block == 0)   
    {
        page_start = block * 8;
    
        for(int i=0; i<8 ;i++)                         // 8 pages in a block
        {
            read_page(temp_data, page_start);
            page_start = page_start + 1;
            for(int z=0; z<_pagesize ;z++)
            {
                data[i][z] = temp_data[z];
            }  
        }
    }
    else
    {
        //do nothing   
    }
 
    return (0);
}
       
// This function writes the char to the address supplied
// Note : We pass the raw address to the underlying functions
void AT45::write_byte(int address, char data) 
{
    _busy();   
    _flashread(1,address);             // read the Flash page into SRAM buffer
    _busy();                           // wait for the read to complete
    _sramwrite(1,address,data);        // Write new data into SRAM
    _busy();                           // Make sure flash isnt busy
    _flashwrite(1,address);            // Write back to the page address
}       
      
int AT45::write_page(char* data, int page)
{
    int address = -1;
    
    if(_pagesize == 256)
    {
        address = page * 256;
    }
    else if(_pagesize == 264)
    {
        address = page * 512;
    }
    else if(_pagesize == 512)
    {
        address = page * 512;
    }
    else if(_pagesize == 528)
    {
        address = page * 1024;
    }
    else if(_pagesize == 1024)
    {
        address = page * 1024;
    }
    else if(_pagesize == 1056)
    {
        address = page * 2048;
    }
    else 
    {
        return (-1); // something isnt configured right
    }
    
    _select();
    _spi->write(0x84); // writing to buffer #1
    _sendaddr (0); // we are writing to the entire buffer

    for(int i=0; i<_pagesize ;i++) 
    {
        _spi->write (data[i]);  
    }
    _deselect();        
        
    _busy(); // make sure the Flahs isnt busy
                
    // issue command to write buffer 1 to the appropraite flash page
    _select();  
    _spi->write (0x83);  
    _sendaddr (_getpaddr(address));  
    _deselect();
    
    return (0);   
}
int AT45::write_block(char *data, int block) // under construction F&#65533;R CHECK AF MIC OG LERCHE
{    
    
    int page_start;
    char page_arr[_pagesize];
    
    if (block < _blocks || block == 0) 
    {
        page_start = block * 8;
        
        for (int i=0; i<8 ; i++) 
        {
            // Copy data from *data at 0 to 511 _ 512 - 1023, and so on every round.  
            memcpy(page_arr, &data[_pagesize*i], _pagesize); 
            
            write_page(page_arr, page_start);
            page_start = page_start + 1;
        }
    } else {
        //do nothing
    }

    return (0);
}
int AT45::write_block(char *data[], int block)      // under construction F&#65533; CHECK AF MIC OG LERCHE 
{
    char temp_data[_pagesize];
    int page_start;
    
    if(block < _blocks || block == 0)   
    {
        page_start = block * 8;
    
        for(int i=0; i<8 ;i++) 
        {
            for(int z=0; z<_pagesize ;z++)
            {
               temp_data[z] = data[i][z];    
            }   
            write_page(temp_data, page_start);
            page_start = page_start + 1;
        }
    }
    else
    {
        //do nothing   
    }
 
    return (0);
}

int AT45::FAT_read(char* data, int page)          
{
    // For 256 byte pages, we read two pages
    if((_pagesize == 256) || (_pagesize == 264)) 
    {
        int address = page * 512; // This is the start address of the 512 byte block
           
        _flashread(1,address); // read the first page of the block into SRAM buffer 1
        _busy(); // Wait until First page has loaded into buffer 1
        
        // this is done directly as we are reading back an entire buffer, and this can be done more optimally than using _sramread
        _select();
        _spi->write(0xd4);
        _sendaddr (0x0);
        _spi->write (0x0);   // dont care byte
        
        for(int i=0;i<256;i++) 
        {
            data[i] = _spi->write (0x0);  
        }
        _deselect();            
                
        _flashread(1,address+256); // read the second page of the block into SRAM buffer 2
        _busy(); // Wait until second page has loaded into buffer 2
        
        // Now the second page is loaded, pull this out into the second half of the data buffer
        // this is done directly as we are reading back an entire buffer, and this can be done more optimally than using _sramread
        _select();
        _spi->write(0xd4);
        _sendaddr (0x0);
        _spi->write (0x0);   // dont care byte
        
        for(int i=0;i<256;i++) 
        {
            data[256+i] = _spi->write (0x0);  
        }
        _deselect();                
        return (0);
    }
    
    // For 512 byte pages, we read just the single page, transfer it    
    else if((_pagesize == 512) || (_pagesize == 528)) 
    {
        int address = page * 512; // This is the start address of the 512 byte block
    
        _busy(); // Wait until First page has loaded into buffer 1
        _flashread(1,address); // read the first page of the block into SRAM buffer 1
        _busy(); // Wait until First page has loaded into buffer 1
        
        // Now the page has loaded, simply transfer it from the sram buffer to the data array
        // this is done directly as we are reading back an entire buffer, and this can be done more optimally than using _sramread
        _select();
        _spi->write(0xd4);
        _sendaddr (0x0);
        _spi->write (0x0);   // dont care byte
        
        for(int i=0;i<512;i++) 
        {
            data[i] = _spi->write (0x0);  
        }
        _deselect();            
        return (0);
    }

    // For 1024 byte pages, we read just a single page, transfer half of it
    else if((_pagesize == 1024) || (_pagesize == 1056)) 
    {
        int address = _getpaddr(page * 512); // This is the start address of the 512 byte block

        _busy(); // Wait until First page has loaded into buffer 1
    
        _flashread(1,address); // read the first page of the block into SRAM buffer 1

        _busy(); // Wait until First page has loaded into buffer 1
        
        // Now the page has loaded, simply transfer it from the sram buffer to the data array
        // this is done directly as we are reading back an entire buffer, and this can be done more optimally than using _sramread

        _select();
        _spi->write(0xd4);

        if (page %2) // odd numbered block, read from adress 0x200
        { 
            _sendaddr (0x200);
        }
        else // even numbered block, then we are reading from sram buffer 0x0 
        {
            _sendaddr (0x0);
        }

        _spi->write (0x0);   // dont care byte
        
        for(int i=0;i<512;i++) 
        {
            data[i] = _spi->write (0x0);  
        }
        _deselect();            
        return (0);
    }
    else 
    {
        return (-1); // something isnt configured right
    }
}
       
int AT45::FAT_write(char* data, int page)        
{
    // For 256 byte pages, we overwrite two pages
    if((_pagesize == 256) || (_pagesize == 264))
    {
        
        // fill the first buffer with the first half of the block
        // do this directly, for better performance
        _select();
        _spi->write(0x84); // writing to buffer #1
        _sendaddr (0); // we are writing to the entire buffer

        for(int i=0;i<256;i++) {
            _spi->write (data[i]);  
        }
        _deselect();        
                
        _flashwrite(1,(page*512));

        // fill the buffer with the second half of the block
        // do this directly, for better performance
        _select();
        _spi->write(0x84); // writing to buffer #1
        _sendaddr (0); // we are writing to the entire buffer

        for(int i=0;i<256;i++) {
            _spi->write (data[256+i]);  
        }
        _deselect();        

        _flashwrite(1,((page*512)+256));
    }
        
    // For 512 byte pages, we overwrite a single page
    else if((_pagesize == 512) || (_pagesize == 528)) 
    {
    
        // fill the first buffer with the block data
        // do this directly, for better performance
        _select();
        _spi->write(0x84); // writing to buffer #1
        _sendaddr (0); // we are writing to the entire buffer

        for(int i=0;i<512;i++) {
            _spi->write (data[i]);  
        }
        _deselect();        
        
        _busy(); // make sure the Flahs isnt busy
                
        // issue command to write buffer 1 to the appropraite flash page
        _select();  
        _spi->write (0x83);  
        _sendaddr (_getpaddr(page * 512));  
        _deselect();                
    }

    // For 1024 byte pages, we do a read modify write
    // must make sure we overwrite the right half of the page!
    else if((_pagesize == 1024) || (_pagesize == 1056)) 
    {

        _busy(); // make sure the flash isnt busy

        int address = _getpaddr(page*512);

        // Read the page into sram
        _flashread(1,address);  
        
        // wait for this operation to complete
        _busy();
        
        // Overwrite the appropriate half
        // do this directly, for better performance
        _select();
        _spi->write(0x84); // writing to buffer #1

        if(page%2)  // this is an odd block number, overwrite second half of buffer 
        {  
            _sendaddr (0x200); // we are writing to the entire buffer
        }
        else        // this is an even block, overwrite the first half
        {  
            _sendaddr (0x0); // we are writing to the entire buffer
        }

        for(int i=0;i<512;i++) 
        {
            _spi->write (data[i]);  
        }
        _deselect();        
        
        // Write the page back
        _busy();
        _flashwrite(1,address); 
    }
    
    // Something has gone wrong
    else 
    {
        return (-1);
    }
    
    return (0);
}
       
// Erase the entire chip
void AT45::chip_erase() 
{ 
    _busy(); // make sure flash isnt already in busy.

    _select();
    // 4 byte command sequence
    _spi->write(0xc7);
    _spi->write(0x94);
    _spi->write(0x80);
    _spi->write(0x9a);
    _deselect();  

    _busy(); // Make erase a blocking function
}        

// Erase one block  
void AT45::block_erase(int block)      
{
    int address = -1;
    
    // Calculate page addresses
    if(block < _blocks || block == 0)   
    {
        if(_pagesize == 256)
        {
            address = block * 2048;
        }
        else if(_pagesize == 264)
        {
            address = block * 4096;
        }
        else if(_pagesize == 512)
        {
            address = block * 4096;
        }
        else if(_pagesize == 528)
        {
            address = block * 8192;
        }
        else if(_pagesize == 1024)
        {
            address = block * 8192;
        }
        else if(_pagesize == 1056)
        {
            address = block * 16384;
        }
        
        _busy();
        _select();
        _spi->write(0x50);
        _sendaddr (address);
        _deselect();
        _busy();   
    }
    else
    {
        //do nothing   
    }
}

// Erase one page       
void AT45::page_erase(int page)
{
    int address = -1;
    
    // Calculate page addresses
    if(page < _pages || page == 0)   
    {
        if(_pagesize == 256)
        {
            address = page * 256;
        }
        else if(_pagesize == 264)
        {
            address = page * 512;
        }
        else if(_pagesize == 512)
        {
            address = page * 512;
        }
        else if(_pagesize == 528)
        {
            address = page * 1024;
        }
        else if(_pagesize == 1024)
        {
            address = page * 1024;
        }
        else if(_pagesize == 1056)
        {
            address = page * 2048;
        }
        
        _busy();
        _select();
        _spi->write(0x81);
        _sendaddr (address);
        _deselect();
        _busy();   
    }
    else
    {
        //do nothing   
    }
}
      
// return the size of the part in bytes
int AT45::device_size() 
{ 
    return _devicesize;
}       

// return the numbers of pages
int AT45::pages() 
{ 
    return _pages;
}       
       
// return the page size of the part in bytes
int AT45::pagesize() 
{ 
    return _pagesize;
}       
       
// A one-time programmable configuration
void AT45::set_pageszie_to_binary()
{
    _busy(); // make sure flash isnt already in busy.

    _select();
    // 4 byte command sequence
    _spi->write(0x3d);
    _spi->write(0x2a);
    _spi->write(0x80);
    _spi->write(0xa6);
    _deselect();  

    _busy(); // Make erase a blocking function   
}

// Return the number of blocks in this device in accordance with the datasheet
int AT45::blocks() 
{  
    return _blocks;
}
       
// Return the Id of the part
int AT45::id() 
{ 
    int id = 0;
    _select();
    _spi->write(0x9f);
    id = (_spi->write(0x00) << 8);
    id |= _spi->write(0x00);
    _deselect();            
    return id; 
}
         
// return the Status
int AT45::status() 
{ 
    int status = 0;
    _select();
    _spi->write(0xd7);
    status = (_spi->write(0x00));
    _deselect();            
    return status; 
}
          
// Make sure the Flash isnt already doing something
void AT45::busy() 
{
    _busy();
}

void AT45::deep_power_down(bool _deep_down_onoff) 
{
    if(_deep_down_onoff == false) // Wake up from deep power down
    {
        _select();
        _spi->write(0xab);
        _deselect();
        _deep_down = true;
        // remenber to want 35uS before using the device.          
    }
    else if(_deep_down_onoff == true) // Go to deep power down
    {
        _busy();
        _select();
        _spi->write(0xb9);
        _deselect();
        _deep_down = false;
    }
    else
    {
        //do nothing   
    }       
}

bool AT45::is_it_awake()
{
    return _deep_down;
}

//=============================================================================
// Private functions
//=============================================================================

void AT45::_initialize()
{
    int _id = 0;
    int _status = 0;

    _id = id();
    _status = status();
    
    if ((_id & 0x1f) == 0x3)  // 2Mbits 
    { 
        _devicesize = 262144; // Size in bytes
        _pages = 1024;        // Number of pages
        _blocks = 128;        // Number of blocks
        if (_status & 0x1) 
        {
            _pagesize = 256;
        }
        else 
        {
            _pagesize = 264;
        }    
    }
    else if ( (_id & 0x1f) == 0x4) // 4Mbits 
    { 
        _devicesize = 524288;
        _pages = 2048;
        _blocks = 256;        
        if (_status & 0x1) 
        {
            _pagesize = 256;
        }
        else 
        {
            _pagesize = 264;
        }    
    }
    else if ( (_id & 0x1f) == 0x5) // 8Mbits 
    { 
        _devicesize = 1048576;
        _pages = 4096;
        _blocks = 512;        
        if (_status & 0x1) 
        {
            _pagesize = 256;
        }
        else 
        {
            _pagesize = 264;
        }    
    }
    else if ( (_id & 0x1f) == 0x6) // 16Mbits 
    { 
        _devicesize = 2097152;
        _pages = 4096;
        _blocks = 512;        
        if (_status & 0x1) 
        {
            _pagesize = 512;
        }
        else 
        {
            _pagesize = 528;
        }
    }
    else if ( (_id & 0x1f) == 0x7) // 32Mbits 
    { 
        _devicesize = 4194304;
        _pages = 8192;
        _blocks = 1024;        
        if (_status & 0x1) 
        {
            _pagesize = 512;
        }
        else 
        {
            _pagesize = 528;
        }
    }
    else if ( (_id & 0x1f) == 0x8) // 64Mbits 
    { 
        _devicesize = 8388608;
        _pages = 8192;
        _blocks = 1024;
        if (_status & 0x1) 
        {
            _pagesize = 1024;
        }
        else 
        {
            _pagesize = 1056;
        }
    }
    else 
    {
        _devicesize = -1;
        _pages = -1;
        _pagesize = -1;
        _blocks = -1;
    }
}

void AT45::_select()
{
    _ncs = 0;    
}

void AT45::_deselect()
{
    _ncs = 1;
}

void AT45::_busy() {
    volatile int iambusy = 1;  
    while (iambusy) {
        // if bit 7 is set, we can proceed
        if ( status() & 0x80 ) {
            iambusy = 0;}
    }
}

// Write to an SRAM buffer
// Note : We create buffer and page addresses in _sram and _flash        
void AT45::_sramwrite(int buffer, int address, int data)
{

    int cmd = 0;
    int baddr = 0;
  
    baddr = _getbaddr(address);
  
    _busy();
  
    _select();
  
    if (buffer == 1) 
        {cmd = 0x84;}
    else 
        {cmd = 0x87;}

    _spi->write(cmd);
    _sendaddr (baddr);
    _spi->write (data);  

    _deselect();            
}

// Read from an SRAM buffer
// Note : We create buffer and page addresses in _sram and _flash
int AT45::_sramread(int buffer, int address) 
{

    int cmd = 0;
    int baddr = 0;
    int bufdata = 0;
  
    baddr = _getbaddr(address);
  
    _select();
  
    if(buffer == 1) 
        {cmd = 0xd4;}
    else 
        {cmd = 0xd6;}
  
    _spi->write(cmd);
    _sendaddr (baddr);
    _spi->write (0x0);   // dont care byte
    bufdata = _spi->write (0x0);  
  
    _deselect();            
 
    return (bufdata);
}

// Write and SRAM buffer to main memory
void AT45::_flashwrite (int buffer, int address)
{

    int cmd = 0;
    int paddr = _getpaddr(address);
 
    _busy();  // Check flash is not busy 
    
    _select();  

    if (buffer == 1) 
        {cmd = 0x83;}
    else 
        {cmd = 0x86;}
      
    _spi->write (cmd);  
    _sendaddr (paddr);  
    _deselect();     

    _busy();  // Check flash is not busy 
}

// Read from Flash memory into SRAM buffer
void AT45::_flashread (int buffer, int address) 
{

    int cmd = 0;
    int paddr = _getpaddr(address); // calculate page address
  
    _busy();  // Check flash is not busy
    _select();
      
    if (buffer == 1)
        {cmd = 0x53;}
    else
        {cmd = 0x55;}

    _spi->write (cmd);  
    _sendaddr (paddr);
    _deselect();            
}

// Read directly from main memory
int AT45::_memread (int address) 
{

    int data = 0;        
    int addr;

    addr = _getpaddr(address) | _getbaddr(address);

    _busy();

    _select();

    _spi->write (0xd2);  // Direct read command
    _sendaddr (addr);
  
    // 4 dont care bytes
    _spi->write (0x00);  
    _spi->write (0x00);  
    _spi->write (0x00);  
    _spi->write (0x00);  
  
    // this one clocks the data
    data = _spi->write (0x00);  
    _deselect();            

    _busy();

    return data;
}

// Work out the page address
// If we have a 2^N page size, it is just the top N bits
// If we have non-2^N, we use the shifted address
int AT45::_getpaddr(int address) 
{

    int paddr;

    if (_pagesize == 256) {                 
        paddr = address & 0xffffff00;}    
    else if (_pagesize == 264) {                 
        paddr = (address << 1) & 0xfffffe00;}
    else if (_pagesize == 512) {                 
        paddr = address & 0xfffffe00;}
    else if (_pagesize == 528 ) {           
       paddr = (address << 1) & 0xfffffc00;}
    else if (_pagesize == 1024) {                 
       paddr = address & 0xfffffc00;}
    else if (_pagesize == 1056 ) {           
        paddr = (address << 1) & 0xfffff800;}
    else {
        paddr = -1;}
    
    return (paddr);
}

// Clean the buffer address. This is the 8/9/10 LSBs
int AT45::_getbaddr(int address) 
{

    int baddr;

    if ((_pagesize == 256) || (_pagesize == 264 )) {
        baddr = address & 0xff;}
    else if ((_pagesize == 512) || (_pagesize == 528 )) {
        baddr = address & 0x1ff;}
    else if ((_pagesize == 1024) || (_pagesize == 1056 )) {
        baddr = address & 0x3ff;}
    else {
        baddr = -1;}

    return (baddr);
}

// Sends the three lest significant bytes of the supplied address
void AT45::_sendaddr (int address) 
{
    _spi->write(address >> 16);
    _spi->write(address >> 8);
    _spi->write(address);      
}