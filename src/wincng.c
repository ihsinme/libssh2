/*
 * Copyright (C) 2013-2020 Marc Hoersken <info@marc-hoersken.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 *   Redistributions of source code must retain the above
 *   copyright notice, this list of conditions and the
 *   following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials
 *   provided with the distribution.
 *
 *   Neither the name of the copyright holder nor the names
 *   of any other contributors may be used to endorse or
 *   promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#include "libssh2_priv.h"

#ifdef LIBSSH2_WINCNG /* compile only if we build with wincng */

/* required for cross-compilation against the w64 mingw-runtime package */
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < 0x0600)
#undef _WIN32_WINNT
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

/* specify the required libraries for dependencies using MSVC */
#ifdef _MSC_VER
#pragma comment(lib, "bcrypt.lib")
#ifdef HAVE_LIBCRYPT32
#pragma comment(lib, "crypt32.lib")
#endif
#endif

#include <windows.h>
#include <bcrypt.h>
#include <ntstatus.h>
#include <math.h>
#include "misc.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_LIBCRYPT32
#include <wincrypt.h>
#endif

#define PEM_RSA_HEADER "-----BEGIN RSA PRIVATE KEY-----"
#define PEM_RSA_FOOTER "-----END RSA PRIVATE KEY-----"
#define PEM_DSA_HEADER "-----BEGIN DSA PRIVATE KEY-----"
#define PEM_DSA_FOOTER "-----END DSA PRIVATE KEY-----"


/*******************************************************************/
/*
 * Windows CNG backend: Missing definitions (for MinGW[-w64])
 */
#ifndef BCRYPT_SUCCESS
#define BCRYPT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#ifndef BCRYPT_RNG_ALGORITHM
#define BCRYPT_RNG_ALGORITHM L"RNG"
#endif

#ifndef BCRYPT_MD5_ALGORITHM
#define BCRYPT_MD5_ALGORITHM L"MD5"
#endif

#ifndef BCRYPT_SHA1_ALGORITHM
#define BCRYPT_SHA1_ALGORITHM L"SHA1"
#endif

#ifndef BCRYPT_SHA256_ALGORITHM
#define BCRYPT_SHA256_ALGORITHM L"SHA256"
#endif

#ifndef BCRYPT_SHA384_ALGORITHM
#define BCRYPT_SHA384_ALGORITHM L"SHA384"
#endif

#ifndef BCRYPT_SHA512_ALGORITHM
#define BCRYPT_SHA512_ALGORITHM L"SHA512"
#endif

#ifndef BCRYPT_RSA_ALGORITHM
#define BCRYPT_RSA_ALGORITHM L"RSA"
#endif

#ifndef BCRYPT_DSA_ALGORITHM
#define BCRYPT_DSA_ALGORITHM L"DSA"
#endif

#ifndef BCRYPT_AES_ALGORITHM
#define BCRYPT_AES_ALGORITHM L"AES"
#endif

#ifndef BCRYPT_RC4_ALGORITHM
#define BCRYPT_RC4_ALGORITHM L"RC4"
#endif

#ifndef BCRYPT_3DES_ALGORITHM
#define BCRYPT_3DES_ALGORITHM L"3DES"
#endif

#ifndef BCRYPT_DH_ALGORITHM
#define BCRYPT_DH_ALGORITHM L"DH"
#endif

/* BCRYPT_KDF_RAW_SECRET is available from Windows 8.1 and onwards */
#ifndef BCRYPT_KDF_RAW_SECRET
#define BCRYPT_KDF_RAW_SECRET L"TRUNCATE"
#endif

#ifndef BCRYPT_ALG_HANDLE_HMAC_FLAG
#define BCRYPT_ALG_HANDLE_HMAC_FLAG 0x00000008
#endif

#ifndef BCRYPT_DSA_PUBLIC_BLOB
#define BCRYPT_DSA_PUBLIC_BLOB L"DSAPUBLICBLOB"
#endif

#ifndef BCRYPT_DSA_PUBLIC_MAGIC
#define BCRYPT_DSA_PUBLIC_MAGIC 0x42505344 /* DSPB */
#endif

#ifndef BCRYPT_DSA_PRIVATE_BLOB
#define BCRYPT_DSA_PRIVATE_BLOB L"DSAPRIVATEBLOB"
#endif

#ifndef BCRYPT_DSA_PRIVATE_MAGIC
#define BCRYPT_DSA_PRIVATE_MAGIC 0x56505344 /* DSPV */
#endif

#ifndef BCRYPT_RSAPUBLIC_BLOB
#define BCRYPT_RSAPUBLIC_BLOB L"RSAPUBLICBLOB"
#endif

#ifndef BCRYPT_RSAPUBLIC_MAGIC
#define BCRYPT_RSAPUBLIC_MAGIC 0x31415352 /* RSA1 */
#endif

#ifndef BCRYPT_RSAFULLPRIVATE_BLOB
#define BCRYPT_RSAFULLPRIVATE_BLOB L"RSAFULLPRIVATEBLOB"
#endif

#ifndef BCRYPT_RSAFULLPRIVATE_MAGIC
#define BCRYPT_RSAFULLPRIVATE_MAGIC 0x33415352 /* RSA3 */
#endif

#ifndef BCRYPT_KEY_DATA_BLOB
#define BCRYPT_KEY_DATA_BLOB L"KeyDataBlob"
#endif

#ifndef BCRYPT_MESSAGE_BLOCK_LENGTH
#define BCRYPT_MESSAGE_BLOCK_LENGTH L"MessageBlockLength"
#endif

#ifndef BCRYPT_NO_KEY_VALIDATION
#define BCRYPT_NO_KEY_VALIDATION 0x00000008
#endif

#ifndef BCRYPT_BLOCK_PADDING
#define BCRYPT_BLOCK_PADDING 0x00000001
#endif

#ifndef BCRYPT_PAD_NONE
#define BCRYPT_PAD_NONE 0x00000001
#endif

#ifndef BCRYPT_PAD_PKCS1
#define BCRYPT_PAD_PKCS1 0x00000002
#endif

#ifndef BCRYPT_PAD_OAEP
#define BCRYPT_PAD_OAEP 0x00000004
#endif

#ifndef BCRYPT_PAD_PSS
#define BCRYPT_PAD_PSS 0x00000008
#endif

#ifndef CRYPT_STRING_ANY
#define CRYPT_STRING_ANY 0x00000007
#endif

#ifndef LEGACY_RSAPRIVATE_BLOB
#define LEGACY_RSAPRIVATE_BLOB L"CAPIPRIVATEBLOB"
#endif

#ifndef PKCS_RSA_PRIVATE_KEY
#define PKCS_RSA_PRIVATE_KEY (LPCSTR)43
#endif


/*******************************************************************/
/*
 * Windows CNG backend: Generic functions
 */

struct _libssh2_wincng_ctx _libssh2_wincng;

void
_libssh2_wincng_init(void)
{
    int ret;

    memset(&_libssh2_wincng, 0, sizeof(_libssh2_wincng));

    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgRNG,
                                      BCRYPT_RNG_ALGORITHM, NULL, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgRNG = NULL;
    }

    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHashMD5,
                                      BCRYPT_MD5_ALGORITHM, NULL, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgHashMD5 = NULL;
    }
    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHashSHA1,
                                      BCRYPT_SHA1_ALGORITHM, NULL, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgHashSHA1 = NULL;
    }
    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHashSHA256,
                                      BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgHashSHA256 = NULL;
    }
    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHashSHA384,
                                      BCRYPT_SHA384_ALGORITHM, NULL, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgHashSHA384 = NULL;
    }
    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHashSHA512,
                                      BCRYPT_SHA512_ALGORITHM, NULL, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgHashSHA512 = NULL;
    }

    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHmacMD5,
                                      BCRYPT_MD5_ALGORITHM, NULL,
                                      BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgHmacMD5 = NULL;
    }
    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHmacSHA1,
                                      BCRYPT_SHA1_ALGORITHM, NULL,
                                      BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgHmacSHA1 = NULL;
    }
    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHmacSHA256,
                                      BCRYPT_SHA256_ALGORITHM, NULL,
                                      BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgHmacSHA256 = NULL;
    }
    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHmacSHA384,
                                      BCRYPT_SHA384_ALGORITHM, NULL,
                                      BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgHmacSHA384 = NULL;
    }
    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHmacSHA512,
                                      BCRYPT_SHA512_ALGORITHM, NULL,
                                      BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgHmacSHA512 = NULL;
    }

    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgRSA,
                                      BCRYPT_RSA_ALGORITHM, NULL, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgRSA = NULL;
    }
    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgDSA,
                                      BCRYPT_DSA_ALGORITHM, NULL, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgDSA = NULL;
    }

    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgAES_CBC,
                                      BCRYPT_AES_ALGORITHM, NULL, 0);
    if(BCRYPT_SUCCESS(ret)) {
        ret = BCryptSetProperty(_libssh2_wincng.hAlgAES_CBC,
                                BCRYPT_CHAINING_MODE,
                                (PBYTE)BCRYPT_CHAIN_MODE_CBC,
                                sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
        if(!BCRYPT_SUCCESS(ret)) {
            ret = BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgAES_CBC, 0);
            if(BCRYPT_SUCCESS(ret)) {
                _libssh2_wincng.hAlgAES_CBC = NULL;
            }
        }
    }

    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgAES_ECB,
                                      BCRYPT_AES_ALGORITHM, NULL, 0);
    if(BCRYPT_SUCCESS(ret)) {
        ret = BCryptSetProperty(_libssh2_wincng.hAlgAES_ECB,
                                BCRYPT_CHAINING_MODE,
                                (PBYTE)BCRYPT_CHAIN_MODE_ECB,
                                sizeof(BCRYPT_CHAIN_MODE_ECB), 0);
        if(!BCRYPT_SUCCESS(ret)) {
            ret = BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgAES_ECB, 0);
            if(BCRYPT_SUCCESS(ret)) {
                _libssh2_wincng.hAlgAES_ECB = NULL;
            }
        }
    }

    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgRC4_NA,
                                      BCRYPT_RC4_ALGORITHM, NULL, 0);
    if(BCRYPT_SUCCESS(ret)) {
        ret = BCryptSetProperty(_libssh2_wincng.hAlgRC4_NA,
                                BCRYPT_CHAINING_MODE,
                                (PBYTE)BCRYPT_CHAIN_MODE_NA,
                                sizeof(BCRYPT_CHAIN_MODE_NA), 0);
        if(!BCRYPT_SUCCESS(ret)) {
            ret = BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgRC4_NA, 0);
            if(BCRYPT_SUCCESS(ret)) {
                _libssh2_wincng.hAlgRC4_NA = NULL;
            }
        }
    }

    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlg3DES_CBC,
                                      BCRYPT_3DES_ALGORITHM, NULL, 0);
    if(BCRYPT_SUCCESS(ret)) {
        ret = BCryptSetProperty(_libssh2_wincng.hAlg3DES_CBC,
                                BCRYPT_CHAINING_MODE,
                                (PBYTE)BCRYPT_CHAIN_MODE_CBC,
                                sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
        if(!BCRYPT_SUCCESS(ret)) {
            ret = BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlg3DES_CBC,
                                               0);
            if(BCRYPT_SUCCESS(ret)) {
                _libssh2_wincng.hAlg3DES_CBC = NULL;
            }
        }
    }

    ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgDH,
                                      BCRYPT_DH_ALGORITHM, NULL, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng.hAlgDH = NULL;
    }
}

