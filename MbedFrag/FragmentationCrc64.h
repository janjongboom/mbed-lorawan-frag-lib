/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright (c) 2017 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _MBEDFRAG_FRAGMENTATION_CRC64_H_
#define _MBEDFRAG_FRAGMENTATION_CRC64_H_

#include "mbed.h"
#include "IFlash.h"
#include "crc.h"

class FragmentationCrc64 {
public:
    /**
     * Calculate the CRC64 hash of a file in flash
     *
     * @param flash         Instance of IFlash
     * @param buffer        A buffer to be used to read into
     * @param buffer_size   The size of the buffer
     */
    FragmentationCrc64(IFlash* flash, uint8_t* buffer, size_t buffer_size)
        : _flash(flash), _buffer(buffer), _buffer_size(buffer_size)
    {
    }

    /**
     * Calculate the CRC64 hash of the file
     *
     * @param address   Offset of the file in flash
     * @param size      Size of the file in flash
     *
     * @returns CRC64 hash of the file
     */
    uint64_t calculate(uint32_t address, size_t size) {
        size_t offset = 0;
        size_t bytes_left = size;

        uint64_t crc = 0;

        while (bytes_left > 0) {
            size_t length = _buffer_size;
            if (length > bytes_left) length = bytes_left;

            _flash->read(offset, _buffer, length);

            crc = crc64(crc, _buffer, length);

            offset += length;
            bytes_left -= length;
        }

        return crc;
    }

private:
    IFlash* _flash;
    uint8_t* _buffer;
    size_t _buffer_size;
};

#endif // _MBEDFRAG_FRAGMENTATION_CRC64_H_
