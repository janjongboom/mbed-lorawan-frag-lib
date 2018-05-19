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
#include "crc.h"
#include "mbed-trace/mbed_trace.h"
#define TRACE_GROUP  "FCRC"

class FragmentationCrc64 {
public:
    /**
     * Calculate the CRC64 hash of a file in flash
     *
     * @param file          The file to calculate the hash off
     * @param buffer        A buffer to be used to read into
     * @param buffer_size   The size of the buffer
     */
    FragmentationCrc64(FILE *file, uint8_t* buffer, size_t buffer_size)
        : _file(file), _buffer(buffer), _buffer_size(buffer_size)
    {
    }

    /**
     * Calculate the CRC64 hash of (part of a) file
     *
     * @param offset        Offset from the start of the file (to skip over the signature), or 0 to read from beginning
     * @param length        Number of bytes to read (set to 0 to read til EOF)
     *
     * @returns CRC64 hash of the file
     */
    uint64_t calculate(size_t offset = 0, size_t length = 0) {
        // determine end of file
        if (fseek(_file, 0, SEEK_END) != 0) {
            tr_debug("Error when calling fseek");
            return 0;
        }
        size_t bytes_to_read = ftell(_file);
        tr_debug("File size is %u bytes", bytes_to_read);

        if (length > 0) {
            bytes_to_read = length;
        }

        tr_debug("Bytes to read is %u", bytes_to_read);

        // set file position to offset
        if (fseek(_file, offset, SEEK_SET) != 0) {
            tr_debug("Error when calling fseek");
            return 0;
        }

        uint64_t crc = 0;

        size_t bytes_read;

        while (bytes_to_read > 0) {
            if (bytes_to_read < _buffer_size) {
                bytes_read = fread(_buffer, 1, bytes_to_read, _file);
            }
            else {
                bytes_read = fread(_buffer, 1, _buffer_size, _file);
            }

            crc = crc64(crc, _buffer, bytes_read);

            bytes_to_read -= bytes_read;
        }

        return crc;
    }

private:
    FILE* _file;
    uint8_t* _buffer;
    size_t _buffer_size;
};

#endif // _MBEDFRAG_FRAGMENTATION_CRC64_H_
