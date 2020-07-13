/*
   Based on https://github.com/jackjack-jj/jeeq, GPLv3.
   Specifically designed to use Smileycoin key pairs for encoding/decoding so
   this is not as general as jeeq.py
   All the math here is explained pretty well on this wikipedia page:
   https://en.wikipedia.org/wiki/ElGamal_encryption
 */
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <endian.h>

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/bio.h>

#include <vector>
#include <stdexcept>

#include "key.h"
#include "jeeq.h"

/* our secp256k1 curve and bitcoin generator
 */
static EC_GROUP *group;
static BN_CTX *ctx;

#define PRIVHEADER_LEN          9
#define PUBHEADER_LEN           7
#define PRIVKEY_LEN             32
#define COMPR_PUBKEY_LEN        33
#define UNCOMPR_PUBKEY_LEN      65
#define CHUNK_SIZE              32
#define VERSION                 0x00
#define GX_HEX                  "79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798"
#define GY_HEX                  "483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8"
#define GORDER_HEX              "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141"
#define GCOFACTOR_HEX           "01"

/* functions that return strings return NULL on error.
   functions that return size_t return 0 on error.
   other functions return 0 on error, 1 on success 
 */

/* write the private header: [version,1][length of len+checksum,2][len,4][checksum,2]
   to m.
 */
static int write_private_header(uint8_t *m, const uint8_t *nmsg, const uint32_t msg_length)
{
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256(nmsg, msg_length, hash);

    m[0] = VERSION;
    m[1] = 0x00;
    m[2] = 0x06;
    /* msg_length in big endian */
    *(uint32_t*)&m[3] = htobe32(msg_length);
    /* no need to convert to big endian */
    m[7] = hash[0];
    m[8] = hash[1];

    /* any error in here would result in a segfault anyways */
    return 1;
}

/* write the public header 6A6A[version,1][length of checksum,2][checksum,2]
   to enc.
 */
static int write_public_header(uint8_t *enc, const uint8_t *pub, const bool is_compressed)
{
    uint8_t hash[SHA256_DIGEST_LENGTH];
    size_t pubkey_length = is_compressed ? COMPR_PUBKEY_LEN : UNCOMPR_PUBKEY_LEN;

    SHA256(pub, pubkey_length, hash);

    enc[0] = 0x6a;
    enc[1] = 0x6a;
    enc[2] = VERSION;
    enc[3] = 0x00;
    enc[4] = 0x02;
    enc[5] = hash[0];
    enc[6] = hash[1];

    return 1;
}

/* take in the raw decoded string and read the header to ensure we decoded
   it correctly.
   return the message's length
 */
static int read_private_header(const uint8_t *dec, size_t *message_length)
{
    if (dec[0] != VERSION) return 0;
    if (dec[1] != 0x00) return 0;
    if (dec[2] != 0x06) return 0;

    uint32_t msg_len = be32toh(*(uint32_t*)&dec[3]);
    uint16_t msg_hash = be16toh(*(uint16_t*)&dec[7]);

    uint8_t mg[SHA256_DIGEST_LENGTH];
    SHA256(&dec[PRIVHEADER_LEN], msg_len, mg);

    if (msg_hash != be16toh(*(uint16_t*)mg)) return 0;
    *message_length = msg_len;

    return 1;
}

/* take in the encoded byte vector and our privkey and check wether
   our derived pubkey matches the pubkey given in the header
   return 1 on success
 */
static int read_public_header(const uint8_t *enc, const BIGNUM* bn_privkey, const bool is_compressed)
{
    /* could be replaced with memcmp */
    if (enc[0] != 0x6a) return 0;
    if (enc[1] != 0x6a) return 0;
    if (enc[2] != VERSION) return 0;
    if (enc[3] != 0x00) return 0;
    if (enc[4] != 0x02) return 0;

    uint16_t pubkey_checksum = be16toh(*(uint16_t*)&enc[5]);

    /* derive both pubkeys from privkey and check that either matches the key
       hashed in the public header
     */
    EC_POINT *pubkey_point = EC_POINT_new(group);
    EC_POINT_mul(group, pubkey_point, bn_privkey, NULL, NULL, ctx);
    
    if (is_compressed)
    {
        uint8_t *pubkey_compressed = (uint8_t*)OPENSSL_malloc(COMPR_PUBKEY_LEN);
        EC_POINT_point2oct(group, pubkey_point, POINT_CONVERSION_COMPRESSED, pubkey_compressed, COMPR_PUBKEY_LEN, ctx);

        uint8_t comppub_hash[SHA256_DIGEST_LENGTH];
        SHA256(pubkey_compressed, COMPR_PUBKEY_LEN, comppub_hash);
        uint16_t comppub_checksum = be16toh(*((uint16_t*)comppub_hash));
        OPENSSL_free(pubkey_compressed);

        if (comppub_checksum != pubkey_checksum)
        {
            EC_POINT_free(pubkey_point);
            return 0;
        }
    } 
    else
    {
        uint8_t *pubkey_uncompressed = (uint8_t*)OPENSSL_malloc(UNCOMPR_PUBKEY_LEN);
        EC_POINT_point2oct(group, pubkey_point, POINT_CONVERSION_UNCOMPRESSED, pubkey_uncompressed, UNCOMPR_PUBKEY_LEN, ctx);

        uint8_t uncomppub_hash[SHA256_DIGEST_LENGTH];
        SHA256(pubkey_uncompressed, UNCOMPR_PUBKEY_LEN, uncomppub_hash);
        uint16_t uncomppub_checksum = be16toh(*((uint16_t*)uncomppub_hash));
        OPENSSL_free(pubkey_uncompressed);

        if (uncomppub_checksum != pubkey_checksum)
        {
            EC_POINT_free(pubkey_point);
            return 0;
        }
    }

    EC_POINT_free(pubkey_point);
    return 1;
}

