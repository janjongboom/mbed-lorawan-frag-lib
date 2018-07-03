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
#include "BlockDevice.h"
#include "FragmentationBlockDeviceWrapper.h"
#include "FragmentationMath.h"
#include "mbed_debug.h"

#include "mbed_trace.h"
#define TRACE_GROUP "FSES"

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
    size_t   FlashOffset;       // Place in flash where the final binary needs to be placed
} FragmentationSessionOpts_t;

enum FragResult {
    FRAG_OK,
    FRAG_SIZE_INCORRECT,
    FRAG_FLASH_WRITE_ERROR,
    FRAG_NO_MEMORY,
    FRAG_COMPLETE
};

/**
 * Sets up a fragmentation session
 */
class FragmentationSession {
public:
    /**
     * Start a fragmentation session
     * @param flash A block device that is wrapped for unaligned operations
     * @param opts  List of options for this session
     */
    FragmentationSession(FragmentationBlockDeviceWrapper* flash, FragmentationSessionOpts_t opts)
        : _flash(flash), _opts(opts),
          _math(flash, opts.NumberOfFragments, opts.FragmentSize, opts.RedundancyPackets, opts.FlashOffset),
          _frames_received(0)
    {
        tr_debug("FragmentationSession starting:");
        tr_debug("\tNumberOfFragments:   %d", opts.NumberOfFragments);
        tr_debug("\tFragmentSize:        %d", opts.FragmentSize);
        tr_debug("\tPadding:             %d", opts.Padding);
        tr_debug("\tMaxRedundancy:       %d", opts.RedundancyPackets);
        tr_debug("\tFlashOffset:         0x%x", opts.FlashOffset);
    }

    /**
     * Allocate the required buffers for the fragmentation session, and clears the flash pages required for the binary file.
     *
     * @returns FRAG_OK if succeeded,
     *          FRAG_NO_MEMORY if allocations failed,
     *          FRAG_FLASH_WRITE_ERROR if clearing the flash failed.
    */
    FragResult initialize() {
        if (_flash->init() != BD_ERROR_OK) {
            tr_warn("Could not initialize FragmentationBlockDeviceWrapper");
            return FRAG_NO_MEMORY;
        }

        // initialize the memory required for the Math module
        if (!_math.initialize()) {
            tr_warn("Could not initialize FragmentationMath");
            return FRAG_NO_MEMORY;
        }

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

        _frames_received++;

        // the first X packets contain the binary as-is... If that is the case, just store it in flash.
        // index is 1-based
        if (index <= _opts.NumberOfFragments) {
            int r = _flash->program(buffer, _opts.FlashOffset + ((index - 1) * size), size);
            if (r != 0) {
                return FRAG_FLASH_WRITE_ERROR;
            }

            _math.set_frame_found(index);

            if (index == _opts.NumberOfFragments && _math.get_lost_frame_count() == 0) {
                return FRAG_COMPLETE;
            }

            return FRAG_OK;
        }

        // redundancy packet coming in
        FragmentationMathSessionParams_t params;
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

    /**
     * Get number of frames received (in total)
     */
    uint16_t get_received_frame_count() {
        return _frames_received;
    }

    FragmentationSessionOpts_t get_options() {
        return _opts;
    }

private:
    FragmentationBlockDeviceWrapper* _flash;
    FragmentationSessionOpts_t _opts;
    FragmentationMath _math;

    uint16_t _frames_received;
};

#endif // _MBEDFRAG_FRAGMENTATION_SESSION_H
