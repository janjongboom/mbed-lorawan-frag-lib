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

#ifndef _MBEDFRAG_FRAGMENTATION_SHA256_H_
#define _MBEDFRAG_FRAGMENTATION_SHA256_H_

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_SHA256_C)

#include "mbed.h"
#include "sha256.h"

#include "mbed-trace/mbed_trace.h"
#define TRACE_GROUP  "FSHA"

class FragmentationSha256 {
public:
    /**
     * Calculate the SHA256 hash of a file
     *
     * @param file          The file to calculate the hash off
     * @param buffer        A buffer to be used to read into
     * @param buffer_size   The size of the buffer
     */
    FragmentationSha256(FILE *file, uint8_t* buffer, size_t buffer_size)
        : _file(file), _buffer(buffer), _buffer_size(buffer_size)
    {
    }

    /**
     * Calculate the SHA256 hash of the file
     *
     * @param output Output buffer
     *
     * @returns true if calculating the hash succeeded, false if an error occured
     */
    bool calculate(unsigned char output[32]) {
        return calculate(0, 0, output);
    }

    /**
     * Calculate the SHA256 hash of part of the file
     *
     * @param offset Offset in the file
     * @param length Number of bytes to read
     * @param output Output buffer
     *
     * @returns true if calculating the hash succeeded, false if an error occured
     */
    bool calculate(size_t offset, size_t length, unsigned char output[32]) {
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

        mbedtls_sha256_init(&_sha256_ctx);
        mbedtls_sha256_starts(&_sha256_ctx, false /* is224 */);

        size_t bytes_read;

        while (bytes_to_read > 0) {
            if (bytes_to_read < _buffer_size) {
                bytes_read = fread(_buffer, 1, bytes_to_read, _file);
            }
            else {
                bytes_read = fread(_buffer, 1, _buffer_size, _file);
            }

            mbedtls_sha256_update(&_sha256_ctx, _buffer, bytes_read);

            bytes_to_read -= bytes_read;
        }

        mbedtls_sha256_finish(&_sha256_ctx, output);
        mbedtls_sha256_free(&_sha256_ctx);

        return true;
    }

private:
    FILE* _file;
    uint8_t* _buffer;
    size_t _buffer_size;
    mbedtls_sha256_context _sha256_ctx;
};

#endif

#endif // _MBEDFRAG_FRAGMENTATION_SHA256_H_