void
_libssh2_wincng_free(void)
{
    if(_libssh2_wincng.hAlgRNG)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgRNG, 0);
    if(_libssh2_wincng.hAlgHashMD5)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHashMD5, 0);
    if(_libssh2_wincng.hAlgHashSHA1)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHashSHA1, 0);
    if(_libssh2_wincng.hAlgHashSHA256)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHashSHA256, 0);
    if(_libssh2_wincng.hAlgHashSHA384)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHashSHA384, 0);
    if(_libssh2_wincng.hAlgHashSHA512)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHashSHA512, 0);
    if(_libssh2_wincng.hAlgHmacMD5)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHmacMD5, 0);
    if(_libssh2_wincng.hAlgHmacSHA1)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHmacSHA1, 0);
    if(_libssh2_wincng.hAlgHmacSHA256)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHmacSHA256, 0);
    if(_libssh2_wincng.hAlgHmacSHA384)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHmacSHA384, 0);
    if(_libssh2_wincng.hAlgHmacSHA512)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHmacSHA512, 0);
    if(_libssh2_wincng.hAlgRSA)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgRSA, 0);
    if(_libssh2_wincng.hAlgDSA)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgDSA, 0);
    if(_libssh2_wincng.hAlgAES_CBC)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgAES_CBC, 0);
    if(_libssh2_wincng.hAlgRC4_NA)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgRC4_NA, 0);
    if(_libssh2_wincng.hAlg3DES_CBC)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlg3DES_CBC, 0);
    if(_libssh2_wincng.hAlgDH)
        (void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgDH, 0);

    memset(&_libssh2_wincng, 0, sizeof(_libssh2_wincng));
}

int
_libssh2_wincng_random(void *buf, int len)
{
    int ret;

    ret = BCryptGenRandom(_libssh2_wincng.hAlgRNG, buf, len, 0);

    return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

static void
_libssh2_wincng_safe_free(void *buf, int len)
{
#ifndef LIBSSH2_CLEAR_MEMORY
    (void)len;
#endif

    if(!buf)
        return;

#ifdef LIBSSH2_CLEAR_MEMORY
    if(len > 0)
        SecureZeroMemory(buf, len);
#endif

    free(buf);
}

/* Copy a big endian set of bits from src to dest.
 * if the size of src is smaller than dest then pad the "left" (MSB)
 * end with zeroes and copy the bits into the "right" (LSB) end. */
static void
memcpy_with_be_padding(unsigned char *dest, unsigned long dest_len,
                       unsigned char *src, unsigned long src_len)
{
    if(dest_len > src_len) {
        memset(dest, 0, dest_len - src_len);
    }
    memcpy((dest + dest_len) - src_len, src, src_len);
}

static int
round_down(int number, int multiple)
{
    return (number / multiple) * multiple;
}

/*******************************************************************/
/*
 * Windows CNG backend: Hash functions
 */

int
_libssh2_wincng_hash_init(_libssh2_wincng_hash_ctx *ctx,
                          BCRYPT_ALG_HANDLE hAlg, unsigned long hashlen,
                          unsigned char *key, unsigned long keylen)
{
    BCRYPT_HASH_HANDLE hHash;
    unsigned char *pbHashObject;
    unsigned long dwHashObject, dwHash, cbData;
    int ret;

    ret = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH,
                            (unsigned char *)&dwHash,
                            sizeof(dwHash),
                            &cbData, 0);
    if((!BCRYPT_SUCCESS(ret)) || dwHash != hashlen) {
        return -1;
    }

    ret = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH,
                            (unsigned char *)&dwHashObject,
                            sizeof(dwHashObject),
                            &cbData, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        return -1;
    }

    pbHashObject = malloc(dwHashObject);
    if(!pbHashObject) {
        return -1;
    }


    ret = BCryptCreateHash(hAlg, &hHash,
                           pbHashObject, dwHashObject,
                           key, keylen, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng_safe_free(pbHashObject, dwHashObject);
        return -1;
    }


    ctx->hHash = hHash;
    ctx->pbHashObject = pbHashObject;
    ctx->dwHashObject = dwHashObject;
    ctx->cbHash = dwHash;

    return 0;
}