/* arguments: a pointer to an allocated bignum, reference to int, pointer
   to x and finally whether we want to get the odd or even y value for that x.
     since not every x has a corresponding y on the curve (don't know the math)
   we try first whether the x works by itself, if not we increment to it until
   we find a y that works. this offset is put in the first byte of the point,
   the first bit tells us whether the pubkey was compressed or uncompressed
   and the rest carries the offset to x needed to find a y.
 */
static int y_from_x(BIGNUM *y, size_t *offset, const BIGNUM *x, const bool odd)
{
    EC_POINT *M = EC_POINT_new(group);

    /* try to find y the easy way */
    if (EC_POINT_set_compressed_coordinates_GFp(group, M, x, odd, ctx) == 1) 
    {
        EC_POINT_get_affine_coordinates_GFp(group, M, NULL, y, ctx);
        *offset = 0;
        EC_POINT_free(M);
        return 1;
    }
    
    int ret = 0;

    BIGNUM *p = BN_new();
    BIGNUM *a = BN_new();
    BIGNUM *b = BN_new();

    EC_GROUP_get_curve_GFp(group, p, a, b, ctx);

    BIGNUM *Mx = BN_new();
    BIGNUM *My = BN_new();
    BIGNUM *My2 = BN_new();
    BIGNUM *aMx2 = BN_new();

    BIGNUM *half = BN_dup(p);
    BN_add_word(half, 1); 
    BN_div_word(half, 4);

    BN_copy(Mx, x);

    for (int i = 1; i < 128; i++)
    {
        BN_add_word(Mx, 1);   

        /* My2 = (Mx^2 * Mx mod p) */
        BN_sqr(My2, Mx, ctx);
        BN_mod_mul(My2, My2, Mx, p, ctx);

        BN_mod_sqr(aMx2, Mx, p, ctx);
        BN_mul(aMx2, aMx2, a, ctx);

        BN_mod(b, b, p, ctx);

        BN_add(My2, My2, aMx2);
        BN_add(My2, My2, b);

        BN_mod_exp(My, My2, half, p, ctx);

        EC_POINT_set_affine_coordinates_GFp(group, M, Mx, My, ctx);
        if (EC_POINT_is_on_curve(group, M, ctx))
        {
            if (odd == BN_is_bit_set(My, 0))
            {
                BN_copy(y, My);
                *offset = i;
            } 
            else 
            {
                BN_sub(y, p, My);
                *offset = i;
            }

            ret = 1;
            break;
        }
    }

    BN_free(p);
    BN_free(a);
    BN_free(b);
    BN_free(Mx);
    BN_free(My);
    BN_free(My2);
    BN_free(aMx2);
    BN_free(half);

    EC_POINT_free(M);

    return ret;
}

/* arguments: a pointer to write the encoded string, a pointer to the raw pubkey,
   pointer to the message that's to be encoded and the message length. enc needs to 
   point to PUBHEADER_LEN + 66*(msg_len/32) of available memory.
   returns how many bytes written, 0 on error
 */
