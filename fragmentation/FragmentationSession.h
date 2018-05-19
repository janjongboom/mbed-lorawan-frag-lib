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

#ifndef _MBEDFRAG_FRAGMENTATION_SESSION_H
#define _MBEDFRAG_FRAGMENTATION_SESSION_H

#include "mbed.h"
#include "FragmentationMath.h"

#include "mbed-trace/mbed_trace.h"
#define TRACE_GROUP  "FSES"

/**
 * The binary is laid out like this:
 * First, the original binary, split by FragmentSize. Padding should indicate when the binary stops (offset from end of last fragment).
 * Then, redundency packets
 */
typedef struct {
    uint16_t NumberOfFragments; // Number of fragments required for the *initial* binary, not counting the redundancy packets
    uint8_t  FragmentSize;      // Size of each fragment in bytes, **without the fragindex**
    uint8_t  Padding;           // Bytes of padding after the last original fragment
    uint16_t RedundancyPackets; // Max. number of redundancy packets we'll receive
} FragmentationSessionOpts_t;

enum FragResult {
    FRAG_OK,
    FRAG_SIZE_INCORRECT,
    FRAG_FLASH_WRITE_ERROR,
    FRAG_NO_MEMORY,
    FRAG_INVALID_FILE_PTR,
    FRAG_COMPLETE
};

/**
 * Sets up a fragmentation session
 */
class FragmentationSession {
public:
    /**
     * Start a fragmentation session
     * @param file      File to be used to write the fragmentation session; will be set to the correct size in initialize()
     * @param opts      List of options for this session
     */
    FragmentationSession(FILE *file, FragmentationSessionOpts_t opts)
        : _file(file), _opts(opts),
          _math(opts.NumberOfFragments, opts.FragmentSize, opts.RedundancyPackets)
    {
        tr_debug("FragmentationSession starting:");
        tr_debug("\tNumberOfFragments:   %d", opts.NumberOfFragments);
        tr_debug("\tFragmentSize:        %d", opts.FragmentSize);
        tr_debug("\tPadding:             %d", opts.Padding);
        tr_debug("\tMaxRedundancy:       %d", opts.RedundancyPackets);
    }

    /**
     * Allocate the required buffers for the fragmentation session, and clears the flash pages required for the binary file.
     *
     * @returns FRAG_OK if succeeded,
     *          FRAG_INVALID_FILE_PTR if an invalid file pointer was passed in,
     *          FRAG_NO_MEMORY if allocations failed,
     *          FRAG_FLASH_WRITE_ERROR if clearing the flash failed.
    */
    FragResult initialize() {
        if (!_file) {
            return FRAG_INVALID_FILE_PTR;
        }

        // initialize the memory required for the Math module
        if (!_math.initialize(_file)) {
            tr_debug("Could not initialize FragmentationMath");
            return FRAG_NO_MEMORY;
        }

        // move to length-1
        const uint8_t zeros[128] = { 0 };

        // search for beginning, and then clear out the complete file...
        fseek(_file, 0, SEEK_SET);

        size_t bytes_left = (_opts.NumberOfFragments * _opts.FragmentSize);
        tr_debug("Clearing out file (%u bytes)", bytes_left);
        while (1) {
            if (bytes_left < sizeof(zeros)) {
                if (fwrite(zeros, 1, bytes_left, _file) != bytes_left) {
                    return FRAG_FLASH_WRITE_ERROR;
                }
                else {
                    // OK
                    break;
                }
            }

            if (fwrite(zeros, 1, sizeof(zeros), _file) != sizeof(zeros)) {
                return FRAG_FLASH_WRITE_ERROR;
            }

            bytes_left -= sizeof(zeros);
        }

        tr_debug("Cleared out file");

        fseek(_file, 0, SEEK_SET);

        return FRAG_OK;
    }

    /**
     * Process a fragmentation frame. Do **not** include the fragindex bytes
     * @param index The index of the frame
     * @param buffer The contents of the frame (without the fragindex bytes)
     * @param size The size of the buffer
     *
     * @returns FRAG_COMPLETE if the binary was reconstructed,
     *          FRAG_OK if the packet was processed, but the binary was not reconstructed,
     *          FRAG_FLASH_WRITE_ERROR if the packet could not be written to flash
     */
    FragResult process_frame(uint16_t index, uint8_t* buffer, size_t size) {
        if (size != _opts.FragmentSize) return FRAG_SIZE_INCORRECT;

        // the first X packets contain the binary as-is... If that is the case, just store it in flash.
        // index is 1-based
        if (index <= _opts.NumberOfFragments) {
            if (fseek(_file, ((index - 1) * size), SEEK_SET) != 0) {
                tr_debug("Could not fseek");
                return FRAG_FLASH_WRITE_ERROR;
            }

            if (fwrite(buffer, 1, size, _file) != size) {
                tr_debug("Could not fwrite");
                return FRAG_FLASH_WRITE_ERROR;
            }

            _math.set_frame_found(index);

            if (index == _opts.NumberOfFragments && _math.get_lost_frame_count() == 0) {
                return FRAG_COMPLETE;
            }

            return FRAG_OK;
        }

        // redundancy packet coming in
        FragmentationSessionParams_t params;
        params.NbOfFrag = _opts.NumberOfFragments;
        params.Redundancy = _opts.RedundancyPackets;
        params.DataSize = _opts.FragmentSize;
        int r = _math.process_redundant_frame(index, buffer, params);
        if (r != FRAG_SESSION_ONGOING) {
            return FRAG_COMPLETE;
        }

        return FRAG_OK;
    }

    /**
     * Convert a FragResult to a string
     */
    static const char* frag_result_string(FragResult result) {
        switch (result) {
            case FRAG_SIZE_INCORRECT: return "Fragment size incorrect";
            case FRAG_FLASH_WRITE_ERROR: return "Writing to flash failed";
            case FRAG_NO_MEMORY: return "Not enough space on the heap";
            case FRAG_INVALID_FILE_PTR: return "Invalid file pointer";
            case FRAG_COMPLETE: return "Complete";

            case FRAG_OK: return "OK";
            default: return "Unkown FragResult";
        }
    }

    /**
     * Get the number of lost fragments
     */
    int get_lost_frame_count() {
        return _math.get_lost_frame_count();
    }

private:
    FILE *_file;
    FragmentationSessionOpts_t _opts;
    FragmentationMath _math;
};

#endif // _MBEDFRAG_FRAGMENTATION_SESSION_H
