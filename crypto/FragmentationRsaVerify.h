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

#ifndef _FRAGMENTATION_RSA_DECRYPT_H_
#define _FRAGMENTATION_RSA_DECRYPT_H_

#include "mbed.h"
#include "mbedtls/pk.h"
#include "mbed_debug.h"

class FragmentationRsaVerify {
public:
    /**
     * Set up an RSA verification session
     * @param N public key modulus
     * @param E public key exponent
     */
    FragmentationRsaVerify(const char* N, const char* E) :
        n(N), e(E)
    {
    }

    /**
     * Decrypt an encrypted message
     * @param hash buffer holding the message digest (sha256 hash of the file)
     * @param signature  buffer holding the ciphertext (signature, signed with private key)
     */
    bool verify(const unsigned char* hash, const unsigned char* signature, size_t signature_size) {
        int ret;

        mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0);

        ret = mbedtls_mpi_read_string(&rsa.N, 16, n);
        if (ret != 0) return false;
        ret = mbedtls_mpi_read_string(&rsa.E, 16, e);
        if (ret != 0) return false;
        rsa.len = (mbedtls_mpi_bitlen( &rsa.N ) + 7) >> 3;

        if( signature_size != rsa.len )
        {
            debug("Invalid RSA signature format\n");
            return false;
        }

        ret = mbedtls_rsa_pkcs1_verify( &rsa, NULL, NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA256, 20, hash, signature );
        mbedtls_rsa_free(&rsa);

        return ret == 0;
    }

private:
    const char* n;
    const char* e;
    size_t publicKeySize;
    mbedtls_rsa_context rsa;
};

#endif // _FRAGMENTATION_RSA_DECRYPT_H_