static uint8_t *encrypt_message(const uint8_t *pubkey, const bool is_compressed, const uint8_t *msg, const size_t msg_len, size_t *enc_len)
{
    uint8_t *ret = NULL;

    EC_POINT *pk = EC_POINT_new(group);

    /* get so many blocks of 32B blocks that msg will fit
       could be done with shifting but who cares
     */
    int chunk_count = (PRIVHEADER_LEN + msg_len)/CHUNK_SIZE + 1;

    uint8_t *m = (uint8_t*)OPENSSL_zalloc(chunk_count * CHUNK_SIZE);

    write_private_header(m, msg, msg_len);
    memcpy(&m[PRIVHEADER_LEN], msg, msg_len);

    BIGNUM *bn_pubkey = BN_new();
    BN_bin2bn(&pubkey[1], 32, bn_pubkey);

    if (is_compressed)
    {
        EC_POINT_set_compressed_coordinates_GFp(group, pk, bn_pubkey, pubkey[0]==0x03, ctx);
    }
    else 
    {
        BIGNUM *bn_pubkey_extra = BN_new();

        BN_bin2bn(&pubkey[1+32], 32, bn_pubkey_extra);
        EC_POINT_set_affine_coordinates_GFp(group, pk, bn_pubkey, bn_pubkey_extra, ctx);

        BN_free(bn_pubkey_extra);
    }

    BIGNUM *rand = BN_new();
    BIGNUM *rand_range = BN_new();
    BIGNUM *Mx = BN_new();
    BIGNUM *My = BN_new();
    printf("the pointer: %p\n", Mx);
    EC_POINT *M = EC_POINT_new(group);
    EC_POINT *T = EC_POINT_new(group);
    EC_POINT *U = EC_POINT_new(group);
    EC_GROUP_get_order(group, rand_range, ctx);
    BN_sub_word(rand_range, 1);

    uint8_t *enc = (uint8_t*)malloc(chunk_count * 2*COMPR_PUBKEY_LEN);
    write_public_header(enc, pubkey, is_compressed);
    int enc_loc = PUBHEADER_LEN;
    int m_loc = 0;
    size_t xoffset = 0;

    for (int i = 0; i < chunk_count; i++)
    {
        /* since rand must be in [1,...,q-1] */
        BN_rand_range(rand, rand_range); 
        BN_add_word(rand, 1);

        BN_bin2bn(&m[m_loc], CHUNK_SIZE, Mx);

        if (y_from_x(My, &xoffset, Mx, true) == 0)
        {
            ret = NULL;
            goto err;
        } 

        BN_add_word(Mx, xoffset);

        EC_POINT_set_affine_coordinates_GFp(group, M, Mx, My, ctx);

        EC_POINT_mul(group, T, rand, NULL, NULL, ctx);
        EC_POINT_mul(group, U, NULL, pk, rand, ctx);
        EC_POINT_add(group, U, U, M, ctx);

        EC_POINT_point2oct(group, T, POINT_CONVERSION_COMPRESSED, 
                &enc[enc_loc], COMPR_PUBKEY_LEN, ctx);
        EC_POINT_point2oct(group, U, POINT_CONVERSION_COMPRESSED, 
                &enc[enc_loc+COMPR_PUBKEY_LEN], COMPR_PUBKEY_LEN, ctx);

        enc[enc_loc] = enc[enc_loc] - 2 + (xoffset << 1);

        enc_loc += 2*COMPR_PUBKEY_LEN;
        m_loc += CHUNK_SIZE;
    }

    *enc_len = enc_loc;
    ret = enc;

err:
    puts("1");
    OPENSSL_clear_free(m, chunk_count * CHUNK_SIZE);
    puts("2");
    BN_free(rand);
    puts("3");
    BN_free(rand_range);
    puts("4");
    BN_free(Mx);
    puts("5");
    BN_free(My);
    puts("6");
    BN_free(bn_pubkey);
    puts("7");
    EC_POINT_free(M);
    EC_POINT_free(T);
    EC_POINT_free(U);
    EC_POINT_free(pk);

    return ret;
}

/* arguments: a pointer to write the decrypted message, should point to at least the
   length of the encoded string divided by 66 (2*compressed point length) of free 
   memory. a pointer to the raw private key, a pointer to the encoded string and 
   finally its length.
   returns how many bytes written, 0 on error
 */
