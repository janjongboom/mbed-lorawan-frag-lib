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

class FragmentationSha256 {
public:
    /**
     * Calculate the SHA256 hash of a file in flash
     *
     * @param flash         Instance of BlockDevice
     * @param buffer        A buffer to be used to read into
     * @param buffer_size   The size of the buffer
     */
    FragmentationSha256(BlockDevice* flash, uint8_t* buffer, size_t buffer_size)
        : _flash(flash), _buffer(buffer), _buffer_size(buffer_size)
    {
    }

    /**
     * Calculate the SHA256 hash of the file
     *
     * @param address   Offset of the file in flash
     * @param size      Size of the file in flash
     *
     * @returns SHA256 hash of the file
     */
    void calculate(uint32_t address, size_t size, unsigned char output[32]) {
        mbedtls_sha256_init(&_sha256_ctx);
        mbedtls_sha256_starts(&_sha256_ctx, false /* is224 */);

        size_t offset = address;
        size_t bytes_left = size;

        while (bytes_left > 0) {
            size_t length = _buffer_size;
            if (length > bytes_left) length = bytes_left;

            _flash->read(_buffer, offset, length);

            mbedtls_sha256_update(&_sha256_ctx, _buffer, length);

            offset += length;
            bytes_left -= length;
        }

        mbedtls_sha256_finish(&_sha256_ctx, output);
        mbedtls_sha256_free(&_sha256_ctx);
    }

private:
    BlockDevice* _flash;
    uint8_t* _buffer;
    size_t _buffer_size;
    mbedtls_sha256_context _sha256_ctx;
};

#endif

#endif // _MBEDFRAG_FRAGMENTATION_SHA256_H_
