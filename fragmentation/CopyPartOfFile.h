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

#ifndef _MBEDFRAG_COPY_PART_OF_FILE_H
#define _MBEDFRAG_COPY_PART_OF_FILE_H

#include "mbed.h"
#include "mbed_trace.h"

#define TRACE_GROUP "CPOF"

/**
 * Copy part of a file into a new file
 *
 * @param source            Opened (for reading) file
 * @param destinationName   New filename
 * @param offset            Offset in the old file to use as source
 * @param length            Number of bytes since offset to write
 * @param buffer            Buffer to use while copying
 * @param buffer_size       Size of the copy buffer
 *
 * @returns 0 if OK, negative code if an error happend
 */
int copy_part_of_file(FILE *source, const char *destinationName, size_t offset, size_t length, uint8_t *buffer, size_t buffer_size) {
    tr_debug("Copying into '%s', offset %u, length %u", destinationName, offset, length);

    FILE *destination = fopen(destinationName, "wb+");
    if (!destination) {
        tr_debug("Could not create destination file '%s'", destinationName);
        return -10;
    }

    int ret;

    ret = fseek(source, offset, SEEK_SET);
    if (ret != 0) {
        tr_debug("Could not fseek on source file");
        return ret;
    }

    size_t bytes_left = length;

    while (bytes_left > 0) {
        size_t bytes_to_read = bytes_left > buffer_size ? buffer_size : bytes_left;
        ret = fread(buffer, 1, bytes_to_read, source);
        if (ret != bytes_to_read) {
            tr_debug("Could not fread on source file");
            fclose(destination);
            return ret;
        }

        ret = fwrite(buffer, 1, bytes_to_read, destination);
        if (ret != bytes_to_read) {
            tr_debug("Could not fwrite on destination file");
            fclose(destination);
            return ret;
        }

        bytes_left -= bytes_to_read;
    }

    fclose(destination);

    return 0;
}

#endif // _MBEDFRAG_COPY_PART_OF_FILE_H
