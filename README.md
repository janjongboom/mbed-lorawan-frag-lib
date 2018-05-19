# LoRa Alliance Multicast Data Fragmentation Library for mbed OS 5

Implementation of Low-Density Parity-Check coding for forward error correction, plus crypto plugins to do verification of firmware updates. All files integrate with the Mbed File System interface (LittleFS or FATFS), to prevent loading large blobs into memory. Based on the work by ARM, The Things Network and Semtech.

* `fragmentation\FragmentationSession.h` - LDPC frontend.
* `fragmentation\FragmentationMath.h` - LDPC implementation.
* `crypto\FragmentationCrc64.h` - CRC64 implementation.
* `crypto\FragmentationSha256.h` - SHA256 implementation.
* `crypto\FragmentationRsaVerify.h` - RSA public key verification implementation.

## Usage

For a demonstration on using these classes to create a firmware update service with forward error correction, see [lorawan-fragmentation-in-flash](https://github.com/janjongboom/lorawan-fragmentation-in-flash).

## Memory usage

All memory is dynamically allocated on the heap, so you can unload heap objects when you start a data fragmentation session.

The amount of memory required for the algorithm depends on:

* Number of fragments required for a full file (without the redundancy packets) (`nbFrag`).
* The size of a data fragment (`fragSize`).
* The maximum number of redundancy frames that are expected (`nbRedundancy`).

Memory required can be calculated via:

```js
  ((nbRedundancy / 8) * nbRedundancy) // matrixM2B
+ (nbFrag * 2)                        // missingFrameIx
+ (nbFrag)                            // matrixRow
+ (fragSize)                          // matrixDataTemp
+ (nbRedundancy * 3)                  // tempVector, tempVector2 and s
```

For a 100K firmware image, split in 201 byte fragments with 200 redundancy packets this comes down to ~7.331 bytes:

```js
fragSize = 201;
nbFrag = (100 * 1024 / fragSize | 0) + 1;
nbRedundancy = 200;

// ((nbRedundancy / 8) * nbRedundancy) + (nbFrag*2) + (nbFrag) + (fragSize) + (nbRedundancy * 3)
// 7331 bytes
```

In addition:

* Some small allocations are made when xor'ing lines while applying the redundant packets.
* The FragmentationSession and FragmentationMath objects take up some space as well.
* Your flash driver probably needs to allocate a buffer the size of it's page size (unless memory is directly addressable).

On a Multi-Tech xDot you probably want to limit the number of redundancy frames to <100, given that an xDot running the Dot-Examples OTA_EXAMPLE has 7040 bytes of free heap space available.

## License

* crc.h is derived from work by Salvatore Sanfilippo.
* FragmentationMath.h is derived from work by Semtech Inc.

All other files are licensed under Apache 2.0 license. See LICENSE file for full license.