int
_libssh2_wincng_hash_update(_libssh2_wincng_hash_ctx *ctx,
                            const unsigned char *data, unsigned long datalen)
{
    int ret;

    ret = BCryptHashData(ctx->hHash, (unsigned char *)data, datalen, 0);

    return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

int
_libssh2_wincng_hash_final(_libssh2_wincng_hash_ctx *ctx,
                           unsigned char *hash)
{
    int ret;

    ret = BCryptFinishHash(ctx->hHash, hash, ctx->cbHash, 0);

    BCryptDestroyHash(ctx->hHash);
    ctx->hHash = NULL;

    _libssh2_wincng_safe_free(ctx->pbHashObject, ctx->dwHashObject);
    ctx->pbHashObject = NULL;
    ctx->dwHashObject = 0;

    return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

int
_libssh2_wincng_hash(unsigned char *data, unsigned long datalen,
                     BCRYPT_ALG_HANDLE hAlg,
                     unsigned char *hash, unsigned long hashlen)
{
    _libssh2_wincng_hash_ctx ctx;
    int ret;

    ret = _libssh2_wincng_hash_init(&ctx, hAlg, hashlen, NULL, 0);
    if(!ret) {
        ret = _libssh2_wincng_hash_update(&ctx, data, datalen);
        ret |= _libssh2_wincng_hash_final(&ctx, hash);
    }

    return ret;
}


/*******************************************************************/
/*
 * Windows CNG backend: HMAC functions
 */

int
_libssh2_wincng_hmac_final(_libssh2_wincng_hash_ctx *ctx,
                           unsigned char *hash)
{
    int ret;

    ret = BCryptFinishHash(ctx->hHash, hash, ctx->cbHash, 0);

    return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

void
_libssh2_wincng_hmac_cleanup(_libssh2_wincng_hash_ctx *ctx)
{
    BCryptDestroyHash(ctx->hHash);
    ctx->hHash = NULL;

    _libssh2_wincng_safe_free(ctx->pbHashObject, ctx->dwHashObject);
    ctx->pbHashObject = NULL;
    ctx->dwHashObject = 0;
}


/*******************************************************************/
/*
 * Windows CNG backend: Key functions
 */

int
_libssh2_wincng_key_sha_verify(_libssh2_wincng_key_ctx *ctx,
                                size_t hashlen,
                                const unsigned char *sig,
                                unsigned long sig_len,
                                const unsigned char *m,
                                unsigned long m_len,
                                unsigned long flags)
{
    BCRYPT_PKCS1_PADDING_INFO paddingInfoPKCS1;
    BCRYPT_ALG_HANDLE hAlgHash;
    void *pPaddingInfo;
    unsigned char *data, *hash;
    unsigned long datalen;
    int ret;

    if(hashlen == SHA_DIGEST_LENGTH) {
        hAlgHash = _libssh2_wincng.hAlgHashSHA1;
        paddingInfoPKCS1.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    }
    else if(hashlen == SHA256_DIGEST_LENGTH) {
        hAlgHash = _libssh2_wincng.hAlgHashSHA256;
        paddingInfoPKCS1.pszAlgId = BCRYPT_SHA256_ALGORITHM;
    }
    else if(hashlen == SHA384_DIGEST_LENGTH) {
        hAlgHash = _libssh2_wincng.hAlgHashSHA384;
        paddingInfoPKCS1.pszAlgId = BCRYPT_SHA384_ALGORITHM;
    }
    else if(hashlen == SHA512_DIGEST_LENGTH) {
        hAlgHash = _libssh2_wincng.hAlgHashSHA512;
        paddingInfoPKCS1.pszAlgId = BCRYPT_SHA512_ALGORITHM;
    }
    else {
        return -1;
    }

    datalen = m_len;
    data = malloc(datalen);
    if(!data) {
        return -1;
    }

    hash = malloc(hashlen);
    if(!hash) {
        free(data);
        return -1;
    }
    memcpy(data, m, datalen);

    ret = _libssh2_wincng_hash(data, datalen,
                               hAlgHash,
                               hash, hashlen);
    _libssh2_wincng_safe_free(data, datalen);

    if(ret) {
        _libssh2_wincng_safe_free(hash, hashlen);
        return -1;
    }

    datalen = sig_len;
    data = malloc(datalen);
    if(!data) {
        _libssh2_wincng_safe_free(hash, hashlen);
        return -1;
    }

    if(flags & BCRYPT_PAD_PKCS1) {
        pPaddingInfo = &paddingInfoPKCS1;
    }
    else
        pPaddingInfo = NULL;

    memcpy(data, sig, datalen);

    ret = BCryptVerifySignature(ctx->hKey, pPaddingInfo,
                                hash, hashlen, data, datalen, flags);

    _libssh2_wincng_safe_free(hash, hashlen);
    _libssh2_wincng_safe_free(data, datalen);

    return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

#ifdef HAVE_LIBCRYPT32
static int
_libssh2_wincng_load_pem(LIBSSH2_SESSION *session,
                         const char *filename,
                         const unsigned char *passphrase,
                         const char *headerbegin,
                         const char *headerend,
                         unsigned char **data,
                         unsigned int *datalen)
{
    FILE *fp;
    int ret;

    fp = fopen(filename, FOPEN_READTEXT);
    if(!fp) {
        return -1;
    }

    ret = _libssh2_pem_parse(session, headerbegin, headerend,
                             passphrase,
                             fp, data, datalen);

    fclose(fp);

    return ret;
}

static int
_libssh2_wincng_load_private(LIBSSH2_SESSION *session,
                             const char *filename,
                             const unsigned char *passphrase,
                             unsigned char **ppbEncoded,
                             unsigned long *pcbEncoded,
                             int tryLoadRSA, int tryLoadDSA)
{
    unsigned char *data = NULL;
    unsigned int datalen = 0;
    int ret = -1;

    if(ret && tryLoadRSA) {
        ret = _libssh2_wincng_load_pem(session, filename, passphrase,
                                       PEM_RSA_HEADER, PEM_RSA_FOOTER,
                                       &data, &datalen);
    }

    if(ret && tryLoadDSA) {
        ret = _libssh2_wincng_load_pem(session, filename, passphrase,
                                       PEM_DSA_HEADER, PEM_DSA_FOOTER,
                                       &data, &datalen);
    }

    if(!ret) {
        *ppbEncoded = data;
        *pcbEncoded = datalen;
    }

    return ret;
}

static int
_libssh2_wincng_load_private_memory(LIBSSH2_SESSION *session,
                                    const char *privatekeydata,
                                    size_t privatekeydata_len,
                                    const unsigned char *passphrase,
                                    unsigned char **ppbEncoded,
                                    unsigned long *pcbEncoded,
                                    int tryLoadRSA, int tryLoadDSA)
{
    unsigned char *data = NULL;
    unsigned int datalen = 0;
    int ret = -1;

    (void)passphrase;

    if(ret && tryLoadRSA) {
        ret = _libssh2_pem_parse_memory(session,
                                        PEM_RSA_HEADER, PEM_RSA_FOOTER,
                                        privatekeydata, privatekeydata_len,
                                        &data, &datalen);
    }

    if(ret && tryLoadDSA) {
        ret = _libssh2_pem_parse_memory(session,
                                        PEM_DSA_HEADER, PEM_DSA_FOOTER,
                                        privatekeydata, privatekeydata_len,
                                        &data, &datalen);
    }

    if(!ret) {
        *ppbEncoded = data;
        *pcbEncoded = datalen;
    }

    return ret;
}

static int
_libssh2_wincng_asn_decode(unsigned char *pbEncoded,
                           unsigned long cbEncoded,
                           LPCSTR lpszStructType,
                           unsigned char **ppbDecoded,
                           unsigned long *pcbDecoded)
{
    unsigned char *pbDecoded = NULL;
    unsigned long cbDecoded = 0;
    int ret;

    ret = CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                              lpszStructType,
                              pbEncoded, cbEncoded, 0, NULL,
                              NULL, &cbDecoded);
    if(!ret) {
        return -1;
    }

    pbDecoded = malloc(cbDecoded);
    if(!pbDecoded) {
        return -1;
    }

    ret = CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                              lpszStructType,
                              pbEncoded, cbEncoded, 0, NULL,
                              pbDecoded, &cbDecoded);
    if(!ret) {
        _libssh2_wincng_safe_free(pbDecoded, cbDecoded);
        return -1;
    }


    *ppbDecoded = pbDecoded;
    *pcbDecoded = cbDecoded;

    return 0;
}

static int
_libssh2_wincng_bn_ltob(unsigned char *pbInput,
                        unsigned long cbInput,
                        unsigned char **ppbOutput,
                        unsigned long *pcbOutput)
{
    unsigned char *pbOutput;
    unsigned long cbOutput, index, offset, length;

    if(cbInput < 1) {
        return 0;
    }

    offset = 0;
    length = cbInput - 1;
    cbOutput = cbInput;
    if(pbInput[length] & (1 << 7)) {
        offset++;
        cbOutput += offset;
    }

    pbOutput = (unsigned char *)malloc(cbOutput);
    if(!pbOutput) {
        return -1;
    }

    pbOutput[0] = 0;
    for(index = 0; ((index + offset) < cbOutput)
                    && (index < cbInput); index++) {
        pbOutput[index + offset] = pbInput[length - index];
    }


    *ppbOutput = pbOutput;
    *pcbOutput = cbOutput;

    return 0;
}

static int
_libssh2_wincng_asn_decode_bn(unsigned char *pbEncoded,
                              unsigned long cbEncoded,
                              unsigned char **ppbDecoded,
                              unsigned long *pcbDecoded)
{
    unsigned char *pbDecoded = NULL, *pbInteger;
    unsigned long cbDecoded = 0, cbInteger;
    int ret;

    ret = _libssh2_wincng_asn_decode(pbEncoded, cbEncoded,
                                     X509_MULTI_BYTE_UINT,
                                     &pbInteger, &cbInteger);
    if(!ret) {
        ret = _libssh2_wincng_bn_ltob(((PCRYPT_DATA_BLOB)pbInteger)->pbData,
                                      ((PCRYPT_DATA_BLOB)pbInteger)->cbData,
                                      &pbDecoded, &cbDecoded);
        if(!ret) {
            *ppbDecoded = pbDecoded;
            *pcbDecoded = cbDecoded;
        }
        _libssh2_wincng_safe_free(pbInteger, cbInteger);
    }

    return ret;
}

static int
_libssh2_wincng_asn_decode_bns(unsigned char *pbEncoded,
                               unsigned long cbEncoded,
                               unsigned char ***prpbDecoded,
                               unsigned long **prcbDecoded,
                               unsigned long *pcbCount)
{
    PCRYPT_DER_BLOB pBlob;
    unsigned char *pbDecoded, **rpbDecoded;
    unsigned long cbDecoded, *rcbDecoded, index, length;
    int ret;

    ret = _libssh2_wincng_asn_decode(pbEncoded, cbEncoded,
                                     X509_SEQUENCE_OF_ANY,
                                     &pbDecoded, &cbDecoded);
    if(!ret) {
        length = ((PCRYPT_DATA_BLOB)pbDecoded)->cbData;

        rpbDecoded = malloc(sizeof(PBYTE) * length);
        if(rpbDecoded) {
            rcbDecoded = malloc(sizeof(DWORD) * length);
            if(rcbDecoded) {
                for(index = 0; index < length; index++) {
                    pBlob = &((PCRYPT_DER_BLOB)
                              ((PCRYPT_DATA_BLOB)pbDecoded)->pbData)[index];
                    ret = _libssh2_wincng_asn_decode_bn(pBlob->pbData,
                                                        pBlob->cbData,
                                                        &rpbDecoded[index],
                                                        &rcbDecoded[index]);
                    if(ret)
                        break;
                }

                if(!ret) {
                    *prpbDecoded = rpbDecoded;
                    *prcbDecoded = rcbDecoded;
                    *pcbCount = length;
                }
                else {
                    for(length = 0; length < index; length++) {
                        _libssh2_wincng_safe_free(rpbDecoded[length],
                                                  rcbDecoded[length]);
                        rpbDecoded[length] = NULL;
                        rcbDecoded[length] = 0;
                    }
                    free(rpbDecoded);
                    free(rcbDecoded);
                }
            }
            else {
                free(rpbDecoded);
                ret = -1;
            }
        }
        else {
            ret = -1;
        }

        _libssh2_wincng_safe_free(pbDecoded, cbDecoded);
    }

    return ret;
}
#endif /* HAVE_LIBCRYPT32 */

static unsigned long
_libssh2_wincng_bn_size(const unsigned char *bignum,
                        unsigned long length)
{
    unsigned long offset;

    if(!bignum)
        return 0;

    length--;

    offset = 0;
    while(!(*(bignum + offset)) && (offset < length))
        offset++;

    length++;

    return length - offset;
}


/*******************************************************************/
/*
 * Windows CNG backend: RSA functions
 */

int
_libssh2_wincng_rsa_new(libssh2_rsa_ctx **rsa,
                        const unsigned char *edata,
                        unsigned long elen,
                        const unsigned char *ndata,
                        unsigned long nlen,
                        const unsigned char *ddata,
                        unsigned long dlen,
                        const unsigned char *pdata,
                        unsigned long plen,
                        const unsigned char *qdata,
                        unsigned long qlen,
                        const unsigned char *e1data,
                        unsigned long e1len,
                        const unsigned char *e2data,
                        unsigned long e2len,
                        const unsigned char *coeffdata,
                        unsigned long coefflen)
{
    BCRYPT_KEY_HANDLE hKey;
    BCRYPT_RSAKEY_BLOB *rsakey;
    LPCWSTR lpszBlobType;
    unsigned char *key;
    unsigned long keylen, offset, mlen, p1len = 0, p2len = 0;
    int ret;

    mlen = max(_libssh2_wincng_bn_size(ndata, nlen),
               _libssh2_wincng_bn_size(ddata, dlen));
    offset = sizeof(BCRYPT_RSAKEY_BLOB);
    keylen = offset + elen + mlen;
    if(ddata && dlen > 0) {
        p1len = max(_libssh2_wincng_bn_size(pdata, plen),
                    _libssh2_wincng_bn_size(e1data, e1len));
        p2len = max(_libssh2_wincng_bn_size(qdata, qlen),
                    _libssh2_wincng_bn_size(e2data, e2len));
        keylen += p1len * 3 + p2len * 2 + mlen;
    }

    key = malloc(keylen);
    if(!key) {
        return -1;
    }

    memset(key, 0, keylen);


    /* https://msdn.microsoft.com/library/windows/desktop/aa375531.aspx */
    rsakey = (BCRYPT_RSAKEY_BLOB *)key;
    rsakey->BitLength = mlen * 8;
    rsakey->cbPublicExp = elen;
    rsakey->cbModulus = mlen;

    memcpy(key + offset, edata, elen);
    offset += elen;

    if(nlen < mlen)
        memcpy(key + offset + mlen - nlen, ndata, nlen);
    else
        memcpy(key + offset, ndata + nlen - mlen, mlen);

    if(ddata && dlen > 0) {
        offset += mlen;

        if(plen < p1len)
            memcpy(key + offset + p1len - plen, pdata, plen);
        else
            memcpy(key + offset, pdata + plen - p1len, p1len);
        offset += p1len;

        if(qlen < p2len)
            memcpy(key + offset + p2len - qlen, qdata, qlen);
        else
            memcpy(key + offset, qdata + qlen - p2len, p2len);
        offset += p2len;

        if(e1len < p1len)
            memcpy(key + offset + p1len - e1len, e1data, e1len);
        else
            memcpy(key + offset, e1data + e1len - p1len, p1len);
        offset += p1len;

        if(e2len < p2len)
            memcpy(key + offset + p2len - e2len, e2data, e2len);
        else
            memcpy(key + offset, e2data + e2len - p2len, p2len);
        offset += p2len;

        if(coefflen < p1len)
            memcpy(key + offset + p1len - coefflen, coeffdata, coefflen);
        else
            memcpy(key + offset, coeffdata + coefflen - p1len, p1len);
        offset += p1len;

        if(dlen < mlen)
            memcpy(key + offset + mlen - dlen, ddata, dlen);
        else
            memcpy(key + offset, ddata + dlen - mlen, mlen);

        lpszBlobType = BCRYPT_RSAFULLPRIVATE_BLOB;
        rsakey->Magic = BCRYPT_RSAFULLPRIVATE_MAGIC;
        rsakey->cbPrime1 = p1len;
        rsakey->cbPrime2 = p2len;
    }
    else {
        lpszBlobType = BCRYPT_RSAPUBLIC_BLOB;
        rsakey->Magic = BCRYPT_RSAPUBLIC_MAGIC;
        rsakey->cbPrime1 = 0;
        rsakey->cbPrime2 = 0;
    }


    ret = BCryptImportKeyPair(_libssh2_wincng.hAlgRSA, NULL, lpszBlobType,
                              &hKey, key, keylen, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng_safe_free(key, keylen);
        return -1;
    }


    *rsa = malloc(sizeof(libssh2_rsa_ctx));
    if(!(*rsa)) {
        BCryptDestroyKey(hKey);
        _libssh2_wincng_safe_free(key, keylen);
        return -1;
    }

    (*rsa)->hKey = hKey;
    (*rsa)->pbKeyObject = key;
    (*rsa)->cbKeyObject = keylen;

    return 0;
}

#ifdef HAVE_LIBCRYPT32
static int
_libssh2_wincng_rsa_new_private_parse(libssh2_rsa_ctx **rsa,
                                      LIBSSH2_SESSION *session,
                                      unsigned char *pbEncoded,
                                      unsigned long cbEncoded)
{
    BCRYPT_KEY_HANDLE hKey;
    unsigned char *pbStructInfo;
    unsigned long cbStructInfo;
    int ret;

    (void)session;

    ret = _libssh2_wincng_asn_decode(pbEncoded, cbEncoded,
                                     PKCS_RSA_PRIVATE_KEY,
                                     &pbStructInfo, &cbStructInfo);

    _libssh2_wincng_safe_free(pbEncoded, cbEncoded);

    if(ret) {
        return -1;
    }


    ret = BCryptImportKeyPair(_libssh2_wincng.hAlgRSA, NULL,
                              LEGACY_RSAPRIVATE_BLOB, &hKey,
                              pbStructInfo, cbStructInfo, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng_safe_free(pbStructInfo, cbStructInfo);
        return -1;
    }


    *rsa = malloc(sizeof(libssh2_rsa_ctx));
    if(!(*rsa)) {
        BCryptDestroyKey(hKey);
        _libssh2_wincng_safe_free(pbStructInfo, cbStructInfo);
        return -1;
    }

    (*rsa)->hKey = hKey;
    (*rsa)->pbKeyObject = pbStructInfo;
    (*rsa)->cbKeyObject = cbStructInfo;

    return 0;
}
#endif /* HAVE_LIBCRYPT32 */

int
_libssh2_wincng_rsa_new_private(libssh2_rsa_ctx **rsa,
                                LIBSSH2_SESSION *session,
                                const char *filename,
                                const unsigned char *passphrase)
{
#ifdef HAVE_LIBCRYPT32
    unsigned char *pbEncoded;
    unsigned long cbEncoded;
    int ret;

    (void)session;

    ret = _libssh2_wincng_load_private(session, filename, passphrase,
                                       &pbEncoded, &cbEncoded, 1, 0);
    if(ret) {
        return -1;
    }

    return _libssh2_wincng_rsa_new_private_parse(rsa, session,
                                                 pbEncoded, cbEncoded);
#else
    (void)rsa;
    (void)filename;
    (void)passphrase;

    return _libssh2_error(session, LIBSSH2_ERROR_FILE,
                          "Unable to load RSA key from private key file: "
                          "Method unsupported in Windows CNG backend");
#endif /* HAVE_LIBCRYPT32 */
}

int
_libssh2_wincng_rsa_new_private_frommemory(libssh2_rsa_ctx **rsa,
                                           LIBSSH2_SESSION *session,
                                           const char *filedata,
                                           size_t filedata_len,
                                           const unsigned char *passphrase)
{
#ifdef HAVE_LIBCRYPT32
    unsigned char *pbEncoded;
    unsigned long cbEncoded;
    int ret;

    (void)session;

    ret = _libssh2_wincng_load_private_memory(session, filedata, filedata_len,
                                              passphrase,
                                              &pbEncoded, &cbEncoded, 1, 0);
    if(ret) {
        return -1;
    }

    return _libssh2_wincng_rsa_new_private_parse(rsa, session,
                                                 pbEncoded, cbEncoded);
#else
    (void)rsa;
    (void)filedata;
    (void)filedata_len;
    (void)passphrase;

    return _libssh2_error(session, LIBSSH2_ERROR_METHOD_NOT_SUPPORTED,
                          "Unable to extract private key from memory: "
                          "Method unsupported in Windows CNG backend");
#endif /* HAVE_LIBCRYPT32 */
}

int
_libssh2_wincng_rsa_sha1_verify(libssh2_rsa_ctx *rsa,
                                const unsigned char *sig,
                                unsigned long sig_len,
                                const unsigned char *m,
                                unsigned long m_len)
{
    return _libssh2_wincng_key_sha_verify(rsa, SHA_DIGEST_LENGTH, sig, sig_len,
                                          m, m_len, BCRYPT_PAD_PKCS1);
}

int
_libssh2_wincng_rsa_sha2_verify(libssh2_rsa_ctx* rsa,
                                size_t hash_len,
                                const unsigned char *sig,
                                unsigned long sig_len,
                                const unsigned char *m,
                                unsigned long m_len)
{
    return _libssh2_wincng_key_sha_verify(rsa, hash_len, sig, sig_len, m,
                                          m_len, BCRYPT_PAD_PKCS1);
}

int
_libssh2_wincng_rsa_sha_sign(LIBSSH2_SESSION *session,
                              libssh2_rsa_ctx *rsa,
                              const unsigned char *hash,
                              size_t hash_len,
                              unsigned char **signature,
                              size_t *signature_len)
{
    BCRYPT_PKCS1_PADDING_INFO paddingInfo;
    unsigned char *data, *sig;
    unsigned long cbData, datalen, siglen;
    int ret;

    if(hash_len == SHA_DIGEST_LENGTH)
        paddingInfo.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    else if(hash_len == SHA256_DIGEST_LENGTH)
        paddingInfo.pszAlgId = BCRYPT_SHA256_ALGORITHM;
    else if(hash_len == SHA384_DIGEST_LENGTH)
        paddingInfo.pszAlgId = BCRYPT_SHA384_ALGORITHM;
    else if(hash_len == SHA512_DIGEST_LENGTH)
        paddingInfo.pszAlgId = BCRYPT_SHA512_ALGORITHM;
    else {
        _libssh2_error(session, LIBSSH2_ERROR_PROTO,
                      "Unsupported hash digest length");
        return -1;
    }

    datalen = (unsigned long)hash_len;
    data = malloc(datalen);
    if(!data) {
        return -1;
    }
    memcpy(data, hash, datalen);

    ret = BCryptSignHash(rsa->hKey, &paddingInfo,
                         data, datalen, NULL, 0,
                         &cbData, BCRYPT_PAD_PKCS1);
    if(BCRYPT_SUCCESS(ret)) {
        siglen = cbData;
        sig = LIBSSH2_ALLOC(session, siglen);
        if(sig) {
            ret = BCryptSignHash(rsa->hKey, &paddingInfo,
                                 data, datalen, sig, siglen,
                                 &cbData, BCRYPT_PAD_PKCS1);
            if(BCRYPT_SUCCESS(ret)) {
                *signature_len = siglen;
                *signature = sig;
            }
            else {
                LIBSSH2_FREE(session, sig);
            }
        }
        else
            ret = STATUS_NO_MEMORY;
    }

    _libssh2_wincng_safe_free(data, datalen);

    return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

void
_libssh2_wincng_rsa_free(libssh2_rsa_ctx *rsa)
{
    if(!rsa)
        return;

    BCryptDestroyKey(rsa->hKey);
    rsa->hKey = NULL;

    _libssh2_wincng_safe_free(rsa->pbKeyObject, rsa->cbKeyObject);
    _libssh2_wincng_safe_free(rsa, sizeof(libssh2_rsa_ctx));
}


/*******************************************************************/
/*
 * Windows CNG backend: DSA functions
 */

#if LIBSSH2_DSA
int
_libssh2_wincng_dsa_new(libssh2_dsa_ctx **dsa,
                        const unsigned char *pdata,
                        unsigned long plen,
                        const unsigned char *qdata,
                        unsigned long qlen,
                        const unsigned char *gdata,
                        unsigned long glen,
                        const unsigned char *ydata,
                        unsigned long ylen,
                        const unsigned char *xdata,
                        unsigned long xlen)
{
    BCRYPT_KEY_HANDLE hKey;
    BCRYPT_DSA_KEY_BLOB *dsakey;
    LPCWSTR lpszBlobType;
    unsigned char *key;
    unsigned long keylen, offset, length;
    int ret;

    length = max(max(_libssh2_wincng_bn_size(pdata, plen),
                     _libssh2_wincng_bn_size(gdata, glen)),
                 _libssh2_wincng_bn_size(ydata, ylen));
    offset = sizeof(BCRYPT_DSA_KEY_BLOB);
    keylen = offset + length * 3;
    if(xdata && xlen > 0)
        keylen += 20;

    key = malloc(keylen);
    if(!key) {
        return -1;
    }

    memset(key, 0, keylen);


    /* https://msdn.microsoft.com/library/windows/desktop/aa833126.aspx */
    dsakey = (BCRYPT_DSA_KEY_BLOB *)key;
    dsakey->cbKey = length;

    memset(dsakey->Count, -1, sizeof(dsakey->Count));
    memset(dsakey->Seed, -1, sizeof(dsakey->Seed));

    if(qlen < 20)
        memcpy(dsakey->q + 20 - qlen, qdata, qlen);
    else
        memcpy(dsakey->q, qdata + qlen - 20, 20);

    if(plen < length)
        memcpy(key + offset + length - plen, pdata, plen);
    else
        memcpy(key + offset, pdata + plen - length, length);
    offset += length;

    if(glen < length)
        memcpy(key + offset + length - glen, gdata, glen);
    else
        memcpy(key + offset, gdata + glen - length, length);
    offset += length;

    if(ylen < length)
        memcpy(key + offset + length - ylen, ydata, ylen);
    else
        memcpy(key + offset, ydata + ylen - length, length);

    if(xdata && xlen > 0) {
        offset += length;

        if(xlen < 20)
            memcpy(key + offset + 20 - xlen, xdata, xlen);
        else
            memcpy(key + offset, xdata + xlen - 20, 20);

        lpszBlobType = BCRYPT_DSA_PRIVATE_BLOB;
        dsakey->dwMagic = BCRYPT_DSA_PRIVATE_MAGIC;
    }
    else {
        lpszBlobType = BCRYPT_DSA_PUBLIC_BLOB;
        dsakey->dwMagic = BCRYPT_DSA_PUBLIC_MAGIC;
    }


    ret = BCryptImportKeyPair(_libssh2_wincng.hAlgDSA, NULL, lpszBlobType,
                              &hKey, key, keylen, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng_safe_free(key, keylen);
        return -1;
    }


    *dsa = malloc(sizeof(libssh2_dsa_ctx));
    if(!(*dsa)) {
        BCryptDestroyKey(hKey);
        _libssh2_wincng_safe_free(key, keylen);
        return -1;
    }

    (*dsa)->hKey = hKey;
    (*dsa)->pbKeyObject = key;
    (*dsa)->cbKeyObject = keylen;

    return 0;
}

#ifdef HAVE_LIBCRYPT32
static int
_libssh2_wincng_dsa_new_private_parse(libssh2_dsa_ctx **dsa,
                                      LIBSSH2_SESSION *session,
                                      unsigned char *pbEncoded,
                                      unsigned long cbEncoded)
{
    unsigned char **rpbDecoded;
    unsigned long *rcbDecoded, index, length;
    int ret;

    (void)session;

    ret = _libssh2_wincng_asn_decode_bns(pbEncoded, cbEncoded,
                                         &rpbDecoded, &rcbDecoded, &length);

    _libssh2_wincng_safe_free(pbEncoded, cbEncoded);

    if(ret) {
        return -1;
    }


    if(length == 6) {
        ret = _libssh2_wincng_dsa_new(dsa,
                                      rpbDecoded[1], rcbDecoded[1],
                                      rpbDecoded[2], rcbDecoded[2],
                                      rpbDecoded[3], rcbDecoded[3],
                                      rpbDecoded[4], rcbDecoded[4],
                                      rpbDecoded[5], rcbDecoded[5]);
    }
    else {
        ret = -1;
    }

    for(index = 0; index < length; index++) {
        _libssh2_wincng_safe_free(rpbDecoded[index], rcbDecoded[index]);
        rpbDecoded[index] = NULL;
        rcbDecoded[index] = 0;
    }

    free(rpbDecoded);
    free(rcbDecoded);

    return ret;
}
#endif /* HAVE_LIBCRYPT32 */

int
_libssh2_wincng_dsa_new_private(libssh2_dsa_ctx **dsa,
                                LIBSSH2_SESSION *session,
                                const char *filename,
                                const unsigned char *passphrase)
{
#ifdef HAVE_LIBCRYPT32
    unsigned char *pbEncoded;
    unsigned long cbEncoded;
    int ret;

    ret = _libssh2_wincng_load_private(session, filename, passphrase,
                                       &pbEncoded, &cbEncoded, 0, 1);
    if(ret) {
        return -1;
    }

    return _libssh2_wincng_dsa_new_private_parse(dsa, session,
                                                 pbEncoded, cbEncoded);
#else
    (void)dsa;
    (void)filename;
    (void)passphrase;

    return _libssh2_error(session, LIBSSH2_ERROR_FILE,
                          "Unable to load DSA key from private key file: "
                          "Method unsupported in Windows CNG backend");
#endif /* HAVE_LIBCRYPT32 */
}

int
_libssh2_wincng_dsa_new_private_frommemory(libssh2_dsa_ctx **dsa,
                                           LIBSSH2_SESSION *session,
                                           const char *filedata,
                                           size_t filedata_len,
                                           const unsigned char *passphrase)
{
#ifdef HAVE_LIBCRYPT32
    unsigned char *pbEncoded;
    unsigned long cbEncoded;
    int ret;

    ret = _libssh2_wincng_load_private_memory(session, filedata, filedata_len,
                                              passphrase,
                                              &pbEncoded, &cbEncoded, 0, 1);
    if(ret) {
        return -1;
    }

    return _libssh2_wincng_dsa_new_private_parse(dsa, session,
                                                 pbEncoded, cbEncoded);
#else
    (void)dsa;
    (void)filedata;
    (void)filedata_len;
    (void)passphrase;

    return _libssh2_error(session, LIBSSH2_ERROR_METHOD_NOT_SUPPORTED,
                          "Unable to extract private key from memory: "
                          "Method unsupported in Windows CNG backend");
#endif /* HAVE_LIBCRYPT32 */
}

int
_libssh2_wincng_dsa_sha1_verify(libssh2_dsa_ctx *dsa,
                                const unsigned char *sig_fixed,
                                const unsigned char *m,
                                unsigned long m_len)
{
    return _libssh2_wincng_key_sha_verify(dsa, SHA_DIGEST_LENGTH, sig_fixed,
                                          40, m, m_len, 0);
}

int
_libssh2_wincng_dsa_sha1_sign(libssh2_dsa_ctx *dsa,
                              const unsigned char *hash,
                              unsigned long hash_len,
                              unsigned char *sig_fixed)
{
    unsigned char *data, *sig;
    unsigned long cbData, datalen, siglen;
    int ret;

    datalen = hash_len;
    data = malloc(datalen);
    if(!data) {
        return -1;
    }

    memcpy(data, hash, datalen);

    ret = BCryptSignHash(dsa->hKey, NULL, data, datalen,
                         NULL, 0, &cbData, 0);
    if(BCRYPT_SUCCESS(ret)) {
        siglen = cbData;
        if(siglen == 40) {
            sig = malloc(siglen);
            if(sig) {
                ret = BCryptSignHash(dsa->hKey, NULL, data, datalen,
                                     sig, siglen, &cbData, 0);
                if(BCRYPT_SUCCESS(ret)) {
                    memcpy(sig_fixed, sig, siglen);
                }

                _libssh2_wincng_safe_free(sig, siglen);
            }
            else
                ret = STATUS_NO_MEMORY;
        }
        else
            ret = STATUS_NO_MEMORY;
    }

    _libssh2_wincng_safe_free(data, datalen);

    return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

void
_libssh2_wincng_dsa_free(libssh2_dsa_ctx *dsa)
{
    if(!dsa)
        return;

    BCryptDestroyKey(dsa->hKey);
    dsa->hKey = NULL;

    _libssh2_wincng_safe_free(dsa->pbKeyObject, dsa->cbKeyObject);
    _libssh2_wincng_safe_free(dsa, sizeof(libssh2_dsa_ctx));
}
#endif


/*******************************************************************/
/*
 * Windows CNG backend: Key functions
 */

#ifdef HAVE_LIBCRYPT32
static unsigned long
_libssh2_wincng_pub_priv_write(unsigned char *key,
                               unsigned long offset,
                               const unsigned char *bignum,
                               const unsigned long length)
{
    _libssh2_htonu32(key + offset, length);
    offset += 4;

    memcpy(key + offset, bignum, length);
    offset += length;

    return offset;
}

static int
_libssh2_wincng_pub_priv_keyfile_parse(LIBSSH2_SESSION *session,
                                       unsigned char **method,
                                       size_t *method_len,
                                       unsigned char **pubkeydata,
                                       size_t *pubkeydata_len,
                                       unsigned char *pbEncoded,
                                       unsigned long cbEncoded)
{
    unsigned char **rpbDecoded;
    unsigned long *rcbDecoded;
    unsigned char *key = NULL, *mth = NULL;
    unsigned long keylen = 0, mthlen = 0;
    unsigned long index, offset, length;
    int ret;

    ret = _libssh2_wincng_asn_decode_bns(pbEncoded, cbEncoded,
                                         &rpbDecoded, &rcbDecoded, &length);

    _libssh2_wincng_safe_free(pbEncoded, cbEncoded);

    if(ret) {
        return -1;
    }


    if(length == 9) { /* private RSA key */
        mthlen = 7;
        mth = LIBSSH2_ALLOC(session, mthlen);
        if(mth) {
            memcpy(mth, "ssh-rsa", mthlen);
        }
        else {
            ret = -1;
        }


        keylen = 4 + mthlen + 4 + rcbDecoded[2] + 4 + rcbDecoded[1];
        key = LIBSSH2_ALLOC(session, keylen);
        if(key) {
            offset = _libssh2_wincng_pub_priv_write(key, 0, mth, mthlen);

            offset = _libssh2_wincng_pub_priv_write(key, offset,
                                                    rpbDecoded[2],
                                                    rcbDecoded[2]);

            _libssh2_wincng_pub_priv_write(key, offset,
                                           rpbDecoded[1],
                                           rcbDecoded[1]);
        }
        else {
            ret = -1;
        }

    }
    else if(length == 6) { /* private DSA key */
        mthlen = 7;
        mth = LIBSSH2_ALLOC(session, mthlen);
        if(mth) {
            memcpy(mth, "ssh-dss", mthlen);
        }
        else {
            ret = -1;
        }

        keylen = 4 + mthlen + 4 + rcbDecoded[1] + 4 + rcbDecoded[2]
                            + 4 + rcbDecoded[3] + 4 + rcbDecoded[4];
        key = LIBSSH2_ALLOC(session, keylen);
        if(key) {
            offset = _libssh2_wincng_pub_priv_write(key, 0, mth, mthlen);

            offset = _libssh2_wincng_pub_priv_write(key, offset,
                                                    rpbDecoded[1],
                                                    rcbDecoded[1]);

            offset = _libssh2_wincng_pub_priv_write(key, offset,
                                                    rpbDecoded[2],
                                                    rcbDecoded[2]);

            offset = _libssh2_wincng_pub_priv_write(key, offset,
                                                    rpbDecoded[3],
                                                    rcbDecoded[3]);

            _libssh2_wincng_pub_priv_write(key, offset,
                                           rpbDecoded[4],
                                           rcbDecoded[4]);
        }
        else {
            ret = -1;
        }

    }
    else {
        ret = -1;
    }


    for(index = 0; index < length; index++) {
        _libssh2_wincng_safe_free(rpbDecoded[index], rcbDecoded[index]);
        rpbDecoded[index] = NULL;
        rcbDecoded[index] = 0;
    }

    free(rpbDecoded);
    free(rcbDecoded);


    if(ret) {
        if(mth)
            LIBSSH2_FREE(session, mth);
        if(key)
            LIBSSH2_FREE(session, key);
    }
    else {
        *method = mth;
        *method_len = mthlen;
        *pubkeydata = key;
        *pubkeydata_len = keylen;
    }

    return ret;
}
#endif /* HAVE_LIBCRYPT32 */

int
_libssh2_wincng_pub_priv_keyfile(LIBSSH2_SESSION *session,
                                 unsigned char **method,
                                 size_t *method_len,
                                 unsigned char **pubkeydata,
                                 size_t *pubkeydata_len,
                                 const char *privatekey,
                                 const char *passphrase)
{
#ifdef HAVE_LIBCRYPT32
    unsigned char *pbEncoded;
    unsigned long cbEncoded;
    int ret;

    ret = _libssh2_wincng_load_private(session, privatekey,
                                       (const unsigned char *)passphrase,
                                       &pbEncoded, &cbEncoded, 1, 1);
    if(ret) {
        return -1;
    }

    return _libssh2_wincng_pub_priv_keyfile_parse(session, method, method_len,
                                                  pubkeydata, pubkeydata_len,
                                                  pbEncoded, cbEncoded);
#else
    (void)method;
    (void)method_len;
    (void)pubkeydata;
    (void)pubkeydata_len;
    (void)privatekey;
    (void)passphrase;

    return _libssh2_error(session, LIBSSH2_ERROR_FILE,
                          "Unable to load public key from private key file: "
                          "Method unsupported in Windows CNG backend");
#endif /* HAVE_LIBCRYPT32 */
}

int
_libssh2_wincng_pub_priv_keyfilememory(LIBSSH2_SESSION *session,
                                       unsigned char **method,
                                       size_t *method_len,
                                       unsigned char **pubkeydata,
                                       size_t *pubkeydata_len,
                                       const char *privatekeydata,
                                       size_t privatekeydata_len,
                                       const char *passphrase)
{
#ifdef HAVE_LIBCRYPT32
    unsigned char *pbEncoded;
    unsigned long cbEncoded;
    int ret;

    ret = _libssh2_wincng_load_private_memory(session, privatekeydata,
                                              privatekeydata_len,
                                              (const unsigned char *)
                                                  passphrase,
                                              &pbEncoded, &cbEncoded, 1, 1);
    if(ret) {
        return -1;
    }

    return _libssh2_wincng_pub_priv_keyfile_parse(session, method, method_len,
                                                  pubkeydata, pubkeydata_len,
                                                  pbEncoded, cbEncoded);
#else
    (void)method;
    (void)method_len;
    (void)pubkeydata_len;
    (void)pubkeydata;
    (void)privatekeydata;
    (void)privatekeydata_len;
    (void)passphrase;

    return _libssh2_error(session, LIBSSH2_ERROR_METHOD_NOT_SUPPORTED,
               "Unable to extract public key from private key in memory: "
               "Method unsupported in Windows CNG backend");
#endif /* HAVE_LIBCRYPT32 */
}

/*******************************************************************/
/*
 * Windows CNG backend: Cipher functions
 */

int
_libssh2_wincng_cipher_init(_libssh2_cipher_ctx *ctx,
                            _libssh2_cipher_type(type),
                            unsigned char *iv,
                            unsigned char *secret,
                            int encrypt)
{
    BCRYPT_KEY_HANDLE hKey;
    BCRYPT_KEY_DATA_BLOB_HEADER *header;
    unsigned char *pbKeyObject, *pbIV, *key, *pbCtr, *pbIVCopy;
    unsigned long dwKeyObject, dwIV, dwCtrLength, dwBlockLength,
                  cbData, keylen;
    int ret;

    (void)encrypt;

    ret = BCryptGetProperty(*type.phAlg, BCRYPT_OBJECT_LENGTH,
                            (unsigned char *)&dwKeyObject,
                            sizeof(dwKeyObject),
                            &cbData, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        return -1;
    }

    ret = BCryptGetProperty(*type.phAlg, BCRYPT_BLOCK_LENGTH,
                            (unsigned char *)&dwBlockLength,
                            sizeof(dwBlockLength),
                            &cbData, 0);
    if(!BCRYPT_SUCCESS(ret)) {
        return -1;
    }

    pbKeyObject = malloc(dwKeyObject);
    if(!pbKeyObject) {
        return -1;
    }


    keylen = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + type.dwKeyLength;
    key = malloc(keylen);
    if(!key) {
        free(pbKeyObject);
        return -1;
    }


    header = (BCRYPT_KEY_DATA_BLOB_HEADER *)key;
    header->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
    header->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
    header->cbKeyData = type.dwKeyLength;

    memcpy(key + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER),
           secret, type.dwKeyLength);

    ret = BCryptImportKey(*type.phAlg, NULL, BCRYPT_KEY_DATA_BLOB, &hKey,
                          pbKeyObject, dwKeyObject, key, keylen, 0);

    _libssh2_wincng_safe_free(key, keylen);

    if(!BCRYPT_SUCCESS(ret)) {
        _libssh2_wincng_safe_free(pbKeyObject, dwKeyObject);
        return -1;
    }

    pbIV = NULL;
    pbCtr = NULL;
    dwIV = 0;
    dwCtrLength = 0;

    if(type.useIV || type.ctrMode) {
        pbIVCopy = malloc(dwBlockLength);
        if(!pbIVCopy) {
            BCryptDestroyKey(hKey);
            _libssh2_wincng_safe_free(pbKeyObject, dwKeyObject);
            return -1;
        }
        memcpy(pbIVCopy, iv, dwBlockLength);

        if(type.ctrMode) {
            pbCtr = pbIVCopy;
            dwCtrLength = dwBlockLength;
        }
        else if(type.useIV) {
            pbIV = pbIVCopy;
            dwIV = dwBlockLength;
        }
    }

    ctx->hKey = hKey;
    ctx->pbKeyObject = pbKeyObject;
    ctx->pbIV = pbIV;
    ctx->pbCtr = pbCtr;
    ctx->dwKeyObject = dwKeyObject;
    ctx->dwIV = dwIV;
    ctx->dwBlockLength = dwBlockLength;
    ctx->dwCtrLength = dwCtrLength;

    return 0;
}
int
_libssh2_wincng_cipher_crypt(_libssh2_cipher_ctx *ctx,
                             _libssh2_cipher_type(type),
                             int encrypt,
                             unsigned char *block,
                             size_t blocklen)
{
    unsigned char *pbOutput, *pbInput;
    unsigned long cbOutput, cbInput;
    int ret;

    (void)type;

    cbInput = (unsigned long)blocklen;

    if(type.ctrMode) {
        pbInput = ctx->pbCtr;
    }
    else {
        pbInput = block;
    }

    if(encrypt || type.ctrMode) {
        ret = BCryptEncrypt(ctx->hKey, pbInput, cbInput, NULL,
                            ctx->pbIV, ctx->dwIV, NULL, 0, &cbOutput, 0);
    }
    else {
        ret = BCryptDecrypt(ctx->hKey, pbInput, cbInput, NULL,
                            ctx->pbIV, ctx->dwIV, NULL, 0, &cbOutput, 0);
    }
    if(BCRYPT_SUCCESS(ret)) {
        pbOutput = malloc(cbOutput);
        if(pbOutput) {
            if(encrypt || type.ctrMode) {
                ret = BCryptEncrypt(ctx->hKey, pbInput, cbInput, NULL,
                                    ctx->pbIV, ctx->dwIV,
                                    pbOutput, cbOutput, &cbOutput, 0);
            }
            else {
                ret = BCryptDecrypt(ctx->hKey, pbInput, cbInput, NULL,
                                    ctx->pbIV, ctx->dwIV,
                                    pbOutput, cbOutput, &cbOutput, 0);
            }
            if(BCRYPT_SUCCESS(ret)) {
                if(type.ctrMode) {
                    _libssh2_xor_data(block, block, pbOutput, blocklen);
                    _libssh2_aes_ctr_increment(ctx->pbCtr, ctx->dwCtrLength);
                }
                else {
                    memcpy(block, pbOutput, cbOutput);
                }
            }

            _libssh2_wincng_safe_free(pbOutput, cbOutput);
        }
        else
            ret = STATUS_NO_MEMORY;
    }

    return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

void
_libssh2_wincng_cipher_dtor(_libssh2_cipher_ctx *ctx)
{
    BCryptDestroyKey(ctx->hKey);
    ctx->hKey = NULL;

    _libssh2_wincng_safe_free(ctx->pbKeyObject, ctx->dwKeyObject);
    ctx->pbKeyObject = NULL;
    ctx->dwKeyObject = 0;

    _libssh2_wincng_safe_free(ctx->pbIV, ctx->dwBlockLength);
    ctx->pbIV = NULL;
    ctx->dwBlockLength = 0;

    _libssh2_wincng_safe_free(ctx->pbCtr, ctx->dwCtrLength);
    ctx->pbCtr = NULL;
    ctx->dwCtrLength = 0;
}


/*******************************************************************/
/*
 * Windows CNG backend: BigNumber functions
 */

_libssh2_bn *
_libssh2_wincng_bignum_init(void)
{
    _libssh2_bn *bignum;

    bignum = (_libssh2_bn *)malloc(sizeof(_libssh2_bn));
    if(bignum) {
        bignum->bignum = NULL;
        bignum->length = 0;
    }

    return bignum;
}

static int
_libssh2_wincng_bignum_resize(_libssh2_bn *bn, unsigned long length)
{
    unsigned char *bignum;

    if(!bn)
        return -1;

    if(length == bn->length)
        return 0;

#ifdef LIBSSH2_CLEAR_MEMORY
    if(bn->bignum && bn->length > 0 && length < bn->length) {
        SecureZeroMemory(bn->bignum + length, bn->length - length);
    }
#endif

    bignum = realloc(bn->bignum, length);
    if(!bignum)
        return -1;

    bn->bignum = bignum;
    bn->length = length;

    return 0;
}

static int
_libssh2_wincng_bignum_rand(_libssh2_bn *rnd, int bits, int top, int bottom)
{
    unsigned char *bignum;
    unsigned long length;

    if(!rnd)
        return -1;

    length = (unsigned long) (ceil(((double)bits) / 8.0) *
                              sizeof(unsigned char));
    if(_libssh2_wincng_bignum_resize(rnd, length))
        return -1;

    bignum = rnd->bignum;

    if(_libssh2_wincng_random(bignum, length))
        return -1;

    /* calculate significant bits in most significant byte */
    bits %= 8;
    if(bits == 0)
        bits = 8;

    /* fill most significant byte with zero padding */
    bignum[0] &= ((1 << bits) - 1);

    /* set most significant bits in most significant byte */
    if(top == 0)
        bignum[0] |= (1 << (bits - 1));
    else if(top == 1)
        bignum[0] |= (3 << (bits - 2));

    /* make odd by setting first bit in least significant byte */
    if(bottom)
        bignum[length - 1] |= 1;

    return 0;
}

static int
_libssh2_wincng_bignum_mod_exp(_libssh2_bn *r,
                               _libssh2_bn *a,
                               _libssh2_bn *p,
                               _libssh2_bn *m)
{
    BCRYPT_KEY_HANDLE hKey;
    BCRYPT_RSAKEY_BLOB *rsakey;
    unsigned char *key, *bignum;
    unsigned long keylen, offset, length;
    int ret;

    if(!r || !a || !p || !m)
        return -1;

    offset = sizeof(BCRYPT_RSAKEY_BLOB);
    keylen = offset + p->length + m->length;

    key = malloc(keylen);
    if(!key)
        return -1;


    /* https://msdn.microsoft.com/library/windows/desktop/aa375531.aspx */
    rsakey = (BCRYPT_RSAKEY_BLOB *)key;
    rsakey->Magic = BCRYPT_RSAPUBLIC_MAGIC;
    rsakey->BitLength = m->length * 8;
    rsakey->cbPublicExp = p->length;
    rsakey->cbModulus = m->length;
    rsakey->cbPrime1 = 0;
    rsakey->cbPrime2 = 0;

    memcpy(key + offset, p->bignum, p->length);
    offset += p->length;

    memcpy(key + offset, m->bignum, m->length);
    offset = 0;

    ret = BCryptImportKeyPair(_libssh2_wincng.hAlgRSA, NULL,
                              BCRYPT_RSAPUBLIC_BLOB, &hKey, key, keylen, 0);
    if(BCRYPT_SUCCESS(ret)) {
        ret = BCryptEncrypt(hKey, a->bignum, a->length, NULL, NULL, 0,
                            NULL, 0, &length, BCRYPT_PAD_NONE);
        if(BCRYPT_SUCCESS(ret)) {
            if(!_libssh2_wincng_bignum_resize(r, length)) {
                length = max(a->length, length);
                bignum = malloc(length);
                if(bignum) {
                    memcpy_with_be_padding(bignum, length,
                                           a->bignum, a->length);

                    ret = BCryptEncrypt(hKey, bignum, length, NULL, NULL, 0,
                                        r->bignum, r->length, &offset,
                                        BCRYPT_PAD_NONE);

                    _libssh2_wincng_safe_free(bignum, length);

                    if(BCRYPT_SUCCESS(ret)) {
                        _libssh2_wincng_bignum_resize(r, offset);
                    }
                }
                else
                    ret = STATUS_NO_MEMORY;
            }
            else
                ret = STATUS_NO_MEMORY;
        }

        BCryptDestroyKey(hKey);
    }

    _libssh2_wincng_safe_free(key, keylen);

    return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

int
_libssh2_wincng_bignum_set_word(_libssh2_bn *bn, unsigned long word)
{
    unsigned long offset, number, bits, length;

    if(!bn)
        return -1;

    bits = 0;
    number = word;
    while(number >>= 1)
        bits++;
    bits++;

    length = (unsigned long) (ceil(((double)bits) / 8.0) *
                              sizeof(unsigned char));
    if(_libssh2_wincng_bignum_resize(bn, length))
        return -1;

    for(offset = 0; offset < length; offset++)
        bn->bignum[offset] = (word >> (offset * 8)) & 0xff;

    return 0;
}

unsigned long
_libssh2_wincng_bignum_bits(const _libssh2_bn *bn)
{
    unsigned char number;
    unsigned long offset, length, bits;

    if(!bn || !bn->bignum || !bn->length)
        return 0;

    offset = 0;
    length = bn->length - 1;
    while(!bn->bignum[offset] && offset < length)
        offset++;

    bits = (length - offset) * 8;
    number = bn->bignum[offset];
    while(number >>= 1)
        bits++;
    bits++;

    return bits;
}

void
_libssh2_wincng_bignum_from_bin(_libssh2_bn *bn, unsigned long len,
                                const unsigned char *bin)
{
    unsigned char *bignum;
    unsigned long offset, length, bits;

    if(!bn || !bin || !len)
        return;

    if(_libssh2_wincng_bignum_resize(bn, len))
        return;

    memcpy(bn->bignum, bin, len);

    bits = _libssh2_wincng_bignum_bits(bn);
    length = (unsigned long) (ceil(((double)bits) / 8.0) *
                              sizeof(unsigned char));

    offset = bn->length - length;
    if(offset > 0) {
        memmove(bn->bignum, bn->bignum + offset, length);

#ifdef LIBSSH2_CLEAR_MEMORY
        SecureZeroMemory(bn->bignum + length, offset);
#endif

        bignum = realloc(bn->bignum, length);
        if(bignum) {
            bn->bignum = bignum;
            bn->length = length;
        }
    }
}

void
_libssh2_wincng_bignum_to_bin(const _libssh2_bn *bn, unsigned char *bin)
{
    if(bin && bn && bn->bignum && bn->length > 0) {
        memcpy(bin, bn->bignum, bn->length);
    }
}

void
_libssh2_wincng_bignum_free(_libssh2_bn *bn)
{
    if(bn) {
        if(bn->bignum) {
            _libssh2_wincng_safe_free(bn->bignum, bn->length);
            bn->bignum = NULL;
        }
        bn->length = 0;
        _libssh2_wincng_safe_free(bn, sizeof(_libssh2_bn));
    }
}


/*******************************************************************/
/*
 * Windows CNG backend: Diffie-Hellman support.
 */

void
_libssh2_dh_init(_libssh2_dh_ctx *dhctx)
{
    /* Random from client */
    dhctx->dh_handle = NULL;
    dhctx->dh_params = NULL;
    dhctx->dh_privbn = NULL;
}

void
_libssh2_dh_dtor(_libssh2_dh_ctx *dhctx)
{
    if(dhctx->dh_handle) {
        BCryptDestroyKey(dhctx->dh_handle);
        dhctx->dh_handle = NULL;
    }
    if(dhctx->dh_params) {
        /* Since public dh_params are shared in clear text,
         * we don't need to securely zero them out here */
        free(dhctx->dh_params);
        dhctx->dh_params = NULL;
    }
    if(dhctx->dh_privbn) {
        _libssh2_wincng_bignum_free(dhctx->dh_privbn);
        dhctx->dh_privbn = NULL;
    }
}

/* Generates a Diffie-Hellman key pair using base `g', prime `p' and the given
 * `group_order'. Can use the given big number context `bnctx' if needed.  The
 * private key is stored as opaque in the Diffie-Hellman context `*dhctx' and
 * the public key is returned in `public'.  0 is returned upon success, else
 * -1.  */
int
_libssh2_dh_key_pair(_libssh2_dh_ctx *dhctx, _libssh2_bn *public,
                     _libssh2_bn *g, _libssh2_bn *p, int group_order)
{
    const int hasAlgDHwithKDF = _libssh2_wincng.hasAlgDHwithKDF;
    while(_libssh2_wincng.hAlgDH && hasAlgDHwithKDF != -1) {
        BCRYPT_DH_PARAMETER_HEADER *dh_params = NULL;
        unsigned long dh_params_len;
        unsigned char *blob = NULL;
        int status;
        /* Note that the DH provider requires that keys be multiples of 64 bits
         * in length. At the time of writing a practical observed group_order
         * value is 257, so we need to round down to 8 bytes of length (64/8)
         * in order for kex to succeed */
        DWORD key_length_bytes = max(round_down(group_order, 8),
                                     max(g->length, p->length));
        BCRYPT_DH_KEY_BLOB *dh_key_blob;
        LPCWSTR key_type;

        /* Prepare a key pair; pass the in the bit length of the key,
         * but the key is not ready for consumption until it is finalized. */
        status = BCryptGenerateKeyPair(_libssh2_wincng.hAlgDH,
                                       &dhctx->dh_handle,
                                       key_length_bytes * 8, 0);
        if(!BCRYPT_SUCCESS(status)) {
            return -1;
        }

        dh_params_len = sizeof(*dh_params) + 2 * key_length_bytes;
        blob = malloc(dh_params_len);
        if(!blob) {
            return -1;
        }

        /* Populate DH parameters blob; after the header follows the `p`
         * value and the `g` value. */
        dh_params = (BCRYPT_DH_PARAMETER_HEADER*)blob;
        dh_params->cbLength = dh_params_len;
        dh_params->dwMagic = BCRYPT_DH_PARAMETERS_MAGIC;
        dh_params->cbKeyLength = key_length_bytes;
        memcpy_with_be_padding(blob + sizeof(*dh_params), key_length_bytes,
                               p->bignum, p->length);
        memcpy_with_be_padding(blob + sizeof(*dh_params) + key_length_bytes,
                               key_length_bytes, g->bignum, g->length);

        status = BCryptSetProperty(dhctx->dh_handle, BCRYPT_DH_PARAMETERS,
                                   blob, dh_params_len, 0);
        if(hasAlgDHwithKDF == -1) {
            /* We know that the raw KDF is not supported, so discard this. */
            free(blob);
        }
        else {
            /* Pass ownership to dhctx; these parameters will be freed when
             * the context is destroyed. We need to keep the parameters more
             * easily available so that we have access to the `g` value when
             * _libssh2_dh_secret is called later. */
            dhctx->dh_params = dh_params;
        }
        dh_params = NULL;
        blob = NULL;

        if(!BCRYPT_SUCCESS(status)) {
            return -1;
        }

        status = BCryptFinalizeKeyPair(dhctx->dh_handle, 0);
        if(!BCRYPT_SUCCESS(status)) {
            return -1;
        }

        key_length_bytes = 0;
        if(hasAlgDHwithKDF == 1) {
            /* Now we need to extract the public portion of the key so that we
             * set it in the `public` bignum to satisfy our caller.
             * First measure up the size of the required buffer. */
            key_type = BCRYPT_DH_PUBLIC_BLOB;
        }
        else {
            /* We also need to extract the private portion of the key to
             * set it in the `*dhctx' bignum if the raw KDF is not supported.
             * First measure up the size of the required buffer. */
            key_type = BCRYPT_DH_PRIVATE_BLOB;
        }
        status = BCryptExportKey(dhctx->dh_handle, NULL, key_type,
                                 NULL, 0, &key_length_bytes, 0);
        if(!BCRYPT_SUCCESS(status)) {
            return -1;
        }

        blob = malloc(key_length_bytes);
        if(!blob) {
            return -1;
        }

        status = BCryptExportKey(dhctx->dh_handle, NULL, key_type,
                                 blob, key_length_bytes,
                                 &key_length_bytes, 0);
        if(!BCRYPT_SUCCESS(status)) {
            if(hasAlgDHwithKDF == 1) {
                /* We have no private data, because raw KDF is supported */
                free(blob);
            }
            else { /* we may have potentially private data, use secure free */
                _libssh2_wincng_safe_free(blob, key_length_bytes);
            }
            return -1;
        }

        if(hasAlgDHwithKDF == -1) {
            /* We know that the raw KDF is not supported, so discard this */
            BCryptDestroyKey(dhctx->dh_handle);
            dhctx->dh_handle = NULL;
        }

        /* BCRYPT_DH_PUBLIC_BLOB corresponds to a BCRYPT_DH_KEY_BLOB header
         * followed by the Modulus, Generator and Public data. Those components
         * each have equal size, specified by dh_key_blob->cbKey. */
        dh_key_blob = (BCRYPT_DH_KEY_BLOB*)blob;
        if(_libssh2_wincng_bignum_resize(public, dh_key_blob->cbKey)) {
            if(hasAlgDHwithKDF == 1) {
                /* We have no private data, because raw KDF is supported */
                free(blob);
            }
            else { /* we may have potentially private data, use secure free */
                _libssh2_wincng_safe_free(blob, key_length_bytes);
            }
            return -1;
        }

        /* Copy the public key data into the public bignum data buffer */
        memcpy(public->bignum,
               blob + sizeof(*dh_key_blob) + 2 * dh_key_blob->cbKey,
               dh_key_blob->cbKey);

        if(dh_key_blob->dwMagic == BCRYPT_DH_PRIVATE_MAGIC) {
            /* BCRYPT_DH_PRIVATE_BLOB additionally contains the Private data */
            dhctx->dh_privbn = _libssh2_wincng_bignum_init();
            if(!dhctx->dh_privbn) {
                _libssh2_wincng_safe_free(blob, key_length_bytes);
                return -1;
            }
            if(_libssh2_wincng_bignum_resize(dhctx->dh_privbn,
                                             dh_key_blob->cbKey)) {
                _libssh2_wincng_safe_free(blob, key_length_bytes);
                return -1;
            }

            /* Copy the private key data into the dhctx bignum data buffer */
            memcpy(dhctx->dh_privbn->bignum,
                   blob + sizeof(*dh_key_blob) + 3 * dh_key_blob->cbKey,
                   dh_key_blob->cbKey);

            /* Make sure the private key is an odd number, because only
             * odd primes can be used with the RSA-based fallback while
             * DH itself does not seem to care about it being odd or not. */
            if(!(dhctx->dh_privbn->bignum[dhctx->dh_privbn->length-1] % 2)) {
                _libssh2_wincng_safe_free(blob, key_length_bytes);
                /* discard everything first, then try again */
                _libssh2_dh_dtor(dhctx);
                _libssh2_dh_init(dhctx);
                continue;
            }
        }

        return 0;
    }

    /* Generate x and e */
    dhctx->dh_privbn = _libssh2_wincng_bignum_init();
    if(!dhctx->dh_privbn)
        return -1;
    if(_libssh2_wincng_bignum_rand(dhctx->dh_privbn, (group_order*8)-1, 0, -1))
        return -1;
    if(_libssh2_wincng_bignum_mod_exp(public, g, dhctx->dh_privbn, p))
        return -1;

    return 0;
}

/* Computes the Diffie-Hellman secret from the previously created context
 * `*dhctx', the public key `f' from the other party and the same prime `p'
 * used at context creation. The result is stored in `secret'.  0 is returned
 * upon success, else -1.  */
int
_libssh2_dh_secret(_libssh2_dh_ctx *dhctx, _libssh2_bn *secret,
                   _libssh2_bn *f, _libssh2_bn *p)
{
    if(_libssh2_wincng.hAlgDH && _libssh2_wincng.hasAlgDHwithKDF != -1 &&
       dhctx->dh_handle && dhctx->dh_params && f) {
        BCRYPT_KEY_HANDLE peer_public = NULL;
        BCRYPT_SECRET_HANDLE agreement = NULL;
        ULONG secret_len_bytes = 0;
        unsigned char *blob;
        int status;
        unsigned char *start, *end;
        BCRYPT_DH_KEY_BLOB *public_blob = NULL;
        DWORD key_length_bytes = max(f->length, dhctx->dh_params->cbKeyLength);
        DWORD public_blob_len = sizeof(*public_blob) + 3 * key_length_bytes;

        {
            /* Populate a BCRYPT_DH_KEY_BLOB; after the header follows the
             * Modulus, Generator and Public data. Those components must have
             * equal size in this representation. */
            unsigned char *dest;
            unsigned char *src;

            blob = malloc(public_blob_len);
            if(!blob) {
                return -1;
            }
            public_blob = (BCRYPT_DH_KEY_BLOB*)blob;
            public_blob->dwMagic = BCRYPT_DH_PUBLIC_MAGIC;
            public_blob->cbKey = key_length_bytes;

            dest = (unsigned char *)(public_blob + 1);
            src = (unsigned char *)(dhctx->dh_params + 1);

            /* Modulus (the p-value from the first call) */
            memcpy_with_be_padding(dest, key_length_bytes, src,
                                   dhctx->dh_params->cbKeyLength);
            /* Generator (the g-value from the first call) */
            memcpy_with_be_padding(dest + key_length_bytes, key_length_bytes,
                                   src + dhctx->dh_params->cbKeyLength,
                                   dhctx->dh_params->cbKeyLength);
            /* Public from the peer */
            memcpy_with_be_padding(dest + 2*key_length_bytes, key_length_bytes,
                                   f->bignum, f->length);
        }

        /* Import the peer public key information */
        status = BCryptImportKeyPair(_libssh2_wincng.hAlgDH, NULL,
                                     BCRYPT_DH_PUBLIC_BLOB, &peer_public, blob,
                                     public_blob_len, 0);
        if(!BCRYPT_SUCCESS(status)) {
            goto out;
        }

        /* Set up a handle that we can use to establish the shared secret
         * between ourselves (our saved dh_handle) and the peer. */
        status = BCryptSecretAgreement(dhctx->dh_handle, peer_public,
                                       &agreement, 0);
        if(!BCRYPT_SUCCESS(status)) {
            goto out;
        }

        /* Compute the size of the buffer that is needed to hold the derived
         * shared secret. */
        status = BCryptDeriveKey(agreement, BCRYPT_KDF_RAW_SECRET, NULL, NULL,
                                 0, &secret_len_bytes, 0);
        if(!BCRYPT_SUCCESS(status)) {
            if(status == STATUS_NOT_SUPPORTED) {
                _libssh2_wincng.hasAlgDHwithKDF = -1;
            }
            goto out;
        }

        /* Expand the secret bignum to be ready to receive the derived secret
         * */
        if(_libssh2_wincng_bignum_resize(secret, secret_len_bytes)) {
            status = STATUS_NO_MEMORY;
            goto out;
        }

        /* And populate the secret bignum */
        status = BCryptDeriveKey(agreement, BCRYPT_KDF_RAW_SECRET, NULL,
                                 secret->bignum, secret_len_bytes,
                                 &secret_len_bytes, 0);
        if(!BCRYPT_SUCCESS(status)) {
            if(status == STATUS_NOT_SUPPORTED) {
                _libssh2_wincng.hasAlgDHwithKDF = -1;
            }
            goto out;
        }

        /* Counter to all the other data in the BCrypt APIs, the raw secret is
         * returned to us in host byte order, so we need to swap it to big
         * endian order. */
        start = secret->bignum;
        end = secret->bignum + secret->length - 1;
        while(start < end) {
            unsigned char tmp = *end;
            *end = *start;
            *start = tmp;
            start++;
            end--;
        }

        status = 0;
        _libssh2_wincng.hasAlgDHwithKDF = 1;

out:
        if(peer_public) {
            BCryptDestroyKey(peer_public);
        }
        if(agreement) {
            BCryptDestroySecret(agreement);
        }
        if(status == STATUS_NOT_SUPPORTED &&
           _libssh2_wincng.hasAlgDHwithKDF == -1) {
            goto fb; /* fallback to RSA-based implementation */
        }
        return BCRYPT_SUCCESS(status) ? 0 : -1;
    }

fb:
    /* Compute the shared secret */
    return _libssh2_wincng_bignum_mod_exp(secret, f, dhctx->dh_privbn, p);
}

/* _libssh2_supported_key_sign_algorithms
 *
 * Return supported key hash algo upgrades, see crypto.h
 *
 */

const char *
_libssh2_supported_key_sign_algorithms(LIBSSH2_SESSION *session,
                                       unsigned char *key_method,
                                       size_t key_method_len)
{
    (void)session;

#if LIBSSH2_RSA_SHA2
    if(key_method_len == 7 &&
        memcmp(key_method, "ssh-rsa", key_method_len) == 0) {
        return "rsa-sha2-512,rsa-sha2-256,ssh-rsa";
    }
#endif

    return NULL;
}

#endif /* LIBSSH2_WINCNG */