static uint8_t *decrypt_message(const uint8_t *privkey, const bool is_compressed, 
                                const uint8_t *enc, const size_t enc_len, size_t *msg_len)
{
    BIGNUM *bn_privkey = BN_new();
    BN_bin2bn(privkey, PRIVKEY_LEN, bn_privkey);

    /* check that the public header is valid and matches our privkey */
    if (read_public_header(enc, bn_privkey, is_compressed) == 0) return NULL;

    int chunk_count = (enc_len - PUBHEADER_LEN) / (2*COMPR_PUBKEY_LEN);

    uint8_t *r = (uint8_t*)OPENSSL_malloc(chunk_count * CHUNK_SIZE);

    uint8_t *Tser = (uint8_t*)OPENSSL_malloc(COMPR_PUBKEY_LEN);
    uint8_t *User = (uint8_t*)OPENSSL_malloc(COMPR_PUBKEY_LEN);
    int xoffset = 0;

    EC_POINT *T = EC_POINT_new(group);
    EC_POINT *U = EC_POINT_new(group);
    EC_POINT *M = EC_POINT_new(group);
    EC_POINT *V = EC_POINT_new(group);

    BIGNUM *Mx = BN_new();

    int enc_loc = PUBHEADER_LEN;
    int r_loc = 0;
    for (int i = 0; i < chunk_count; i++)
    {
        memcpy(Tser, &enc[enc_loc], COMPR_PUBKEY_LEN);
        memcpy(User, &enc[enc_loc+COMPR_PUBKEY_LEN], COMPR_PUBKEY_LEN);
        xoffset = Tser[0] >> 1;
        Tser[0] = 2 + (Tser[0]&1);

        EC_POINT_oct2point(group, T, Tser, COMPR_PUBKEY_LEN, ctx);
        EC_POINT_oct2point(group, U, User, COMPR_PUBKEY_LEN, ctx);

        EC_POINT_mul(group, V, NULL, T, bn_privkey, ctx);
        EC_POINT_invert(group, V, ctx);
        EC_POINT_add(group, M, U, V, ctx);
        
        EC_POINT_get_affine_coordinates_GFp(group, M, Mx, NULL, ctx);

        BN_sub_word(Mx, xoffset);

        BN_bn2binpad(Mx, &r[r_loc], CHUNK_SIZE);

        r_loc += CHUNK_SIZE;
        enc_loc += 2*COMPR_PUBKEY_LEN;
    }

    OPENSSL_free(Tser);
    OPENSSL_free(User);

    EC_POINT_clear_free(V);
    EC_POINT_clear_free(U);
    EC_POINT_clear_free(M);
    EC_POINT_clear_free(T);

    BN_clear_free(bn_privkey);

    size_t s = 0;
    if (read_private_header(r, &s) == 0) return NULL;
    uint8_t *msg = (uint8_t*)malloc(s);
    memcpy(msg, &r[PRIVHEADER_LEN], s);
    *msg_len = s;

    OPENSSL_clear_free(r, r_loc);

    return msg;
}

using namespace std;

namespace Jeeq {

int Init()
{
    BIGNUM *cofactor;
    BIGNUM *order;
    EC_POINT *generator;

    ctx = BN_CTX_new();
    group = EC_GROUP_new_by_curve_name(NID_secp256k1);

    generator = EC_POINT_new(group);
    order = BN_new();
    cofactor = BN_new();

    BN_hex2bn(&order, GORDER_HEX);
    BN_hex2bn(&cofactor, GCOFACTOR_HEX);

    BIGNUM *Gx = BN_new();
    BIGNUM *Gy = BN_new();

    BN_hex2bn(&Gx, GX_HEX);
    BN_hex2bn(&Gy, GY_HEX);

    EC_POINT_set_affine_coordinates_GFp(group, generator, Gx, Gy, ctx);
    EC_GROUP_set_generator(group, generator, order, cofactor);

    BN_free(Gx);
    BN_free(Gy);
    BN_free(order);
    BN_free(cofactor);
    
    return 1;
}

int Cleanup()
{
    BN_CTX_free(ctx);
    EC_GROUP_free(group);

    return 1;
}

vector<uint8_t> EncryptMessage(const CPubKey pubkey, const vector<uint8_t> msg)
{
    // check pubkey
    if (!pubkey.IsValid()) 
        throw runtime_error("Jeeq::EncryptMessage(): called with an invalid public key");
    //  ensure that msg is not empty
    if (msg.size() == 0) throw runtime_error("Jeeq::EncryptMessage(): no message to encrypt");

    size_t enc_len = 0;
    uint8_t *benc = encrypt_message(pubkey.begin(), pubkey.IsCompressed(), msg.data(), msg.size(), &enc_len);
    if (benc == NULL) 
        throw runtime_error("Jeeq::EncryptMessage(): failed to encrypt message");

    vector<uint8_t> enc(benc, benc+enc_len);
    return enc;
}

vector<uint8_t> DecryptMessage(const CKey privkey, const vector<uint8_t> enc)
{
    // check privkey
    if (!privkey.IsValid()) 
        throw runtime_error("Jeeq::DecryptMessage(): called with an invalid private key");
    // ensure that enc is not empty
    if (enc.size() == 0) 
        throw runtime_error("Jeeq::DecryptMessage(): no encoded message to decrypt");

    size_t dec_len = 0;
    uint8_t *bdec = decrypt_message(privkey.begin(), privkey.IsCompressed(), 
                      enc.data(), enc.size(), &dec_len);
    if (bdec == NULL) 
        throw runtime_error("Jeeq::DecryptMessage(): failed to decrypt message");

    vector<uint8_t> dec(bdec, bdec+dec_len);
    return dec;
}

}

