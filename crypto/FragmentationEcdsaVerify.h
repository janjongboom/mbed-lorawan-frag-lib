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

#ifndef _FRAGMENTATION_ECDSA_VERIFY_H_
#define _FRAGMENTATION_ECDSA_VERIFY_H_

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_ECDSA_C)

#include "mbed.h"
#include "mbedtls/pk.h"

#include "mbed-trace/mbed_trace.h"
#define TRACE_GROUP  "FECD"

class FragmentationEcdsaVerify {
public:
    /**
     * Set up an ECDSA verification session
     * @param aPubKey Public key in string format (starting with -----BEGIN PUBLIC KEY)
     * @param aPubKeySize Size of the public key
     */
    FragmentationEcdsaVerify(const char* aPubKey, size_t aPubKeySize) :
        pubKey((const unsigned char*)aPubKey), pubKeySize(aPubKeySize)
    {
    }

    /**
     * Decrypt an encrypted message
     * @param hash buffer holding the message digest (sha256 hash of the file)
     * @param signature  buffer holding the ciphertext (signature, signed with private key)
     * @param signature_size Length of the signature buffer
     */
    bool verify(const unsigned char* hash, unsigned char* signature, size_t signature_size) {
        int ret;

        mbedtls_pk_init(&pk);

        ret = mbedtls_pk_parse_public_key(&pk, pubKey, pubKeySize);
        if (ret != 0) {
            tr_debug("ECDSA failed to parse public key (-0x%04x)", ret);
            return false;
        }

        ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, 0, signature, signature_size);
        if (ret != 0) {
            tr_debug("ECDSA failed to verify message (-0x%04x)", ret);
            return false;
        }

        mbedtls_pk_free(&pk);

        return ret == 0;
    }

private:
    const unsigned char *pubKey;
    size_t pubKeySize;
    mbedtls_pk_context pk;
};

#endif // defined(MBEDTLS_ECDSA_C)

#endif // _FRAGMENTATION_ECDSA_VERIFY_H_
